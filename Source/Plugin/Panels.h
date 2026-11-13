#pragma once

#include "Controls.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>

class OpenHatterProcessor;

namespace openhatter::ui {

class HatterStagePanel : public juce::Component, private juce::Timer {
public:
    explicit HatterStagePanel(OpenHatterProcessor& processor);
    ~HatterStagePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;

    OpenHatterProcessor& processor_;
    juce::Image mascot_;
    juce::TextButton auditionBtn_{ "PUSH IT" };
    uint32_t seenTriggers_ = 0;
    double hatTriggerTimeMs_ = 0.0;
    float hatPose_ = 0.0f;
    float shockwave_ = 0.0f;
    float impactIntensity_ = 1.0f;
    float impactFlash_ = 0.0f;
    float headTilt_ = 0.0f;
    float idleBob_ = 0.0f;
};

class HatSynthPanel : public juce::Component {
public:
    explicit HatSynthPanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setAdvancedMode(bool advanced);
    void setPage(int page);
    [[nodiscard]] bool isAdvancedMode() const noexcept { return advancedMode_; }

private:
    bool advancedMode_ = false;

    // Stick Bounces / Pre-Chick
    Knob preBursts_;
    Knob preSpread_;
    Knob preDecay_;
    Knob preJitter_;
    Knob flamSpread_;
    Knob preLevel_;

    // Tunable Cymbal Matrix & Body
    Choice toneMode_;
    Choice toneNote_;
    Knob toneOctave_;
    Knob toneFine_;
    Toggle lockPitchBtn_;
    Knob toneFreq_;
    Knob toneQ_;
    Knob toneDecay_;
    Knob tonePitchEnv_;
    Knob tonePitchDecay_;
    Knob toneLevel_;
    Knob ampAttack_;
    Knob ampDecay_;
    Knob ampSustain_;
    Knob ampRelease_;

    // Air Sizzle & Shimmer Noise
    Choice noiseType_;
    Knob noiseAttack_;
    Knob noiseHold_;
    Knob noiseDecay_;
    Knob noiseCurve_;
    Knob noiseHPF_;
    Knob noiseLPF_;
    Knob noiseWidth_;
    Knob noiseLevel_;

    // Stick Chick
    Choice clickType_;
    Knob clickLevel_;
    Knob clickFreq_;

    SectionLockButton articulationLock_;
    SectionLockButton cymbalLock_;
    SectionLockButton airLock_;
    std::array<juce::Rectangle<int>, 3> sectionBounds_{};
    int page_ = 0;
};

class EffectsPanel : public juce::Component {
public:
    explicit EffectsPanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setAdvancedMode(bool advanced);
    [[nodiscard]] bool isAdvancedMode() const noexcept { return advancedMode_; }

private:
    bool advancedMode_ = false;

    // Shimmer Exciter / Distortion
    Choice distType_;
    Knob distDrive_;
    Knob distTone_;
    Knob distMix_;

    // Compressor
    Knob compThresh_;
    Knob compRatio_;
    Knob compAttack_;
    Knob compRelease_;
    Knob compMakeup_;
    Knob compMix_;

    // Limiter
    Knob limiterCeiling_;
    Knob limiterRelease_;
    Toggle limiterEnabled_;

    // Shimmer Reverb & Choke Gate
    Knob reverbSize_;
    Knob reverbDecay_;
    Knob reverbDamp_;
    Knob reverbGate_;
    Knob reverbMix_;

    // Reverse & Master
    Choice reverseMode_;
    Knob reverseMix_;
    Choice midiMode_;
    Toggle lockInBtn_;
    Knob masterWidth_;
    Knob outputGain_;

    SectionLockButton exciterLock_;
    SectionLockButton dynamicsLock_;
    SectionLockButton spaceLock_;
    std::array<juce::Rectangle<int>, 3> sectionBounds_{};
};

} // namespace openhatter::ui
