#include "Panels.h"
#include <BinaryData.h>
#include "EdmLookAndFeel.h"
#include "Parameters.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <algorithm>
#include <numeric>
#include <ranges>

namespace openhatter::ui {
namespace {

constexpr const char* kVelocityCurveHint = "Below 1.00 lifts soft hits. Above 1.00 makes them quieter.";
constexpr int kLearnPollHz = 20;
constexpr int kLearnTimeoutTicks = kLearnPollHz * 10;

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
      cymbals_(juce::Drawable::createFromImageData(BinaryData::cymbalmotion_svg, BinaryData::cymbalmotion_svgSize)),
      holdAttachment_(*processor.parameters.getParameter(params::kAmpEnabled), [this](float value) {
          held_ = value > .5f;
          repaint();
      }) {
    if (cymbals_) upperPlate_ = cymbals_->findChildWithID("upper-plate");
    holdAttachment_.sendInitialUpdate();
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
    const float body = processor_.parameters.getRawParameterValue(params::kBodyDecay)->load() * decayScale;
    if (note != lastNote_ || !juce::approximatelyEqual(velocity, lastVelocity_)
        || !juce::approximatelyEqual(air, airMs_) || !juce::approximatelyEqual(body, bodyMs_)) {
        lastNote_ = note;
        lastVelocity_ = velocity;
        airMs_ = air;
        bodyMs_ = body;
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
    const float scale = std::max({ 250.0f, airMs_, bodyMs_ });
    const auto decayBar = [&](int y, const char* label, float value, juce::Colour colour) {
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.setColour(theme::kText.withAlpha(0.85f));
        g.drawText(label, 326, y, 90, 22, juce::Justification::left);
        g.setColour(theme::kText);
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        // Held notes keep both layers at full level until note-off.
        auto valueText = params::formatMs(value);
        if (held_) valueText = "Held";
        g.drawText(valueText, pitchX - 131, y, 70, 22, juce::Justification::right);
        const juce::Rectangle<float> track(326, static_cast<float>(y + 27), static_cast<float>(pitchX - 387), 4);
        g.setColour(theme::kPanelEdge);
        g.fillRoundedRectangle(track, 2);
        g.setColour(colour.withAlpha(held_ ? theme::kDimAlpha : 1.0f));
        g.fillRoundedRectangle(track.withWidth(track.getWidth() * (held_ ? 1.0f : value / scale)), 2);
    };
    decayBar(58, "Air", airMs_, theme::kAccentAqua);
    decayBar(104, "Body", bodyMs_, theme::kAccentBrass);

    g.setColour(theme::kText.withAlpha(0.88f));
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.drawText("MIDI input", pitchX, 16, 200, 22, juce::Justification::left);
    const bool received = lastNote_ >= 0;
    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(received ? 43.0f : 25.0f, juce::Font::bold));
    g.drawText(received ? juce::MidiMessage::getMidiNoteName(lastNote_, true, true, params::kMiddleCOctave) : juce::String("Waiting for MIDI"),
               pitchX, 45, 222, 54, juce::Justification::left);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(15.0f));
    if (received) {
        g.drawText(params::formatHz(processor_.displayMidiFrequency.load()), pitchX, 98, 211, 20, juce::Justification::left);
        g.setColour(theme::kText.withAlpha(0.92f));
        g.setFont(juce::FontOptions(16.0f, juce::Font::bold));
        g.drawText("Note " + juce::String(lastNote_) + " / Velocity " + juce::String(juce::roundToInt(lastVelocity_ * 127)),
                   pitchX, 119, 222, 24, juce::Justification::left);
    } else {
        g.drawText("Play a note to set the pitch.", pitchX, 99, 224, 24, juce::Justification::left);
    }
}

HatSynthPanel::HatSynthPanel(OpenHatterProcessor& processor)
    : noiseType_(processor.parameters, params::kNoiseType, "Air type", theme::kAccentAqua),
      noiseAttack_(processor.parameters, params::kNoiseAttack, "Attack", theme::kAccentAqua),
      noiseHold_(processor.parameters, params::kNoiseHold, "Hold", theme::kAccentAqua),
      noiseDecay_(processor.parameters, params::kNoiseDecay, "Decay", theme::kAccentAqua),
      noiseCurve_(processor.parameters, params::kNoiseCurve, "Curve", theme::kAccentAqua),
      noiseHPF_(processor.parameters, params::kNoiseHPF, "High-pass", theme::kAccentAqua),
      noiseLPF_(processor.parameters, params::kNoiseLPF, "Low-pass", theme::kAccentAqua),
      noiseWidth_(processor.parameters, params::kNoiseStereoWidth, "Air width", theme::kAccentAqua),
      decayScale_(processor.parameters, params::kDecayScale, "Decay morph", theme::kAccentAqua),
      noiseLevel_(processor.parameters, params::kNoiseLevel, "Air level", theme::kAccentAqua),
      hissColor_(processor.parameters, params::kHissColor, "Air color", theme::kAccentAqua),
      hissDensity_(processor.parameters, params::kHissDensity, "Density", theme::kAccentAqua),
      hissDrive_(processor.parameters, params::kHissDrive, "Saturation", theme::kAccentAqua),
      bodyLevel_(processor.parameters, params::kBodyLevel, "Body level", theme::kAccentBrass),
      bodyTone_(processor.parameters, params::kBodyTone, "Body tone", theme::kAccentBrass),
      bodyDecay_(processor.parameters, params::kBodyDecay, "Body decay", theme::kAccentBrass),
      preBursts_(processor.parameters, params::kPreBursts, "Strikes", theme::kAccentCoral),
      preSpread_(processor.parameters, params::kPreSpread, "Bounce spread", theme::kAccentCoral),
      preDecay_(processor.parameters, params::kPreDecay, "Bounce decay", theme::kAccentCoral),
      preJitter_(processor.parameters, params::kPreJitter, "Humanize", theme::kAccentCoral),
      flamSpread_(processor.parameters, params::kFlamSpread, "Bounce pan", theme::kAccentCoral),
      keyTracking_(processor.parameters, params::kKeyTracking, "Key tracking", theme::kAccentCoral),
      preLevel_(processor.parameters, params::kPreLevel, "Bounce level", theme::kAccentCoral),
      clickType_(processor.parameters, params::kClickType, "Contact", theme::kAccentCoral),
      clickLevel_(processor.parameters, params::kClickLevel, "Stick level", theme::kAccentCoral),
      clickFreq_(processor.parameters, params::kClickFreq, "Stick tone", theme::kAccentCoral),
      articulationLock_("Articulation", theme::kAccentCoral,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::articulation); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::articulation, on); }),
      airLock_("Air", theme::kAccentAqua,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::air); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::air, on); }),
      holdAttachment_(*processor.parameters.getParameter(params::kAmpEnabled), [this](float value) {
          const bool held = value > .5f;
          const auto accent = held ? theme::kAccentBrass : theme::kAccentAqua;
          playModeBadge_.setText(held ? "Held" : "One-shot", juce::dontSendNotification);
          playModeBadge_.setColour(juce::Label::textColourId, accent);
          playModeBadge_.setColour(juce::Label::outlineColourId, accent.withAlpha(0.6f));
          playModeBadge_.setTooltip(held ? "MIDI notes sustain until note-off. Choose One-shot on Envelope to change it."
                                         : "Each MIDI note plays its full air tail. Choose Hold notes on Envelope to change it.");
          std::ranges::for_each(std::array{ &noiseAttack_, &noiseHold_, &noiseDecay_, &noiseCurve_ }, [held](Knob* knob) {
              knob->setActive(!held, "Held notes ignore this setting. Only Preview hat uses it. "
                                     "Choose One-shot on Envelope to use it.");
          });
      }),
      airLevelDependents_(*processor.parameters.getParameter(params::kNoiseLevel), "Raise Air level to hear this.",
                          { &noiseDecay_, &noiseCurve_ }),
      bodyLevelDependents_(*processor.parameters.getParameter(params::kBodyLevel), "Raise Body level to hear this.",
                           { &bodyTone_, &bodyDecay_ }) {
    noiseType_.setTooltip("Choose the noise model for the air tail, from 909 Crisp to Spectral Wash.");
    noiseDecay_.setTooltip("Set how long the air tail rings after each hit.");
    noiseLPF_.setTooltip("Cut the air above this frequency. Lower values darken the tail.");
    noiseLevel_.setTooltip("Set the level of the air tail.");
    noiseAttack_.setTooltip("Fade the air in over this time at the start of each hit.");
    noiseHold_.setTooltip("Hold the air at full level for this time before it decays.");
    noiseCurve_.setTooltip("Shape the air decay. Negative values drop fast and then trail off. Positive values stay loud longer.");
    noiseHPF_.setTooltip("Cut the air below this frequency. Higher values thin the tail.");
    noiseWidth_.setTooltip("Set the stereo width of the air. 100% keeps the source width, and 0% is mono.");
    decayScale_.setTooltip("Scale the air and body decays together to morph between closed and open hats.");
    hissColor_.setTooltip("Tilt the air tone from warm bronze (-100%) to bright shimmer (+100%).");
    hissDensity_.setTooltip("Control metallic harmonic modulation, unison spread, or comb feedback density.");
    hissDrive_.setTooltip("Add analog saturation to the air and bronze body layers.");
    clickType_.setTooltip("Choose the stick sound: a crisp stick, a pedal foot or a laser tick.");
    preBursts_.setTooltip("Set how many times the stick strikes. The last strike is the main hit.");
    clickFreq_.setTooltip("Set the pitch of the stick click.");
    clickLevel_.setTooltip("Set the level of the stick click on the main hit.");
    preSpread_.setTooltip("Set the time between stick strikes.");
    preDecay_.setTooltip("Set how fast each bounce strike fades.");
    preLevel_.setTooltip("Set the level of the bounce strikes before the main hit.");
    preJitter_.setTooltip("Vary the bounce timing and level a little on each hit.");
    flamSpread_.setTooltip("Spread the bounce strikes left and right.");
    bodyLevel_.setTooltip("Mix the spun-bronze cymbal shell resonators and low-mid chest punch.");
    bodyTone_.setTooltip("Sweep the bronze shell resonant chest modes from dark warm weight to biting bronze.");
    bodyDecay_.setTooltip("Set how long the bronze cymbal shell resonance sustains.");
    keyTracking_.setTooltip(::suite::eq::kKeyTrackingTip);
    playModeBadge_.setFont(juce::FontOptions(14.5f, juce::Font::bold));
    playModeBadge_.setJustificationType(juce::Justification::centred);
    playModeBadge_.setColour(juce::Label::backgroundColourId, theme::kBackground);
    std::ranges::for_each(std::array<juce::Component*, 29>{
             &noiseType_, &noiseAttack_, &noiseHold_, &noiseDecay_, &noiseCurve_, &noiseHPF_, &noiseLPF_,
             &noiseWidth_, &decayScale_, &noiseLevel_, &hissColor_, &hissDensity_, &hissDrive_,
             &bodyLevel_, &bodyTone_, &bodyDecay_, &preBursts_, &preSpread_, &preDecay_, &preJitter_,
             &flamSpread_, &keyTracking_, &preLevel_, &clickType_, &clickLevel_, &clickFreq_,
             &articulationLock_, &airLock_, &playModeBadge_ },
        [this](juce::Component* child) { addAndMakeVisible(child); });
    holdAttachment_.sendInitialUpdate();
    setAdvancedMode(false);
}

void HatSynthPanel::setAdvancedMode(bool advanced) {
    advanced_ = advanced;
    std::ranges::for_each(std::array<juce::Component*, 12>{
             &noiseAttack_, &noiseHold_, &noiseCurve_, &noiseHPF_, &noiseWidth_, &decayScale_,
             &preSpread_, &preDecay_, &preJitter_, &flamSpread_, &keyTracking_, &preLevel_ },
        [advanced](juce::Component* knob) { knob->setVisible(advanced); });
    resized();
}

void HatSynthPanel::paint(juce::Graphics& g) {
    drawSection(g, sectionBounds_[0], "Air tail", theme::kAccentAqua);
    drawSection(g, sectionBounds_[1], "Bronze body & stick", theme::kAccentCoral);
}

void HatSynthPanel::resized() {
    const int width = (getWidth() - 10) / 2;
    std::ranges::for_each(std::views::iota(0, 2), [&](int i) {
        sectionBounds_[static_cast<size_t>(i)] = { i * (width + 10), 0, width, getHeight() };
    });
    auto air = sectionBounds_[0].reduced(11, 9);
    auto stick = sectionBounds_[1].reduced(11, 9);
    std::ranges::for_each(std::array{ std::pair{ &air, &airLock_ }, std::pair{ &stick, &articulationLock_ } },
        [](const auto& pair) {
            pair.second->setBounds(pair.first->removeFromTop(36).removeFromRight(56));
        });
    const auto airLockBounds = airLock_.getBounds();
    playModeBadge_.setBounds(airLockBounds.getX() - 100, airLockBounds.getCentreY() - 14, 92, 28);
    constexpr int kComboWidth = 228;
    const auto airCombo = air.removeFromTop(64);
    noiseType_.setBounds(airCombo.withSizeKeepingCentre(std::min(kComboWidth, airCombo.getWidth()), 64));
    // Contact sits above the stick row it changes.
    const int rowHeight = (stick.getHeight() - 64) / (advanced_ ? 4 : 2);
    layoutRow(stick.removeFromTop(rowHeight), { &bodyLevel_, &bodyTone_, &bodyDecay_ });
    const auto contactCombo = stick.removeFromTop(64);
    clickType_.setBounds(contactCombo.withSizeKeepingCentre(std::min(kComboWidth, contactCombo.getWidth()), 64));

    if (advanced_) {
        layoutRows(air, {
            { &noiseDecay_, &noiseLPF_, &noiseLevel_ },
            { &hissColor_, &hissDensity_, &hissDrive_ },
            { &noiseAttack_, &noiseHold_, &noiseCurve_ },
            { &noiseHPF_, &noiseWidth_, &decayScale_ }
        });
        layoutRows(stick, {
            { &preBursts_, &clickFreq_, &clickLevel_ },
            { &preSpread_, &preDecay_, &preLevel_ },
            { &preJitter_, &flamSpread_, &keyTracking_ }
        });
        return;
    }
    layoutRows(air, {
        { &noiseDecay_, &noiseLPF_, &noiseLevel_ },
        { &hissColor_, &hissDensity_, &hissDrive_ }
    });
    layoutRows(stick, { { &preBursts_, &clickFreq_, &clickLevel_ } });
}

EnvelopePanel::EnvelopePanel(OpenHatterProcessor& processor)
    : processor_(processor),
      attack_(processor.parameters, params::kAmpAttack, "Attack", theme::kAccentBrass),
      decay_(processor.parameters, params::kAmpDecay, "Decay", theme::kAccentBrass),
      sustain_(processor.parameters, params::kAmpSustain, "Sustain", theme::kAccentAqua),
      release_(processor.parameters, params::kAmpRelease, "Release", theme::kAccentCoral),
      envelopeLock_("Envelope", theme::kAccentBrass,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::envelope); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::envelope, on); }),
      gateAttachment_(*processor.parameters.getParameter(params::kAmpEnabled), [this](float value) {
          holdNotes_.setToggleState(value > .5f, juce::dontSendNotification);
          oneShot_.setToggleState(value <= .5f, juce::dontSendNotification);
          auto& release = release_.slider();
          const double oneShotLimit = release.valueToProportionOfLength(core::kOneShotMaxReleaseMs);
          release.getProperties().set(kArcLimitProperty, value > .5f ? 1.0 : oneShotLimit);
          repaint();
      }) {
    oneShot_.setTooltip("Let each hit play its full air tail, even after key up.");
    holdNotes_.setTooltip("Sustain until a matching note-off on the original MIDI channel.");
    oneShot_.getProperties().set("lockParamId", params::kAmpEnabled);
    holdNotes_.getProperties().set("lockParamId", params::kAmpEnabled);
    attack_.setTooltip("Fade each note in over this time.");
    decay_.setTooltip("Set the time from full level down to the sustain level.");
    sustain_.setTooltip("Set the level after the decay. Hold notes keeps this level until note-off.");
    release_.setTooltip("Set the fade after note-off in Hold notes mode. One-shot hits cap it at "
                        + params::formatMs(core::kOneShotMaxReleaseMs) + ".");
    addAndMakeVisible(envelopeLock_);
    std::ranges::for_each(std::array{ &oneShot_, &holdNotes_ }, [this](juce::TextButton* button) {
        button->setToggleable(true);
        button->setColour(juce::TextButton::buttonOnColourId, theme::kAccentAqua);
        addAndMakeVisible(button);
    });
    oneShot_.onClick = [this] { gateAttachment_.setValueAsCompleteGesture(0.0f); };
    holdNotes_.onClick = [this] { gateAttachment_.setValueAsCompleteGesture(1.0f); };
    std::ranges::for_each(std::array{ &attack_, &decay_, &sustain_, &release_ }, [this](Knob* knob) {
        addAndMakeVisible(knob);
        knob->slider().onValueChange = [this] { repaint(); };
    });
    gateAttachment_.sendInitialUpdate();
}

EnvelopePanel::EnvelopeGeometry EnvelopePanel::computeGeometry() const noexcept {
    const auto plot = getLocalBounds().reduced(40, 0).withTrimmedTop(114).withTrimmedBottom(200).toFloat();
    const auto relative = [](const Knob& knob) {
        auto& slider = const_cast<Knob&>(knob).slider();
        return static_cast<float>(slider.valueToProportionOfLength(slider.getValue()));
    };
    const float attackX = plot.getX() + plot.getWidth() * (.04f + .20f * relative(attack_));
    const float decayX = attackX + plot.getWidth() * (.10f + .20f * relative(decay_));
    auto& releaseSlider = const_cast<Knob&>(release_).slider();
    const double releaseVal = holdNotes_.getToggleState()
        ? releaseSlider.getValue()
        : std::min(releaseSlider.getValue(), 200.0);
    const float releaseRel = static_cast<float>(releaseSlider.valueToProportionOfLength(releaseVal));
    const float releaseX = plot.getRight() - plot.getWidth() * (.08f + .20f * releaseRel);
    const float sustainY = plot.getBottom() - plot.getHeight() * relative(sustain_);
    return { plot, attackX, decayX, releaseX, sustainY };
}

EnvelopePanel::DragHandle EnvelopePanel::hitTestHandle(juce::Point<float> pos) const noexcept {
    const auto g = computeGeometry();
    const std::array<std::pair<DragHandle, juce::Point<float>>, 3> handles{{
        { DragHandle::attack, { g.attackX, g.plot.getY() } },
        { DragHandle::decaySustain, { g.decayX, g.sustainY } },
        { DragHandle::releaseSustain, { g.releaseX, g.sustainY } }
    }};
    DragHandle best = DragHandle::none;
    float bestDist = 16.0f;
    std::ranges::for_each(handles, [&](const auto& entry) {
        const float dist = entry.second.getDistanceFrom(pos);
        if (dist <= bestDist) {
            bestDist = dist;
            best = entry.first;
        }
    });
    if (best != DragHandle::none) return best;
    if (pos.x >= g.decayX && pos.x <= g.releaseX && std::abs(pos.y - g.sustainY) <= 10.0f) {
        return DragHandle::sustainSegment;
    }
    return DragHandle::none;
}

void EnvelopePanel::setHandleGesture(DragHandle handle, bool begin) {
    const auto touch = [&](const char* paramId) {
        if (auto* param = processor_.parameters.getParameter(paramId)) {
            if (begin) {
                param->beginChangeGesture();
                return;
            }
            param->endChangeGesture();
        }
    };
    switch (handle) {
        case DragHandle::attack:
            touch(params::kAmpAttack);
            return;
        case DragHandle::decaySustain:
            touch(params::kAmpDecay);
            touch(params::kAmpSustain);
            return;
        case DragHandle::releaseSustain:
            touch(params::kAmpRelease);
            touch(params::kAmpSustain);
            return;
        case DragHandle::sustainSegment:
            touch(params::kAmpSustain);
            return;
        case DragHandle::none:
            return;
    }
}

void EnvelopePanel::updateDrag(juce::Point<float> pos) {
    const auto g = computeGeometry();
    const float w = std::max(1.0f, g.plot.getWidth());
    const float h = std::max(1.0f, g.plot.getHeight());
    const auto setKnobProportion = [](Knob& knob, float prop) {
        const float clamped = std::clamp(prop, 0.0f, 1.0f);
        const double value = knob.slider().proportionOfLengthToValue(static_cast<double>(clamped));
        knob.slider().setValue(value, juce::sendNotificationSync);
    };
    switch (activeHandle_) {
        case DragHandle::attack:
            setKnobProportion(attack_, (pos.x - g.plot.getX() - w * .04f) / (w * .20f));
            return;
        case DragHandle::decaySustain:
            setKnobProportion(decay_, (pos.x - g.attackX - w * .10f) / (w * .20f));
            setKnobProportion(sustain_, (g.plot.getBottom() - pos.y) / h);
            return;
        case DragHandle::releaseSustain:
            setKnobProportion(release_, (g.plot.getRight() - pos.x - w * .08f) / (w * .20f));
            setKnobProportion(sustain_, (g.plot.getBottom() - pos.y) / h);
            return;
        case DragHandle::sustainSegment:
            setKnobProportion(sustain_, (g.plot.getBottom() - pos.y) / h);
            return;
        case DragHandle::none:
            return;
    }
}

void EnvelopePanel::mouseMove(const juce::MouseEvent& event) {
    const auto hit = hitTestHandle(event.position);
    if (hit == hoverHandle_) return;
    hoverHandle_ = hit;
    setMouseCursor(hoverHandle_ == DragHandle::none
        ? juce::MouseCursor::NormalCursor
        : (hoverHandle_ == DragHandle::sustainSegment
            ? juce::MouseCursor::UpDownResizeCursor
            : juce::MouseCursor::DraggingHandCursor));
    repaint();
}

void EnvelopePanel::mouseExit(const juce::MouseEvent&) {
    if (activeHandle_ != DragHandle::none || hoverHandle_ == DragHandle::none) return;
    hoverHandle_ = DragHandle::none;
    setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void EnvelopePanel::mouseDown(const juce::MouseEvent& event) {
    if (event.mods.isPopupMenu()) return;
    activeHandle_ = hitTestHandle(event.position);
    if (activeHandle_ == DragHandle::none) return;
    setHandleGesture(activeHandle_, true);
    updateDrag(event.position);
}

void EnvelopePanel::mouseDrag(const juce::MouseEvent& event) {
    if (activeHandle_ == DragHandle::none) return;
    updateDrag(event.position);
}

void EnvelopePanel::mouseUp(const juce::MouseEvent& event) {
    if (activeHandle_ == DragHandle::none) return;
    setHandleGesture(activeHandle_, false);
    activeHandle_ = DragHandle::none;
    hoverHandle_ = hitTestHandle(event.position);
    setMouseCursor(hoverHandle_ == DragHandle::none
        ? juce::MouseCursor::NormalCursor
        : juce::MouseCursor::DraggingHandCursor);
    repaint();
}

void EnvelopePanel::paint(juce::Graphics& g) {
    drawSection(g, getLocalBounds(), "Amplitude envelope", theme::kAccentBrass);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(16.0f));
    const bool held = holdNotes_.getToggleState();
    g.drawText(held ? "Sustain the air until the matching MIDI note ends. Drag any point to shape ADSR."
                    : "Play each strike to the end of its air tail. Drag any point to shape ADSR.",
               22, 53, getWidth() - 290, 33, juce::Justification::left);
    const auto geom = computeGeometry();
    const auto& plot = geom.plot;
    const float base = plot.getBottom();
    const float top = plot.getY();
    g.setColour(theme::kPanelEdge.withAlpha(.5f));
    std::ranges::for_each(std::views::iota(0, 5), [&](int i) {
        g.drawHorizontalLine(juce::roundToInt(top + plot.getHeight() * static_cast<float>(i) / 4), plot.getX(), plot.getRight());
    });
    juce::Path line;
    line.startNewSubPath(plot.getX(), base);
    line.lineTo(geom.attackX, top);
    line.lineTo(geom.decayX, geom.sustainY);
    line.lineTo(geom.releaseX, geom.sustainY);
    line.lineTo(plot.getRight(), base);
    auto fill = line;
    fill.closeSubPath();
    g.setGradientFill(juce::ColourGradient(theme::kAccentAqua.withAlpha(.25f), 0, top,
                                            theme::kAccentAqua.withAlpha(.015f), 0, base, false));
    g.fillPath(fill);
    g.setColour(theme::kAccentAqua);
    g.strokePath(line, juce::PathStrokeType(2.6f));
    const std::array<std::tuple<DragHandle, juce::Point<float>, juce::Colour>, 3> points{{
        { DragHandle::attack, { geom.attackX, top }, theme::kAccentBrass },
        { DragHandle::decaySustain, { geom.decayX, geom.sustainY }, theme::kAccentAqua },
        { DragHandle::releaseSustain, { geom.releaseX, geom.sustainY }, theme::kAccentCoral }
    }};
    std::ranges::for_each(points, [&](const auto& item) {
        const auto& [handle, point, colour] = item;
        const bool hot = activeHandle_ == handle || hoverHandle_ == handle;
        g.setColour(colour.withAlpha(hot ? 0.35f : 0.16f));
        const float haloSize = hot ? 22.0f : 16.0f;
        g.fillEllipse(juce::Rectangle<float>(haloSize, haloSize).withCentre(point));
        g.setColour(colour);
        g.drawEllipse(juce::Rectangle<float>(12.0f, 12.0f).withCentre(point), hot ? 2.2f : 1.5f);
        g.setColour(theme::kText);
        g.fillEllipse(juce::Rectangle<float>(hot ? 8.0f : 6.5f, hot ? 8.0f : 6.5f).withCentre(point));
    });
    g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
    g.setColour(theme::kText.withAlpha(0.9f));
    g.drawText(held ? "Key down" : "Strike", juce::Rectangle<float>(plot.getX(), base + 8, 120, 22), juce::Justification::left);
    g.drawText(held ? "Key up" : "Tail fade", juce::Rectangle<float>(geom.releaseX - 55, base + 8, 110, 22), juce::Justification::centred);
}

void EnvelopePanel::resized() {
    envelopeLock_.setBounds(getWidth() - 67, 7, 56, 36);
    oneShot_.setBounds(getWidth() - 252, 46, 110, 38);
    holdNotes_.setBounds(getWidth() - 138, 46, 114, 38);
    layoutRow(getLocalBounds().reduced(24, 0).removeFromBottom(161), { &attack_, &decay_, &sustain_, &release_ });
}

LearnControl::LearnControl(juce::RangedAudioParameter& target, std::atomic<uint32_t>& eventCount,
                           std::atomic<int>& lastValue, const juce::String& tooltip)
    : target_(target, [](float) {}), eventCount_(eventCount), lastValue_(lastValue) {
    setWantsKeyboardFocus(true);
    button_.setTitle("Learn " + target.getName(64));
    button_.setTooltip(tooltip);
    button_.setToggleable(true);
    button_.setColour(juce::TextButton::buttonOnColourId, theme::kAccentCoral);
    button_.onClick = [this] {
        if (isTimerRunning()) {
            stopListening();
            return;
        }
        armedCount_ = eventCount_.load();
        ticksLeft_ = kLearnTimeoutTicks;
        button_.setToggleState(true, juce::dontSendNotification);
        button_.setButtonText("Listening");
        grabKeyboardFocus();
        startTimerHz(kLearnPollHz);
    };
    addAndMakeVisible(button_);
}

void LearnControl::resized() {
    button_.setBounds(getLocalBounds().withSizeKeepingCentre(std::min(130, getWidth() - 8), 38));
}

bool LearnControl::keyPressed(const juce::KeyPress& key) {
    if (key != juce::KeyPress::escapeKey)
        return false;
    return cancelIfListening();
}

bool LearnControl::cancelIfListening() {
    if (!isTimerRunning())
        return false;
    stopListening();
    return true;
}

void LearnControl::timerCallback() {
    if (eventCount_.load() != armedCount_) {
        target_.setValueAsCompleteGesture(static_cast<float>(lastValue_.load()));
        stopListening();
        return;
    }
    if (--ticksLeft_ <= 0)
        stopListening();
}

void LearnControl::stopListening() {
    stopTimer();
    button_.setToggleState(false, juce::dontSendNotification);
    button_.setButtonText("Learn");
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
      rollAccent_(processor.parameters, params::kRollAccent, "Accent", theme::kAccentBrass),
      chokeNoteLearn_(*processor.parameters.getParameter(params::kMidiChokeNote), processor.displayNoteCount,
                      processor.displayMidiNoteNumber,
                      "Click, then play a note to set the choke note. Click again or press Escape to cancel."),
      chokeCcLearn_(*processor.parameters.getParameter(params::kMidiChokeCc), processor.displayControllerCount,
                    processor.displayLastController,
                    "Click, then move a hardware control to set the choke CC. Click again or press Escape to cancel."),
      rollDependents_(*processor.parameters.getParameter(params::kRollEnabled), "Turn Roll on to use this.",
                      { &rollDivision_, &rollSwing_, &rollAccent_ }),
      reverbMixDependents_(*processor.parameters.getParameter(params::kReverbMix),
                           "Raise the reverb Mix on Effects to hear this.", { &velocityReverb_ }) {
    std::ranges::for_each(std::array<juce::Component*, 11>{
             &velocityAir_, &velocityChick_, &velocityReverb_, &chokeNote_, &chokeCc_,
             &rollEnabled_, &rollDivision_, &rollSwing_, &rollAccent_, &chokeNoteLearn_, &chokeCcLearn_},
        [this](juce::Component* component) { addAndMakeVisible(component); });

    velocityAir_.setTooltip(juce::String("Shape the air level against velocity. ") + kVelocityCurveHint);
    velocityChick_.setTooltip(juce::String("Shape the stick level against velocity. ") + kVelocityCurveHint);
    velocityReverb_.setTooltip(juce::String("Shape the reverb send against velocity. ") + kVelocityCurveHint);
    const std::array knobs{ &velocityAir_, &velocityChick_, &velocityReverb_ };
    std::ranges::for_each(std::views::iota(size_t{0}, knobs.size()), [&](size_t i) {
        knobs[i]->slider().onValueChange = [this, i] { repaint(curveBounds_[i]); };
    });
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
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(15.0f));
    g.drawText(juce::String("Each curve plots level against velocity. ") + kVelocityCurveHint,
               velocityBounds_.getX() + 22, velocityBounds_.getY() + 40, velocityBounds_.getWidth() - 44, 22,
               juce::Justification::left);

    const std::array knobs{ &velocityAir_, &velocityChick_, &velocityReverb_ };
    std::ranges::for_each(std::views::iota(size_t{0}, knobs.size()), [&](size_t i) {
        const auto plot = curveBounds_[i].toFloat();
        g.setColour(theme::kBackground);
        g.fillRoundedRectangle(plot, 6.0f);
        g.setColour(theme::kPanelEdge);
        g.drawRoundedRectangle(plot, 6.0f, 1.0f);
        const auto inner = plot.reduced(10.0f);
        g.drawLine({ inner.getBottomLeft(), inner.getTopRight() }, 1.0f);
        const float exponent = static_cast<float>(knobs[i]->slider().getValue());
        juce::Path curve;
        curve.startNewSubPath(inner.getBottomLeft());
        std::ranges::for_each(std::views::iota(1, 41), [&](int step) {
            const float velocity = static_cast<float>(step) / 40.0f;
            curve.lineTo(inner.getX() + inner.getWidth() * velocity,
                         inner.getBottom() - inner.getHeight() * std::pow(velocity, exponent));
        });
        g.setColour(knobs[i]->slider().findColour(juce::Slider::rotarySliderFillColourId));
        g.strokePath(curve, juce::PathStrokeType(2.4f));
    });
}

void PerformancePanel::resized() {
    auto area = getLocalBounds();
    velocityBounds_ = area.removeFromTop(410);
    rollBounds_ = area.withTrimmedTop(10);
    auto velocity = velocityBounds_.reduced(10, 9).withTrimmedTop(64);
    layoutRow(velocity.removeFromBottom(150), { &velocityAir_, &velocityChick_, &velocityReverb_ });
    const int column = velocity.getWidth() / 3;
    std::ranges::for_each(curveBounds_, [&](juce::Rectangle<int>& plot) {
        plot = velocity.removeFromLeft(column).withSizeKeepingCentre(std::min(column - 40, 240), velocity.getHeight() - 16);
    });
    auto roll = rollBounds_.reduced(11, 9).withTrimmedTop(34);
    auto modes = roll.removeFromLeft(144);
    layoutRows(modes, { { &rollEnabled_ }, { &rollDivision_ } });
    // Reserve the Learn strip across the entire 4-knob row so all dials share one baseline.
    auto learnStrip = roll.removeFromBottom(46);
    layoutRow(roll, { &rollSwing_, &rollAccent_, &chokeNote_, &chokeCc_ });
    chokeNoteLearn_.setBounds(chokeNote_.getX(), learnStrip.getY(), chokeNote_.getWidth(), learnStrip.getHeight());
    chokeCcLearn_.setBounds(chokeCc_.getX(), learnStrip.getY(), chokeCc_.getWidth(), learnStrip.getHeight());
}

bool PerformancePanel::cancelActiveLearn() {
    const bool noteCancelled = chokeNoteLearn_.cancelIfListening();
    const bool ccCancelled = chokeCcLearn_.cancelIfListening();
    return noteCancelled || ccCancelled;
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
      reverbGate_(processor.parameters, params::kReverbGate, "Reverb gate", theme::kAccentAqua),
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
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::spaceAndOutput, on); }),
      reverseModeDependents_(*processor.parameters.getParameter(params::kReverseMode),
                             "Choose a Reverse sync mode to use this.", { &reverseMix_ }),
      reverbMixDependents_(*processor.parameters.getParameter(params::kReverbMix), "Raise the reverb Mix to hear this.",
                           { &reverbSize_, &reverbDecay_, &reverbDamp_, &reverbGate_ }),
      exciterMixDependents_(*processor.parameters.getParameter(params::kDistMix), "Raise the Exciter Mix to hear this.",
                            { &distType_, &distDrive_, &distTone_ }),
      compMixDependents_(*processor.parameters.getParameter(params::kCompMix), "Raise the Dynamics Mix to hear this.",
                         { &compThresh_, &compRatio_, &compAttack_, &compRelease_, &compMakeup_ }) {
    echoMix_.setTooltip("Blend a tempo-synced dotted-sixteenth stereo ping-pong echo.");
    distType_.setTooltip("Choose the exciter's saturation curve: tape, tube, hard clip or crystal.");
    distDrive_.setTooltip("Set how hard the exciter drives the signal.");
    distTone_.setTooltip("Tilt the exciter tone. Negative values darken it. Positive values add high sheen.");
    distMix_.setTooltip("Blend the exciter with the dry signal.");
    compThresh_.setTooltip("Set the level where the compressor starts to reduce gain.");
    compRatio_.setTooltip("Set how much the compressor reduces gain above the threshold.");
    compAttack_.setTooltip("Set how fast the compressor reacts to a peak.");
    compRelease_.setTooltip("Set how fast the compressor lets go after a peak.");
    compMakeup_.setTooltip("Add gain after the compressor.");
    compMix_.setTooltip("Blend the compressed signal with the dry signal for parallel compression.");
    reverbSize_.setTooltip("Set the size of the reverb room.");
    reverbDecay_.setTooltip("Set how long the reverb tail rings.");
    reverbDamp_.setTooltip("Darken the reverb tail. Higher values remove more highs.");
    reverbGate_.setTooltip("Close the reverb this long after each hit. Off lets the reverb ring out.");
    reverbMix_.setTooltip("Set the level of the reverb return.");
    reverseMode_.setTooltip("Add a reversed copy of each hit that ends on the next beat, half beat or quarter beat. "
                            "Beat 4 Auto reverses only hits on the last beat of a bar.");
    reverseMix_.setTooltip("Set the level of the reversed copy.");
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

    const auto twoRowArea = [](juce::Rectangle<int> card) {
        auto inner = card.reduced(8, 10).withTrimmedTop(36);
        return inner.withHeight(std::min(inner.getHeight(), 320));
    };
    layoutRows(twoRowArea(sectionBounds_[0]), {
        { &distType_, &distDrive_ },
        { &distTone_, &distMix_ }
    });
    layoutRows(twoRowArea(sectionBounds_[1]), {
        { &compThresh_, &compRatio_, &compAttack_ },
        { &compRelease_, &compMakeup_, &compMix_ }
    });
    layoutRows(sectionBounds_[2].reduced(7, 10).withTrimmedTop(36), {
        { &reverbSize_, &reverbDecay_, &reverbDamp_ },
        { &reverbGate_, &reverbMix_, &echoMix_ },
        { &reverseMode_, &reverseMix_ }
    });
    reverseMode_.setBounds(reverseMode_.getX(), reverseMix_.getBottom() - 61, reverseMode_.getWidth(), 64);
}

PeakMeter::PeakMeter(std::atomic<float>& peak) : peak_(peak) {
    // Drop the peak held while the editor was closed.
    peak_.store(0.0f);
    setTooltip("Output peak in dBFS after Output gain. The light latches above 0 dBFS. Click the meter to clear it.");
    startTimerHz(30);
}

void PeakMeter::timerCallback() {
    pushPeak(peak_.exchange(0.0f), juce::Time::getMillisecondCounterHiRes());
}

void PeakMeter::pushPeak(float linearPeak, double nowMs) {
    constexpr double kHoldMs = 1000.0;
    constexpr float kFallDbPerSecond = 24.0f;
    const float levelBefore = levelDb_;
    const float heldBefore = heldDb_;
    const bool clippedBefore = clipped_;
    const float db = juce::Decibels::gainToDecibels(linearPeak, kFloorDb);
    const float fallDb = static_cast<float>(nowMs - lastMs_) * 0.001f * kFallDbPerSecond;
    lastMs_ = nowMs;
    levelDb_ = std::max(db, levelDb_ - fallDb);
    if (db >= heldDb_ || nowMs - heldAtMs_ >= kHoldMs) {
        heldDb_ = db;
        heldAtMs_ = nowMs;
    }
    clipped_ = clipped_ || linearPeak > 1.0f;
    if (!juce::approximatelyEqual(levelDb_, levelBefore) || !juce::approximatelyEqual(heldDb_, heldBefore)
        || clipped_ != clippedBefore)
        repaint();
}

void PeakMeter::mouseDown(const juce::MouseEvent&) {
    clipped_ = false;
    repaint();
}

void PeakMeter::paint(juce::Graphics& g) {
    constexpr float kTopDb = 6.0f;
    auto area = getLocalBounds();
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawText("Peak", area.removeFromTop(19), juce::Justification::centred);
    const auto readout = area.removeFromBottom(20).withSizeKeepingCentre(std::min(getWidth() - 4, 68), 20);
    auto barZone = area;
    const auto light = barZone.removeFromTop(8).toFloat();
    barZone.removeFromTop(4);
    const float barX = std::max(6.0f, static_cast<float>(getWidth()) * 0.5f - 21.0f);
    const juce::Rectangle<float> bar(barX, static_cast<float>(barZone.getY()), 12.0f,
                                     static_cast<float>(std::max(0, barZone.getHeight() - 4)));
    const auto yFor = [&](float db) {
        return juce::jmap(juce::jlimit(kFloorDb, kTopDb, db), kFloorDb, kTopDb, bar.getBottom(), bar.getY());
    };

    g.setColour(theme::kBackground);
    g.fillRoundedRectangle(bar, 3.0f);
    g.setColour(theme::kAccentAqua);
    g.fillRoundedRectangle(bar.withTop(yFor(levelDb_)), 3.0f);
    g.setColour(theme::kTextDim);
    g.fillRect(bar.withY(yFor(0.0f)).withHeight(1.0f).expanded(2.0f, 0.0f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    const float tickX = bar.getRight() + 2.0f;
    const float labelX = tickX + 7.0f;
    const float labelW = std::max(0.0f, static_cast<float>(getWidth()) - labelX - 2.0f);
    for (const auto& [tickDb, tickText] : { std::pair{ 0.0f, "0" }, std::pair{ -12.0f, "-12" }, std::pair{ -24.0f, "-24" } }) {
        const float y = yFor(tickDb);
        g.fillRect(tickX, y - 0.5f, 3.5f, 1.0f);
        g.drawText(tickText, juce::Rectangle<float>(labelX, y - 6.5f, labelW, 13.0f), juce::Justification::left, false);
    }
    if (heldDb_ > kFloorDb) {
        g.setColour(theme::kText);
        g.fillRect(bar.withY(yFor(heldDb_) - 1.0f).withHeight(2.0f));
    }
    g.setColour(clipped_ ? theme::kAccentBrass : theme::kPanelEdge);
    g.fillRoundedRectangle(light.withSizeKeepingCentre(16.0f, 6.0f).withX(bar.getCentreX() - 8.0f), 2.0f);

    auto text = params::formatDb(heldDb_);
    if (heldDb_ <= kFloorDb) text = "-inf dB";
    g.setColour(theme::kBackground);
    g.fillRect(readout);
    g.setColour(theme::kPanelEdge);
    g.drawRect(readout);
    g.setColour(clipped_ ? theme::kAccentBrass : theme::kText);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText(text, readout, juce::Justification::centred);
}

OutputPanel::OutputPanel(OpenHatterProcessor& processor)
    : eqLow_(processor.parameters, params::kEqLowFreq, params::kEqLowDb, "Low EQ", theme::kAccentAqua, theme::kAccentBrass),
      eqMid_(processor.parameters, params::kEqMidFreq, params::kEqMidDb, "Mid EQ", theme::kAccentAqua, theme::kAccentBrass),
      eqHigh_(processor.parameters, params::kEqHighFreq, params::kEqHighDb, "High EQ", theme::kAccentAqua, theme::kAccentBrass),
      masterWidth_(processor.parameters, params::kStereoWidth, "Width", theme::kAccentAqua),
      outputGain_(processor.parameters, params::kOutputGain, "Output gain", theme::kAccentAqua),
      limiterCeiling_(processor.parameters, params::kLimiterCeiling, "Limiter ceiling", theme::kAccentBrass),
      limiterRelease_(processor.parameters, params::kLimiterRelease, "Limiter release", theme::kAccentBrass),
      limiterEnabled_(processor.parameters, params::kLimiterEnabled, "Limiter off", "Limiter on", theme::kAccentBrass),
      peakMeter_(processor.displayPeak),
      outputLock_("Output", theme::kAccentAqua,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::output); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::output, on); }),
      limiterDependents_(*processor.parameters.getParameter(params::kLimiterEnabled), "Turn the limiter on to use this.",
                         { &limiterCeiling_, &limiterRelease_ }) {
    eqLow_.setTooltip(::suite::eq::kLowTip);
    eqMid_.setTooltip(::suite::eq::kMidTip);
    eqHigh_.setTooltip(::suite::eq::kHighTip);
    limiterEnabled_.setTooltip("Switch output peak limiting on or off.");
    masterWidth_.setTooltip("Set the stereo width of the output. 100% keeps it unchanged, and 0% is mono.");
    limiterRelease_.setTooltip("Set how fast the limiter recovers after a peak.");
    limiterCeiling_.setTooltip("Set the highest peak the limiter lets through. Output gain comes after the limiter, "
                               "so a boost raises peaks above this ceiling.");
    outputGain_.setTooltip("Set the level after the limiter. A boost raises peaks above the limiter ceiling. "
                           "The peak meter shows the result.");
    std::ranges::for_each(std::array<juce::Component*, 10>{
             &eqLow_, &eqMid_, &eqHigh_, &masterWidth_, &outputGain_,
             &limiterCeiling_, &limiterRelease_, &limiterEnabled_, &peakMeter_, &outputLock_},
        [this](juce::Component* component) { addAndMakeVisible(component); });
}

void OutputPanel::paint(juce::Graphics& g) {
    drawSection(g, getLocalBounds(), "Output", theme::kAccentAqua);
}

void OutputPanel::resized() {
    outputLock_.setBounds(getWidth() - 67, 7, 56, 36);
    auto row = getLocalBounds().reduced(10, 6).withTrimmedTop(36);
    // The row follows the signal order.
    const std::array<std::pair<juce::Component*, float>, 9> items{{
        { &masterWidth_, 8.8f }, { &eqLow_, 15.5f }, { &eqMid_, 15.5f }, { &eqHigh_, 15.5f },
        { &limiterEnabled_, 9.0f }, { &limiterCeiling_, 8.8f }, { &limiterRelease_, 8.8f },
        { &outputGain_, 8.8f }, { &peakMeter_, 8.0f }
    }};
    const float total = std::accumulate(items.begin(), items.end(), 0.0f,
                                        [](float sum, const auto& item) { return sum + item.second; });
    const int gap = 4;
    const int avail = std::max(0, row.getWidth() - gap * static_cast<int>(items.size() - 1));
    std::ranges::for_each(items, [&](const auto& item) {
        const int w = juce::roundToInt(static_cast<float>(avail) * (item.second / total));
        item.first->setBounds(row.removeFromLeft(w));
        row.removeFromLeft(gap);
    });
}

} // namespace openhatter::ui
