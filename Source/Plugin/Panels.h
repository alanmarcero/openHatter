#pragma once

#include "Controls.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <memory>

class OpenHatterProcessor;

namespace openhatter::ui {

class MidiMonitorPanel : public juce::Component, private juce::Timer {
public:
    explicit MidiMonitorPanel(OpenHatterProcessor& processor);
    ~MidiMonitorPanel() override;
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    OpenHatterProcessor& processor_;
    int lastNote_ = -1;
    float lastVelocity_ = 0.0f;
};

class HatSynthPanel : public juce::Component {
public:
    explicit HatSynthPanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setAdvancedMode(bool) {}
    void setPage(int) {}
    [[nodiscard]] bool isAdvancedMode() const noexcept { return false; }

private:
    // Metallic resonator
    Choice toneMode_;
    Knob toneFreq_;
    Knob toneQ_;
    Knob toneDecay_;
    Knob tonePitchEnv_;
    Knob tonePitchDecay_;
    Knob metalSpread_;
    Knob metalDampingSlope_;
    Knob metalStereoSpread_;
    Knob toneLevel_;

    // Noise tail
    Choice noiseType_;
    Knob noiseAttack_;
    Knob noiseHold_;
    Knob noiseDecay_;
    Knob noiseCurve_;
    Knob noiseHPF_;
    Knob noiseLPF_;
    Knob noiseWidth_;
    Knob noiseLevel_;

    // Articulation and one-shot envelope
    Knob preBursts_;
    Knob preSpread_;
    Knob preDecay_;
    Knob preJitter_;
    Knob flamSpread_;
    Knob preLevel_;
    Toggle fullAdsr_;
    Knob ampAttack_;
    Knob ampDecay_;
    Knob ampSustain_;
    Knob ampRelease_;
    Choice clickType_;
    Knob clickLevel_;
    Knob clickFreq_;

    SectionLockButton articulationLock_;
    SectionLockButton cymbalLock_;
    SectionLockButton airLock_;
    std::array<juce::Rectangle<int>, 4> sectionBounds_{};
};

class PerformancePanel : public juce::Component {
public:
    explicit PerformancePanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct ResonatorControls {
        std::unique_ptr<Knob> tune;
        std::unique_ptr<Toggle> enabled;
        std::unique_ptr<Toggle> solo;
    };

    Knob velocityMetal_;
    Knob velocityAir_;
    Knob velocityChick_;
    Knob velocityReverb_;
    Knob chokeNote_;
    Knob chokeCc_;
    Toggle rollEnabled_;
    Choice rollDivision_;
    Knob rollSwing_;
    Knob rollAccent_;
    std::array<ResonatorControls, 6> resonators_;
    juce::Rectangle<int> sequencerBounds_;
    juce::Rectangle<int> resonatorBounds_;
};

class EffectsPanel : public juce::Component {
public:
    explicit EffectsPanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setAdvancedMode(bool) {}
    [[nodiscard]] bool isAdvancedMode() const noexcept { return false; }

private:
    Choice distType_;
    Knob distDrive_;
    Knob distTone_;
    Knob distMix_;
    Knob compThresh_;
    Knob compRatio_;
    Knob compAttack_;
    Knob compRelease_;
    Knob compMakeup_;
    Knob compMix_;
    Knob limiterCeiling_;
    Knob limiterRelease_;
    Toggle limiterEnabled_;
    Knob reverbSize_;
    Knob reverbDecay_;
    Knob reverbDamp_;
    Knob reverbGate_;
    Knob reverbMix_;
    Choice reverseMode_;
    Knob reverseMix_;
    Knob masterWidth_;
    Knob outputGain_;
    SectionLockButton exciterLock_;
    SectionLockButton dynamicsLock_;
    SectionLockButton spaceLock_;
    std::array<juce::Rectangle<int>, 3> sectionBounds_{};
};

} // namespace openhatter::ui
