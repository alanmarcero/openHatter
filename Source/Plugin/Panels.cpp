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
    g.setColour(theme::kPanel.brighter(0.025f));
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
    g.setColour(theme::kPanel.brighter(0.05f));
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

    g.setColour(theme::kPanel.brighter(0.05f));
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
    for (juce::Component* component : std::array<juce::Component*, 31>{
             &toneMode_, &toneFreq_, &toneQ_, &toneDecay_, &tonePitchEnv_, &tonePitchDecay_, &toneLevel_,
             &noiseType_, &noiseAttack_, &noiseHold_, &noiseDecay_, &noiseCurve_, &noiseHPF_, &noiseLPF_, &noiseWidth_, &noiseLevel_,
             &preBursts_, &preSpread_, &preDecay_, &preJitter_, &flamSpread_, &preLevel_,
             &ampAttack_, &ampDecay_, &ampSustain_, &ampRelease_, &clickType_, &clickLevel_, &clickFreq_, &articulationLock_, &cymbalLock_})
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
        { &tonePitchEnv_, &tonePitchDecay_, nullptr }
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
        { &ampAttack_, &ampDecay_ },
        { &ampSustain_, &ampRelease_ },
        { &clickType_, &clickLevel_ },
        { &clickFreq_, nullptr }
    });
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
