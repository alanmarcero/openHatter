#include "Panels.h"
#include <BinaryData.h>
#include "Parameters.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <algorithm>
#include <ranges>

namespace openhatter::ui {
namespace {

void drawSection(juce::Graphics& g, juce::Rectangle<int> bounds,
                 const juce::String& title, juce::Colour accent) {
    g.setColour(theme::kPanel);
    g.fillRoundedRectangle(bounds.toFloat(), 9.0f);
    g.setColour(accent);
    g.fillRoundedRectangle(static_cast<float>(bounds.getX() + 16), static_cast<float>(bounds.getY() + 21), 18, 3, 1.5f);
    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    g.drawText(title, bounds.getX() + 43, bounds.getY() + 5, bounds.getWidth() - 58, 35,
               juce::Justification::centredLeft);
}

void layoutRows(juce::Rectangle<int> area,
                std::initializer_list<std::initializer_list<juce::Component*>> rows) {
    const int rowCount = static_cast<int>(rows.size());
    if (rowCount == 0) return;
    const int rowHeight = area.getHeight() / rowCount;
    int y = area.getY();
    for (const auto& row : rows) {
        auto rowArea = juce::Rectangle<int>(area.getX(), y, area.getWidth(), rowHeight);
        layoutRow(rowArea, row);
        y += rowHeight;
    }
}

} // namespace

MidiMonitorPanel::MidiMonitorPanel(OpenHatterProcessor& processor)
    : processor_(processor),
      cymbals_(juce::Drawable::createFromImageData(BinaryData::cymbalmotion_svg, BinaryData::cymbalmotion_svgSize)) {
    if (cymbals_) upperPlate_ = cymbals_->findChildWithID("upper-plate");
    timerCallback();
    startTimerHz(30);
}

MidiMonitorPanel::~MidiMonitorPanel() { stopTimer(); }

void MidiMonitorPanel::timerCallback() {
    const auto hostPhase = processor_.displayBeatPhase.load(std::memory_order_relaxed);
    const float phase = hostPhase >= 0 ? hostPhase : static_cast<float>(std::fmod(
        (juce::Time::getMillisecondCounterHiRes() - openedAt_) * processor_.displayBpm.load() / 60000.0, 1.0));
    if (!juce::approximatelyEqual(phase, beatPhase_)) {
        beatPhase_ = phase;
        if (isShowing()) repaint(0, 0, 310, getHeight());
    }
    const int note = processor_.displayMidiNoteReceived.load() ? processor_.displayMidiNoteNumber.load() : -1;
    const float velocity = processor_.displayVelocity.load();
    const float decayScale = std::clamp(processor_.parameters.getRawParameterValue(params::kDecayScale)->load(), 0.25f, 2.0f);
    const float air = processor_.parameters.getRawParameterValue(params::kNoiseDecay)->load() * decayScale;
    if (note != lastNote_ || !juce::approximatelyEqual(velocity, lastVelocity_)
        || !juce::approximatelyEqual(air, airMs_)) {
        lastNote_ = note;
        lastVelocity_ = velocity;
        airMs_ = air;
        repaint();
    }
}

void MidiMonitorPanel::paint(juce::Graphics& g) {
    const auto bounds = getLocalBounds().toFloat();
    g.setGradientFill(juce::ColourGradient(theme::kPanel, bounds.getTopLeft(),
                                            theme::kBackground, bounds.getBottomRight(), false));
    g.fillRoundedRectangle(bounds, 10.0f);
    g.setColour(theme::kPanelEdge);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 10.0f, 1.0f);
    if (cymbals_) {
        // An open offbeat stroke lifts the top plate and lets it settle before the next beat.
        const float distance = (beatPhase_ - .5f) * 7.0f;
        const float lift = std::exp(-distance * distance);
        if (upperPlate_)
            upperPlate_->setTransform(juce::AffineTransform::rotation(-.035f * lift, 150, 83).translated(0, -14 * lift));
        cymbals_->draw(g, 1.0f, juce::AffineTransform::translation(12, 10));
    }

    const int pitchX = getWidth() - 234;
    g.setColour(theme::kPanelEdge);
    g.drawVerticalLine(pitchX - 26, 24.0f, bounds.getHeight() - 24.0f);
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.setColour(theme::kText);
    g.drawText("Layer decay", 326, 16, 200, 22, juce::Justification::left);
    const float scale = std::max(250.0f, airMs_);
    const auto decayBar = [&](int y, const char* label, float value, juce::Colour colour) {
        g.setFont(juce::FontOptions(15.0f));
        g.setColour(theme::kTextDim);
        g.drawText(label, 326, y, 90, 22, juce::Justification::left);
        g.setColour(theme::kText);
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText(juce::String(juce::roundToInt(value)) + " ms", pitchX - 131, y, 70, 22, juce::Justification::right);
        const juce::Rectangle<float> track(326, static_cast<float>(y + 27), static_cast<float>(pitchX - 387), 4);
        g.setColour(theme::kPanelEdge);
        g.fillRoundedRectangle(track, 2);
        g.setColour(colour);
        g.fillRoundedRectangle(track.withWidth(track.getWidth() * value / scale), 2);
    };
    decayBar(71, "Air", airMs_, theme::kAccentAqua);

    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText("MIDI input", pitchX, 21, 200, 21, juce::Justification::left);
    const bool received = lastNote_ >= 0;
    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(received ? 43.0f : 25.0f, juce::Font::bold));
    g.drawText(received ? juce::MidiMessage::getMidiNoteName(lastNote_, true, true, 4) : juce::String("Waiting for MIDI"),
               pitchX, 45, 222, 54, juce::Justification::left);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(15.0f));
    if (received) {
        g.drawText(juce::String(processor_.displayMidiFrequency.load(), 1) + " Hz", pitchX, 98, 211, 20, juce::Justification::left);
        g.drawText("Note " + juce::String(lastNote_) + "   /   Velocity " + juce::String(juce::roundToInt(lastVelocity_ * 127)),
                   pitchX, 120, 212, 21, juce::Justification::left);
    } else {
        g.drawText("Play a note to set the pitch.", pitchX, 99, 224, 24, juce::Justification::left);
    }
}

HatSynthPanel::HatSynthPanel(OpenHatterProcessor& processor)
    : noiseType_(processor.parameters, params::kNoiseType, "Air texture", theme::kAccentAqua),
      noiseAttack_(processor.parameters, params::kNoiseAttack, "Attack", theme::kAccentAqua),
      noiseHold_(processor.parameters, params::kNoiseHold, "Hold", theme::kAccentAqua),
      noiseDecay_(processor.parameters, params::kNoiseDecay, "Decay", theme::kAccentAqua),
      noiseCurve_(processor.parameters, params::kNoiseCurve, "Curve", theme::kAccentAqua),
      noiseHPF_(processor.parameters, params::kNoiseHPF, "High-pass", theme::kAccentAqua),
      noiseLPF_(processor.parameters, params::kNoiseLPF, "Low-pass", theme::kAccentAqua),
      noiseWidth_(processor.parameters, params::kNoiseStereoWidth, "Width", theme::kAccentAqua),
      decayScale_(processor.parameters, params::kDecayScale, "Decay morph", theme::kAccentAqua),
      noiseLevel_(processor.parameters, params::kNoiseLevel, "Level", theme::kAccentAqua),
      preBursts_(processor.parameters, params::kPreBursts, "Strikes", theme::kAccentCoral),
      preSpread_(processor.parameters, params::kPreSpread, "Spread", theme::kAccentCoral),
      preDecay_(processor.parameters, params::kPreDecay, "Decay", theme::kAccentCoral),
      preJitter_(processor.parameters, params::kPreJitter, "Humanize", theme::kAccentCoral),
      flamSpread_(processor.parameters, params::kFlamSpread, "Pan", theme::kAccentCoral),
      preLevel_(processor.parameters, params::kPreLevel, "Bounce level", theme::kAccentCoral),
      clickType_(processor.parameters, params::kClickType, "Contact", theme::kAccentCoral),
      clickLevel_(processor.parameters, params::kClickLevel, "Strike level", theme::kAccentCoral),
      clickFreq_(processor.parameters, params::kClickFreq, "Frequency", theme::kAccentCoral),
      articulationLock_("Articulation", theme::kAccentCoral,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::articulation); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::articulation, on); }),
      airLock_("Air", theme::kAccentAqua,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::air); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::air, on); }) {
    decayScale_.setTooltip("Scale air and bounce decays together to morph between closed and open hats.");
    for (juce::Component* child : std::initializer_list<juce::Component*>{
             &noiseType_, &noiseAttack_,
             &noiseHold_, &noiseDecay_, &noiseCurve_, &noiseHPF_, &noiseLPF_, &noiseWidth_, &decayScale_, &noiseLevel_,
             &preBursts_, &preSpread_, &preDecay_, &preJitter_, &flamSpread_, &preLevel_, &clickType_,
             &clickLevel_, &clickFreq_, &articulationLock_, &airLock_ })
        addAndMakeVisible(child);
    setAdvancedMode(false);
}

void HatSynthPanel::setAdvancedMode(bool advanced) {
    advanced_ = advanced;
    for (auto* knob : { &noiseAttack_, &noiseHold_, &noiseCurve_, &noiseHPF_, &noiseWidth_,
                        &decayScale_, &preSpread_, &preDecay_, &preJitter_, &flamSpread_, &preLevel_ })
        knob->setVisible(advanced);
    resized();
}

void HatSynthPanel::paint(juce::Graphics& g) {
    drawSection(g, sectionBounds_[0], "Air tail", theme::kAccentAqua);
    drawSection(g, sectionBounds_[1], "Stick & bounce", theme::kAccentCoral);
}

void HatSynthPanel::resized() {
    const int width = (getWidth() - 10) / 2;
    for (int i = 0; i < 2; ++i)
        sectionBounds_[static_cast<size_t>(i)] = { i * (width + 10), 0, width, getHeight() };
    auto air = sectionBounds_[0].reduced(11, 9);
    auto stick = sectionBounds_[1].reduced(11, 9);
    for (auto [area, lock] : { std::pair{ &air, &airLock_ }, { &stick, &articulationLock_ } })
        lock->setBounds(area->removeFromTop(36).removeFromRight(56));
    for (auto [area, choice] : { std::pair{ &air, &noiseType_ }, { &stick, &clickType_ } })
        choice->setBounds(area->removeFromTop(advanced_ ? 68 : 83));

    const int rowHeight = advanced_ ? air.getHeight() / 3 : air.getHeight();
    layoutRow(air.removeFromTop(rowHeight), { &noiseDecay_, &noiseLPF_, &noiseLevel_ });
    layoutRow(stick.removeFromTop(rowHeight), { &preBursts_, &clickFreq_, &clickLevel_ });
    if (advanced_) {
        layoutRows(air, { { &noiseAttack_, &noiseHold_, &noiseCurve_ }, { &noiseHPF_, &noiseWidth_, &decayScale_ } });
        layoutRows(stick, { { &preSpread_, &preDecay_, &preLevel_ }, { &preJitter_, &flamSpread_ } });
    }
}

EnvelopePanel::EnvelopePanel(OpenHatterProcessor& processor)
    : attack_(processor.parameters, params::kAmpAttack, "Attack", theme::kAccentBrass),
      decay_(processor.parameters, params::kAmpDecay, "Decay", theme::kAccentBrass),
      sustain_(processor.parameters, params::kAmpSustain, "Sustain", theme::kAccentAqua),
      release_(processor.parameters, params::kAmpRelease, "Release", theme::kAccentCoral),
      envelopeLock_("Envelope", theme::kAccentBrass,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::envelope); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::envelope, on); }),
      gateAttachment_(*processor.parameters.getParameter(params::kAmpEnabled), [this](float value) {
          holdNotes_.setToggleState(value > .5f, juce::dontSendNotification);
          oneShot_.setToggleState(value <= .5f, juce::dontSendNotification);
          repaint();
      }) {
    oneShot_.setTooltip("Let each hit play its full air tail, even after key up.");
    holdNotes_.setTooltip("Sustain until a matching note-off on the original MIDI channel.");
    addAndMakeVisible(envelopeLock_);
    for (auto* button : { &oneShot_, &holdNotes_ }) {
        button->setToggleable(true);
        button->setColour(juce::TextButton::buttonOnColourId, theme::kAccentAqua);
        addAndMakeVisible(button);
    }
    oneShot_.onClick = [this] { gateAttachment_.setValueAsCompleteGesture(0.0f); };
    holdNotes_.onClick = [this] { gateAttachment_.setValueAsCompleteGesture(1.0f); };
    for (auto* knob : { &attack_, &decay_, &sustain_, &release_ }) {
        addAndMakeVisible(knob);
        knob->slider().onValueChange = [this] { repaint(); };
    }
    gateAttachment_.sendInitialUpdate();
}

void EnvelopePanel::paint(juce::Graphics& g) {
    drawSection(g, getLocalBounds(), "Amplitude envelope", theme::kAccentBrass);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(16.0f));
    const bool held = holdNotes_.getToggleState();
    g.drawText(held ? "Sustain the air until the matching MIDI note ends."
                    : "Play each strike to the end of its air tail.",
               22, 53, getWidth() - 290, 33, juce::Justification::left);
    auto plot = getLocalBounds().reduced(40, 0).withTrimmedTop(114).withTrimmedBottom(200).toFloat();
    const float base = plot.getBottom(), top = plot.getY();
    const auto relative = [](Knob& knob) { return static_cast<float>(knob.slider().valueToProportionOfLength(knob.slider().getValue())); };
    const float attackX = plot.getX() + plot.getWidth() * (.04f + .20f * relative(attack_));
    const float decayX = attackX + plot.getWidth() * (.10f + .20f * relative(decay_));
    const float releaseX = plot.getRight() - plot.getWidth() * (.08f + .20f * relative(release_));
    const float sustainY = base - plot.getHeight() * relative(sustain_);
    g.setColour(theme::kPanelEdge.withAlpha(.5f));
    for (int i = 0; i <= 4; ++i)
        g.drawHorizontalLine(juce::roundToInt(top + plot.getHeight() * static_cast<float>(i) / 4), plot.getX(), plot.getRight());
    juce::Path line;
    line.startNewSubPath(plot.getX(), base);
    line.lineTo(attackX, top);
    line.lineTo(decayX, sustainY);
    line.lineTo(releaseX, sustainY);
    line.lineTo(plot.getRight(), base);
    auto fill = line;
    fill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(theme::kAccentAqua.withAlpha(.25f), 0, top,
                                            theme::kAccentAqua.withAlpha(.015f), 0, base, false));
    g.fillPath(fill);
    g.setColour(theme::kAccentAqua);
    g.strokePath(line, juce::PathStrokeType(2.6f));
    for (auto point : { juce::Point<float>{attackX, top}, {decayX, sustainY}, {releaseX, sustainY} })
        g.fillEllipse(juce::Rectangle<float>(7, 7).withCentre(point));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText(held ? "Key down" : "Strike", juce::Rectangle<float>(plot.getX(), base + 8, 100, 20), juce::Justification::left);
    g.drawText(held ? "Key up" : "Tail fade", juce::Rectangle<float>(releaseX - 45, base + 8, 90, 20), juce::Justification::centred);
}

void EnvelopePanel::resized() {
    envelopeLock_.setBounds(getWidth() - 67, 7, 56, 36);
    oneShot_.setBounds(getWidth() - 252, 46, 110, 38);
    holdNotes_.setBounds(getWidth() - 138, 46, 114, 38);
    layoutRow(getLocalBounds().reduced(24, 0).removeFromBottom(161), { &attack_, &decay_, &sustain_, &release_ });
}

PerformancePanel::PerformancePanel(OpenHatterProcessor& processor)
    : velocityAir_(processor.parameters, params::kVelocityAir, "Air", theme::kAccentAqua),
      velocityChick_(processor.parameters, params::kVelocityChick, "Stick", theme::kAccentCoral),
      velocityReverb_(processor.parameters, params::kVelocityReverb, "Reverb", theme::kAccentAqua),
      chokeNote_(processor.parameters, params::kMidiChokeNote, "Choke note", theme::kAccentCoral),
      chokeCc_(processor.parameters, params::kMidiChokeCc, "Choke CC", theme::kAccentCoral),
      rollEnabled_(processor.parameters, params::kRollEnabled, "Roll off", "Roll on", theme::kAccentBrass),
      rollDivision_(processor.parameters, params::kRollDivision, "Division", theme::kAccentBrass),
      rollSwing_(processor.parameters, params::kRollSwing, "Swing", theme::kAccentBrass),
      rollAccent_(processor.parameters, params::kRollAccent, "Accent", theme::kAccentBrass) {
    for (juce::Component* component : std::array<juce::Component*, 9>{
             &velocityAir_, &velocityChick_, &velocityReverb_, &chokeNote_, &chokeCc_,
             &rollEnabled_, &rollDivision_, &rollSwing_, &rollAccent_})
        addAndMakeVisible(component);

    velocityAir_.setTooltip("Velocity curve for the filtered noise layer.");
    velocityChick_.setTooltip("Velocity curve for the chick and stick transient layer.");
    velocityReverb_.setTooltip("Velocity curve for the reverb return.");
    chokeNote_.setTooltip("A note-off for this MIDI note chokes the current hat and stops the roll.");
    chokeCc_.setTooltip("A value of 64 or higher on this CC chokes the current hat and stops the roll.");
    rollEnabled_.setTooltip("Start a host-tempo hat roll after the next MIDI note-on.");
    rollDivision_.setTooltip("Host-synced roll step size.");
    rollSwing_.setTooltip("Delays alternating roll steps while keeping the average rate on the host grid.");
    rollAccent_.setTooltip("Velocity boost for every fourth roll hit.");
}

void PerformancePanel::paint(juce::Graphics& g) {
    drawSection(g, velocityBounds_, "Velocity response", theme::kAccentAqua);
    drawSection(g, rollBounds_, "Host roll & MIDI choke", theme::kAccentCoral);
}

void PerformancePanel::resized() {
    auto top = getLocalBounds().removeFromTop(229);
    velocityBounds_ = top.removeFromLeft(397);
    rollBounds_ = top.withTrimmedLeft(10);
    layoutRow(velocityBounds_.reduced(10, 9).withTrimmedTop(40), { &velocityAir_, &velocityChick_, &velocityReverb_ });
    auto roll = rollBounds_.reduced(11, 9).withTrimmedTop(34);
    auto modes = roll.removeFromLeft(144);
    layoutRows(modes, { { &rollEnabled_ }, { &rollDivision_ } });
    layoutRow(roll, { &rollSwing_, &rollAccent_, &chokeNote_, &chokeCc_ });
}

EffectsPanel::EffectsPanel(OpenHatterProcessor& processor)
    : distType_(processor.parameters, params::kDistType, "Exciter type", theme::kAccentCoral),
      distDrive_(processor.parameters, params::kDistDrive, "Drive", theme::kAccentCoral),
      distTone_(processor.parameters, params::kDistTone, "Sheen", theme::kAccentCoral),
      distMix_(processor.parameters, params::kDistMix, "Mix", theme::kAccentCoral),
      compThresh_(processor.parameters, params::kCompThresh, "Threshold", theme::kAccentBrass),
      compRatio_(processor.parameters, params::kCompRatio, "Ratio", theme::kAccentBrass),
      compAttack_(processor.parameters, params::kCompAttack, "Attack", theme::kAccentBrass),
      compRelease_(processor.parameters, params::kCompRelease, "Release", theme::kAccentBrass),
      compMakeup_(processor.parameters, params::kCompMakeup, "Makeup", theme::kAccentBrass),
      compMix_(processor.parameters, params::kCompMix, "Mix", theme::kAccentBrass),
      reverbSize_(processor.parameters, params::kReverbSize, "Size", theme::kAccentAqua),
      reverbDecay_(processor.parameters, params::kReverbDecay, "Decay", theme::kAccentAqua),
      reverbDamp_(processor.parameters, params::kReverbDamp, "Damping", theme::kAccentAqua),
      reverbGate_(processor.parameters, params::kReverbGate, "Choke", theme::kAccentAqua),
      reverbMix_(processor.parameters, params::kReverbMix, "Mix", theme::kAccentAqua),
      echoMix_(processor.parameters, params::kEchoMix, "Echo mix", theme::kAccentAqua),
      reverseMode_(processor.parameters, params::kReverseMode, "Reverse sync", theme::kAccentAqua),
      reverseMix_(processor.parameters, params::kReverseMix, "Reverse mix", theme::kAccentAqua),
      exciterLock_("Exciter", theme::kAccentCoral,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::exciter); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::exciter, on); }),
      dynamicsLock_("Dynamics", theme::kAccentBrass,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::dynamics); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::dynamics, on); }),
      spaceLock_("Space", theme::kAccentAqua,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::spaceAndOutput); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::spaceAndOutput, on); }) {
    echoMix_.setTooltip("Blend a tempo-synced dotted-sixteenth stereo ping-pong echo.");
    std::ranges::for_each(std::array<juce::Component*, 21>{
             &distType_, &distDrive_, &distTone_, &distMix_, &compThresh_, &compRatio_, &compAttack_, &compRelease_,
             &compMakeup_, &compMix_, &reverbSize_, &reverbDecay_,
             &reverbDamp_, &reverbGate_, &reverbMix_, &echoMix_, &reverseMode_, &reverseMix_,
             &exciterLock_, &dynamicsLock_, &spaceLock_},
        [this](juce::Component* component) { addAndMakeVisible(component); });
}

void EffectsPanel::paint(juce::Graphics& g) {
    drawSection(g, sectionBounds_[0], "Exciter", theme::kAccentCoral);
    drawSection(g, sectionBounds_[1], "Dynamics", theme::kAccentBrass);
    drawSection(g, sectionBounds_[2], "Space", theme::kAccentAqua);
}

void EffectsPanel::resized() {
    constexpr int gap = 10;
    const int width = (getWidth() - gap * 2) / 3;
    std::ranges::for_each(std::views::iota(size_t{0}, sectionBounds_.size()), [&](size_t i) {
        sectionBounds_[i] = { static_cast<int>(i) * (width + gap), 0, width, getHeight() };
    });
    std::ranges::for_each(std::array{ std::pair{ 0, &exciterLock_ }, std::pair{ 1, &dynamicsLock_ }, std::pair{ 2, &spaceLock_ } },
        [&](const auto& pair) {
            pair.second->setBounds(sectionBounds_[static_cast<size_t>(pair.first)].getRight() - 67, 7, 56, 36);
        });

    layoutRows(sectionBounds_[0].reduced(8, 10).withTrimmedTop(36), {
        { &distType_, &distDrive_ },
        { &distTone_, &distMix_ }
    });
    layoutRows(sectionBounds_[1].reduced(7, 10).withTrimmedTop(36), {
        { &compThresh_, &compRatio_, &compAttack_ },
        { &compRelease_, &compMakeup_, &compMix_ }
    });
    layoutRows(sectionBounds_[2].reduced(7, 10).withTrimmedTop(36), {
        { &reverbSize_, &reverbDecay_, &reverbDamp_ },
        { &reverbGate_, &reverbMix_, &echoMix_ },
        { &reverseMode_, &reverseMix_ }
    });
}

OutputPanel::OutputPanel(OpenHatterProcessor& processor)
    : eqLow_(processor.parameters, params::kEqLowFreq, params::kEqLowDb, "Low EQ", theme::kAccentAqua, theme::kAccentBrass),
      eqMid_(processor.parameters, params::kEqMidFreq, params::kEqMidDb, "Mid EQ", theme::kAccentAqua, theme::kAccentBrass),
      eqHigh_(processor.parameters, params::kEqHighFreq, params::kEqHighDb, "High EQ", theme::kAccentAqua, theme::kAccentBrass),
      masterWidth_(processor.parameters, params::kStereoWidth, "Width", theme::kAccentAqua),
      outputGain_(processor.parameters, params::kOutputGain, "Output", theme::kAccentAqua),
      limiterCeiling_(processor.parameters, params::kLimiterCeiling, "Ceiling", theme::kAccentBrass),
      limiterRelease_(processor.parameters, params::kLimiterRelease, "Limit release", theme::kAccentBrass),
      limiterEnabled_(processor.parameters, params::kLimiterEnabled, "Limiter off", "Limiter on", theme::kAccentBrass),
      outputLock_("Output", theme::kAccentAqua,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::output); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::output, on); }) {
    eqLow_.setTooltip("Inner cap sets Low shelf gain (-12 to +12 dB); outer collar glides cutoff frequency smoothly.");
    eqMid_.setTooltip("Inner cap sets Mid peak gain (-12 to +12 dB); outer collar glides center frequency smoothly.");
    eqHigh_.setTooltip("Inner cap sets High shelf gain (-12 to +12 dB); outer collar glides cutoff frequency smoothly.");
    limiterEnabled_.setTooltip("Switch output peak limiting on or off.");
    std::ranges::for_each(std::array<juce::Component*, 9>{
             &eqLow_, &eqMid_, &eqHigh_, &masterWidth_, &outputGain_,
             &limiterCeiling_, &limiterRelease_, &limiterEnabled_, &outputLock_},
        [this](juce::Component* component) { addAndMakeVisible(component); });
}

void OutputPanel::paint(juce::Graphics& g) {
    drawSection(g, getLocalBounds(), "Output", theme::kAccentAqua);
}

void OutputPanel::resized() {
    outputLock_.setBounds(getWidth() - 67, 7, 56, 36);
    auto row = getLocalBounds().reduced(10, 6).withTrimmedTop(36);
    const std::array<std::pair<juce::Component*, float>, 8> items{{
        { &eqLow_, 14.0f }, { &eqMid_, 14.0f }, { &eqHigh_, 14.0f },
        { &masterWidth_, 9.5f }, { &outputGain_, 9.5f },
        { &limiterCeiling_, 9.5f }, { &limiterRelease_, 9.5f }, { &limiterEnabled_, 10.0f }
    }};
    const float total = 90.0f;
    const int gap = 4;
    const int avail = std::max(0, row.getWidth() - gap * static_cast<int>(items.size() - 1));
    std::ranges::for_each(items, [&](const auto& item) {
        const int w = juce::roundToInt(static_cast<float>(avail) * (item.second / total));
        item.first->setBounds(row.removeFromLeft(w));
        row.removeFromLeft(gap);
    });
}

} // namespace openhatter::ui
