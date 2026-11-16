#include "Panels.h"
#include "Parameters.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <algorithm>

namespace openhatter::ui {
namespace {

void drawSection(juce::Graphics& g, juce::Rectangle<int> bounds,
                 const juce::String& title, juce::Colour accent) {
    auto area = bounds.toFloat().reduced(0.5f);
    g.setColour(theme::kPanel);
    g.fillRoundedRectangle(area, 7.0f);
    g.setColour(theme::kPanelEdge);
    g.drawRoundedRectangle(area, 7.0f, 1.0f);
    g.setColour(accent.withAlpha(0.8f));
    g.fillRoundedRectangle(area.removeFromTop(2.0f).reduced(9.0f, 0.0f), 1.0f);
    g.setColour(accent);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(title, bounds.reduced(9, 0).withY(bounds.getY() + 6).withHeight(18),
               juce::Justification::centredLeft, true);
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

void drawMetric(juce::Graphics& g, juce::Rectangle<int> bounds,
                const juce::String& label, const juce::String& value,
                const juce::String& unit, juce::Colour accent) {
    g.setColour(theme::kPanel);
    g.fillRoundedRectangle(bounds.toFloat(), 7.0f);
    g.setColour(theme::kPanelEdge);
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 7.0f, 1.0f);
    auto area = bounds.reduced(12, 8);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(label, area.removeFromTop(16), juce::Justification::left, true);
    g.setColour(accent);
    g.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    g.drawText(value, area.removeFromTop(38), juce::Justification::left, true);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText(unit, area.removeFromTop(15), juce::Justification::left, true);
}

} // namespace

MidiMonitorPanel::MidiMonitorPanel(OpenHatterProcessor& processor) : processor_(processor) {
    startTimerHz(20);
}

MidiMonitorPanel::~MidiMonitorPanel() {
    stopTimer();
}

void MidiMonitorPanel::timerCallback() {
    const int note = processor_.displayMidiNoteNumber.load(std::memory_order_relaxed);
    const float velocity = processor_.displayVelocity.load(std::memory_order_relaxed);
    const bool received = processor_.displayMidiNoteReceived.load(std::memory_order_relaxed);
    const int displayed = received ? note : -1;
    if (displayed != lastNote_ || std::abs(velocity - lastVelocity_) > 0.005f) {
        lastNote_ = displayed;
        lastVelocity_ = velocity;
        repaint();
    }
}

void MidiMonitorPanel::paint(juce::Graphics& g) {
    drawPanel(g, getLocalBounds(), "MIDI IN  /  DIRECT PITCH", theme::kAccentAqua);
    auto area = getLocalBounds().reduced(12, 30);
    if (area.isEmpty()) return;

    const bool received = processor_.displayMidiNoteReceived.load(std::memory_order_relaxed);
    const int note = std::clamp(processor_.displayMidiNoteNumber.load(std::memory_order_relaxed), 0, 127);
    const float frequency = processor_.displayMidiFrequency.load(std::memory_order_relaxed);
    const float velocity = std::clamp(processor_.displayVelocity.load(std::memory_order_relaxed), 0.0f, 1.0f);
    const auto noteNames = std::array<const char*, 12>{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const juce::String noteLabel = received
        ? juce::String(noteNames[static_cast<size_t>(note % 12)]) + juce::String(note / 12 - 1)
        : juce::String("Waiting for MIDI");
    const int leadWidth = std::min(340, area.getWidth() / 3);
    auto lead = area.removeFromLeft(leadWidth);
    area.removeFromLeft(10);
    const int metricWidth = (area.getWidth() - 20) / 3;
    auto frequencyArea = area.removeFromLeft(metricWidth);
    area.removeFromLeft(10);
    auto midiArea = area.removeFromLeft(metricWidth);
    area.removeFromLeft(10);

    g.setColour(theme::kPanel);
    g.fillRoundedRectangle(lead.toFloat(), 7.0f);
    g.setColour(theme::kPanelEdge);
    g.drawRoundedRectangle(lead.toFloat().reduced(0.5f), 7.0f, 1.0f);
    auto noteArea = lead.reduced(16, 10);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText(received ? "LAST NOTE" : "NO NOTE RECEIVED", noteArea.removeFromTop(16), juce::Justification::left, true);
    g.setColour(received ? theme::kAccentBrass : theme::kTextDim);
    g.setFont(juce::FontOptions(received ? 39.0f : 22.0f, juce::Font::bold));
    g.drawText(noteLabel, noteArea.removeFromTop(received ? 48 : 36), juce::Justification::left, true);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(received ? "Pitch follows every incoming note-on" : "Send MIDI to track pitch and octave",
               noteArea, juce::Justification::left, true);

    drawMetric(g, frequencyArea, "FREQUENCY", received ? juce::String(frequency, 2) : "--", "Hz", theme::kAccentAqua);
    drawMetric(g, midiArea, "MIDI NOTE", received ? juce::String(note) : "--", "0-127", theme::kAccentBrass);
    drawMetric(g, area, "VELOCITY", received ? juce::String(juce::roundToInt(velocity * 127.0f)) : "--",
               received ? juce::String(juce::roundToInt(velocity * 100.0f)) + "%" : "0-127",
               theme::kAccentCoral);
}

HatSynthPanel::HatSynthPanel(OpenHatterProcessor& processor)
    : toneMode_(processor.parameters, params::kToneMode, "MODE", theme::kAccentBrass),
      toneFreq_(processor.parameters, params::kToneFreq, "BELL", theme::kAccentBrass),
      toneQ_(processor.parameters, params::kToneQ, "RESONANCE", theme::kAccentBrass),
      toneDecay_(processor.parameters, params::kToneDecay, "DECAY", theme::kAccentBrass),
      tonePitchEnv_(processor.parameters, params::kTonePitchEnv, "SWEEP", theme::kAccentBrass),
      tonePitchDecay_(processor.parameters, params::kTonePitchDecay, "SWEEP TIME", theme::kAccentBrass),
      metalSpread_(processor.parameters, params::kMetalSpread, "SPREAD", theme::kAccentBrass),
      metalDampingSlope_(processor.parameters, params::kMetalDampingSlope, "DAMP SLOPE", theme::kAccentBrass),
      metalStereoSpread_(processor.parameters, params::kMetalStereoSpread, "STEREO", theme::kAccentBrass),
      toneLevel_(processor.parameters, params::kToneLevel, "LEVEL", theme::kAccentBrass),
      noiseType_(processor.parameters, params::kNoiseType, "TEXTURE", theme::kAccentAqua),
      noiseAttack_(processor.parameters, params::kNoiseAttack, "ATTACK", theme::kAccentAqua),
      noiseHold_(processor.parameters, params::kNoiseHold, "HOLD", theme::kAccentAqua),
      noiseDecay_(processor.parameters, params::kNoiseDecay, "DECAY", theme::kAccentAqua),
      noiseCurve_(processor.parameters, params::kNoiseCurve, "CURVE", theme::kAccentAqua),
      noiseHPF_(processor.parameters, params::kNoiseHPF, "HIGH PASS", theme::kAccentAqua),
      noiseLPF_(processor.parameters, params::kNoiseLPF, "LOW PASS", theme::kAccentAqua),
      noiseWidth_(processor.parameters, params::kNoiseStereoWidth, "WIDTH", theme::kAccentAqua),
      noiseLevel_(processor.parameters, params::kNoiseLevel, "LEVEL", theme::kAccentAqua),
      preBursts_(processor.parameters, params::kPreBursts, "BOUNCES", theme::kAccentCoral),
      preSpread_(processor.parameters, params::kPreSpread, "SPREAD", theme::kAccentCoral),
      preDecay_(processor.parameters, params::kPreDecay, "DECAY", theme::kAccentCoral),
      preJitter_(processor.parameters, params::kPreJitter, "HUMANIZE", theme::kAccentCoral),
      flamSpread_(processor.parameters, params::kFlamSpread, "PAN", theme::kAccentCoral),
      preLevel_(processor.parameters, params::kPreLevel, "LEVEL", theme::kAccentCoral),
      fullAdsr_(processor.parameters, params::kAmpEnabled, "FULL ADSR", theme::kAccentBrass),
      ampAttack_(processor.parameters, params::kAmpAttack, "ATTACK", theme::kAccentBrass),
      ampDecay_(processor.parameters, params::kAmpDecay, "DECAY", theme::kAccentBrass),
      ampSustain_(processor.parameters, params::kAmpSustain, "SUSTAIN", theme::kAccentBrass),
      ampRelease_(processor.parameters, params::kAmpRelease, "RELEASE", theme::kAccentBrass),
      clickType_(processor.parameters, params::kClickType, "CHICK TYPE", theme::kAccentBrass),
      clickLevel_(processor.parameters, params::kClickLevel, "CHICK LEVEL", theme::kAccentBrass),
      clickFreq_(processor.parameters, params::kClickFreq, "CHICK FREQ", theme::kAccentBrass),
      articulationLock_("Articulation", theme::kAccentCoral,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::articulation); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::articulation, on); }),
      cymbalLock_("Metal", theme::kAccentBrass,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::cymbalAndTuning); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::cymbalAndTuning, on); }),
      airLock_("Air", theme::kAccentAqua,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::air); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::air, on); }) {
    fullAdsr_.setTooltip("Hold MIDI notes until a matching note-off. Release extends to 20 seconds.");
    metalSpread_.setTooltip("Move upper metal partials outward or inward by up to two octaves.");
    metalDampingSlope_.setTooltip("Adjust upper-mode decay relative to the lowest resonator.");
    metalStereoSpread_.setTooltip("Scale stereo dispersion across the upper metal partials.");
    for (juce::Component* component : std::array<juce::Component*, 35>{
             &toneMode_, &toneFreq_, &toneQ_, &toneDecay_, &tonePitchEnv_, &tonePitchDecay_,
             &metalSpread_, &metalDampingSlope_, &metalStereoSpread_, &toneLevel_,
             &noiseType_, &noiseAttack_, &noiseHold_, &noiseDecay_, &noiseCurve_, &noiseHPF_, &noiseLPF_, &noiseWidth_, &noiseLevel_,
             &preBursts_, &preSpread_, &preDecay_, &preJitter_, &flamSpread_, &preLevel_,
             &fullAdsr_, &ampAttack_, &ampDecay_, &ampSustain_, &ampRelease_, &clickType_, &clickLevel_, &clickFreq_, &articulationLock_, &cymbalLock_})
        addAndMakeVisible(component);
    addAndMakeVisible(airLock_);
}

void HatSynthPanel::paint(juce::Graphics& g) {
    drawPanel(g, getLocalBounds(), "SOUND DESIGN", theme::kAccentBrass);
    drawSection(g, sectionBounds_[0], "METAL RESONATOR", theme::kAccentBrass);
    drawSection(g, sectionBounds_[1], "AIR TAIL", theme::kAccentAqua);
    drawSection(g, sectionBounds_[2], "BOUNCE / GROOVE", theme::kAccentCoral);
    drawSection(g, sectionBounds_[3], "ENVELOPE / CHICK", theme::kAccentBrass);
}

void HatSynthPanel::resized() {
    auto area = getLocalBounds().reduced(10, 29);
    constexpr int gap = 7;
    const int width = (area.getWidth() - gap * 3) / 4;
    for (size_t i = 0; i < sectionBounds_.size(); ++i) {
        sectionBounds_[i] = { area.getX() + static_cast<int>(i) * (width + gap), area.getY(), width, area.getHeight() };
    }

    cymbalLock_.setBounds(sectionBounds_[0].getRight() - 68, sectionBounds_[0].getY() + 5, 58, 18);
    airLock_.setBounds(sectionBounds_[1].getRight() - 68, sectionBounds_[1].getY() + 5, 58, 18);
    articulationLock_.setBounds(sectionBounds_[2].getRight() - 68, sectionBounds_[2].getY() + 5, 58, 18);

    auto metal = sectionBounds_[0].reduced(6, 27);
    layoutRows(metal, {
        { &toneMode_, &toneFreq_, nullptr },
        { &toneQ_, &toneDecay_, &toneLevel_ },
        { &tonePitchEnv_, &tonePitchDecay_, nullptr },
        { &metalSpread_, &metalDampingSlope_, &metalStereoSpread_ }
    });

    auto air = sectionBounds_[1].reduced(6, 27);
    layoutRows(air, {
        { &noiseType_, &noiseAttack_, &noiseHold_ },
        { &noiseDecay_, &noiseCurve_, &noiseHPF_ },
        { &noiseLPF_, &noiseWidth_, &noiseLevel_ }
    });

    auto articulation = sectionBounds_[2].reduced(6, 27);
    layoutRows(articulation, {
        { &preBursts_, &preSpread_ },
        { &preDecay_, &preJitter_ },
        { &flamSpread_, &preLevel_ }
    });

    auto envelope = sectionBounds_[3].reduced(5, 27);
    layoutRows(envelope, {
        { &fullAdsr_ },
        { &ampAttack_, &ampDecay_ },
        { &ampSustain_, &ampRelease_ },
        { &clickType_, &clickLevel_ },
        { &clickFreq_, nullptr }
    });
}

PerformancePanel::PerformancePanel(OpenHatterProcessor& processor)
    : velocityMetal_(processor.parameters, params::kVelocityMetal, "METAL VEL", theme::kAccentBrass),
      velocityAir_(processor.parameters, params::kVelocityAir, "AIR VEL", theme::kAccentAqua),
      velocityChick_(processor.parameters, params::kVelocityChick, "CHICK VEL", theme::kAccentCoral),
      velocityReverb_(processor.parameters, params::kVelocityReverb, "REVERB VEL", theme::kAccentAqua),
      chokeNote_(processor.parameters, params::kMidiChokeNote, "CHOKE NOTE", theme::kAccentCoral),
      chokeCc_(processor.parameters, params::kMidiChokeCc, "CHOKE CC", theme::kAccentCoral),
      rollEnabled_(processor.parameters, params::kRollEnabled, "ROLL", theme::kAccentBrass),
      rollDivision_(processor.parameters, params::kRollDivision, "DIVISION", theme::kAccentBrass),
      rollSwing_(processor.parameters, params::kRollSwing, "SWING", theme::kAccentBrass),
      rollAccent_(processor.parameters, params::kRollAccent, "ACCENT", theme::kAccentBrass) {
    for (juce::Component* component : std::array<juce::Component*, 10>{
             &velocityMetal_, &velocityAir_, &velocityChick_, &velocityReverb_, &chokeNote_, &chokeCc_,
             &rollEnabled_, &rollDivision_, &rollSwing_, &rollAccent_})
        addAndMakeVisible(component);

    velocityMetal_.setTooltip("Velocity curve for the tuned metal resonators.");
    velocityAir_.setTooltip("Velocity curve for the filtered noise layer.");
    velocityChick_.setTooltip("Velocity curve for the chick and stick transient layer.");
    velocityReverb_.setTooltip("Velocity curve for the reverb return.");
    chokeNote_.setTooltip("A note-off for this MIDI note chokes the current hat and stops the roll.");
    chokeCc_.setTooltip("A value of 64 or higher on this CC chokes the current hat and stops the roll.");
    rollEnabled_.setTooltip("Start a host-tempo hat roll after the next MIDI note-on.");
    rollDivision_.setTooltip("Host-synced roll step size.");
    rollSwing_.setTooltip("Delays alternating roll steps while keeping the average rate on the host grid.");
    rollAccent_.setTooltip("Velocity boost for every fourth roll hit.");

    for (int resonator = 1; resonator <= 6; ++resonator) {
        auto& controls = resonators_[static_cast<size_t>(resonator - 1)];
        const auto prefix = "resonator_" + juce::String(resonator);
        controls.tune = std::make_unique<Knob>(processor.parameters, prefix + "_tune",
                                               "R" + juce::String(resonator) + " TUNE", theme::kAccentBrass);
        controls.enabled = std::make_unique<Toggle>(processor.parameters, prefix + "_enabled", "ON", theme::kAccentAqua);
        controls.solo = std::make_unique<Toggle>(processor.parameters, prefix + "_solo", "SOLO", theme::kAccentCoral);
        controls.tune->setTooltip("Tune this metallic resonator by up to two octaves.");
        controls.enabled->setTooltip("Mute or enable this resonator component.");
        controls.solo->setTooltip("Solo this resonator. Multiple solo buttons can be active together.");
        addAndMakeVisible(controls.tune.get());
        addAndMakeVisible(controls.enabled.get());
        addAndMakeVisible(controls.solo.get());
    }
}

void PerformancePanel::paint(juce::Graphics& g) {
    drawPanel(g, getLocalBounds(), "PERFORMANCE / LAYERS", theme::kAccentBrass);
    drawSection(g, sequencerBounds_, "VELOCITY / MIDI / HOST ROLL", theme::kAccentAqua);
    drawSection(g, resonatorBounds_, "SIX-PART METAL RESONATOR BANK", theme::kAccentBrass);
}

void PerformancePanel::resized() {
    auto area = getLocalBounds().reduced(10, 29);
    sequencerBounds_ = area.removeFromTop(180);
    area.removeFromTop(8);
    resonatorBounds_ = area;

    layoutRows(sequencerBounds_.reduced(6, 26), {
        { &velocityMetal_, &velocityAir_, &velocityChick_, &velocityReverb_, &chokeNote_, &chokeCc_ },
        { &rollEnabled_, &rollDivision_, &rollSwing_, &rollAccent_, nullptr, nullptr }
    });

    auto grid = resonatorBounds_.reduced(7, 27);
    const int columnWidth = grid.getWidth() / static_cast<int>(resonators_.size());
    for (size_t i = 0; i < resonators_.size(); ++i) {
        auto cell = juce::Rectangle<int>(grid.getX() + static_cast<int>(i) * columnWidth,
                                        grid.getY(), columnWidth, grid.getHeight()).reduced(3, 0);
        const int toggleHeight = 30;
        auto toggles = cell.removeFromBottom(toggleHeight);
        resonators_[i].tune->setBounds(cell);
        const int toggleWidth = (toggles.getWidth() - 4) / 2;
        resonators_[i].enabled->setBounds(toggles.removeFromLeft(toggleWidth));
        toggles.removeFromLeft(4);
        resonators_[i].solo->setBounds(toggles);
    }
}

EffectsPanel::EffectsPanel(OpenHatterProcessor& processor)
    : distType_(processor.parameters, params::kDistType, "MODE", theme::kAccentCoral),
      distDrive_(processor.parameters, params::kDistDrive, "DRIVE", theme::kAccentCoral),
      distTone_(processor.parameters, params::kDistTone, "SHEEN", theme::kAccentCoral),
      distMix_(processor.parameters, params::kDistMix, "MIX", theme::kAccentCoral),
      compThresh_(processor.parameters, params::kCompThresh, "THRESHOLD", theme::kAccentBrass),
      compRatio_(processor.parameters, params::kCompRatio, "RATIO", theme::kAccentBrass),
      compAttack_(processor.parameters, params::kCompAttack, "ATTACK", theme::kAccentBrass),
      compRelease_(processor.parameters, params::kCompRelease, "RELEASE", theme::kAccentBrass),
      compMakeup_(processor.parameters, params::kCompMakeup, "MAKEUP", theme::kAccentBrass),
      compMix_(processor.parameters, params::kCompMix, "MIX", theme::kAccentBrass),
      limiterCeiling_(processor.parameters, params::kLimiterCeiling, "CEILING", theme::kAccentBrass),
      limiterRelease_(processor.parameters, params::kLimiterRelease, "LIMIT RELEASE", theme::kAccentBrass),
      limiterEnabled_(processor.parameters, params::kLimiterEnabled, "LIMITER", theme::kAccentBrass),
      reverbSize_(processor.parameters, params::kReverbSize, "SIZE", theme::kAccentAqua),
      reverbDecay_(processor.parameters, params::kReverbDecay, "DECAY", theme::kAccentAqua),
      reverbDamp_(processor.parameters, params::kReverbDamp, "DAMPING", theme::kAccentAqua),
      reverbGate_(processor.parameters, params::kReverbGate, "CHOKE", theme::kAccentAqua),
      reverbMix_(processor.parameters, params::kReverbMix, "MIX", theme::kAccentAqua),
      reverseMode_(processor.parameters, params::kReverseMode, "REVERSE SYNC", theme::kAccentAqua),
      reverseMix_(processor.parameters, params::kReverseMix, "REVERSE MIX", theme::kAccentAqua),
      masterWidth_(processor.parameters, params::kStereoWidth, "WIDTH", theme::kAccentAqua),
      outputGain_(processor.parameters, params::kOutputGain, "OUTPUT", theme::kAccentAqua),
      exciterLock_("Exciter", theme::kAccentCoral,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::exciter); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::exciter, on); }),
      dynamicsLock_("Dynamics", theme::kAccentBrass,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::dynamics); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::dynamics, on); }),
      spaceLock_("Space & Output", theme::kAccentAqua,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::spaceAndOutput); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::spaceAndOutput, on); }) {
    for (juce::Component* component : std::array<juce::Component*, 26>{
             &distType_, &distDrive_, &distTone_, &distMix_, &compThresh_, &compRatio_, &compAttack_, &compRelease_,
             &compMakeup_, &compMix_, &limiterCeiling_, &limiterRelease_, &limiterEnabled_, &reverbSize_, &reverbDecay_,
             &reverbDamp_, &reverbGate_, &reverbMix_, &reverseMode_, &reverseMix_, &masterWidth_, &outputGain_,
             &exciterLock_, &dynamicsLock_, &spaceLock_, nullptr})
        if (component != nullptr) addAndMakeVisible(component);
}

void EffectsPanel::paint(juce::Graphics& g) {
    drawPanel(g, getLocalBounds(), "FINISH", theme::kAccentCoral);
    drawSection(g, sectionBounds_[0], "EXCITER", theme::kAccentCoral);
    drawSection(g, sectionBounds_[1], "DYNAMICS", theme::kAccentBrass);
    drawSection(g, sectionBounds_[2], "SPACE / OUTPUT", theme::kAccentAqua);
}

void EffectsPanel::resized() {
    auto area = getLocalBounds().reduced(10, 29);
    constexpr int gap = 8;
    const int width = (area.getWidth() - gap * 2) / 3;
    for (size_t i = 0; i < sectionBounds_.size(); ++i)
        sectionBounds_[i] = { area.getX() + static_cast<int>(i) * (width + gap), area.getY(), width, area.getHeight() };

    exciterLock_.setBounds(sectionBounds_[0].getRight() - 68, sectionBounds_[0].getY() + 5, 58, 18);
    dynamicsLock_.setBounds(sectionBounds_[1].getRight() - 68, sectionBounds_[1].getY() + 5, 58, 18);
    spaceLock_.setBounds(sectionBounds_[2].getRight() - 68, sectionBounds_[2].getY() + 5, 58, 18);

    layoutRows(sectionBounds_[0].reduced(8, 29), {
        { &distType_, &distDrive_ },
        { &distTone_, &distMix_ }
    });
    layoutRows(sectionBounds_[1].reduced(7, 29), {
        { &compThresh_, &compRatio_, &compAttack_ },
        { &compRelease_, &compMakeup_, &compMix_ },
        { &limiterCeiling_, &limiterRelease_, &limiterEnabled_ }
    });
    layoutRows(sectionBounds_[2].reduced(7, 29), {
        { &reverbSize_, &reverbDecay_, &reverbDamp_ },
        { &reverbGate_, &reverbMix_, &reverseMode_ },
        { &reverseMix_, &masterWidth_, &outputGain_ }
    });
}

} // namespace openhatter::ui
