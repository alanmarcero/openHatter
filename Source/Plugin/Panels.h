#pragma once

#include "Controls.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <atomic>
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
    std::unique_ptr<juce::Drawable> cymbals_;
    juce::Component* upperPlate_ = nullptr;
    const double openedAt_ = juce::Time::getMillisecondCounterHiRes();
    float beatPhase_ = 0;
    int lastNote_ = -1;
    float lastVelocity_ = 0.0f;
    float airMs_ = 0;
    float bodyMs_ = 0;
    bool held_ = false;
    juce::ParameterAttachment holdAttachment_;
};

class HatSynthPanel : public juce::Component {
public:
    explicit HatSynthPanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setAdvancedMode(bool advanced);

private:
    // Hiss & air tail
    Choice noiseType_;
    Knob noiseAttack_;
    Knob noiseHold_;
    Knob noiseDecay_;
    Knob noiseCurve_;
    Knob noiseHPF_;
    Knob noiseLPF_;
    Knob noiseWidth_;
    Knob decayScale_;
    Knob noiseLevel_;
    Knob hissColor_;
    Knob hissDensity_;
    Knob hissDrive_;

    // Spun bronze body & stick articulation
    Knob bodyLevel_;
    Knob bodyTone_;
    Knob bodyDecay_;
    Knob preBursts_;
    Knob preSpread_;
    Knob preDecay_;
    Knob preJitter_;
    Knob flamSpread_;
    Knob keyTracking_;
    Knob preLevel_;
    Choice clickType_;
    Knob clickLevel_;
    Knob clickFreq_;

    SectionLockButton articulationLock_;
    SectionLockButton airLock_;
    juce::Label playModeBadge_;
    juce::ParameterAttachment holdAttachment_;
    std::array<juce::Rectangle<int>, 2> sectionBounds_{};
    bool advanced_ = false;
};

class EnvelopePanel : public juce::Component {
public:
    explicit EnvelopePanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    enum class DragHandle { none, attack, decaySustain, releaseSustain, sustainSegment };
    struct EnvelopeGeometry {
        juce::Rectangle<float> plot;
        float attackX = 0.0f;
        float decayX = 0.0f;
        float releaseX = 0.0f;
        float sustainY = 0.0f;
    };

    [[nodiscard]] EnvelopeGeometry computeGeometry() const noexcept;
    [[nodiscard]] DragHandle hitTestHandle(juce::Point<float> pos) const noexcept;
    void updateDrag(juce::Point<float> pos);
    void setHandleGesture(DragHandle handle, bool begin);

    OpenHatterProcessor& processor_;
    juce::TextButton oneShot_{ "One-shot" }, holdNotes_{ "Hold notes" };
    Knob attack_, decay_, sustain_, release_;
    SectionLockButton envelopeLock_;
    juce::ParameterAttachment gateAttachment_;
    DragHandle hoverHandle_ = DragHandle::none;
    DragHandle activeHandle_ = DragHandle::none;
};

class PerformancePanel : public juce::Component {
public:
    explicit PerformancePanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Knob velocityAir_;
    Knob velocityChick_;
    Knob velocityReverb_;
    Knob chokeNote_;
    Knob chokeCc_;
    Toggle rollEnabled_;
    Choice rollDivision_;
    Knob rollSwing_;
    Knob rollAccent_;
    juce::ParameterAttachment rollAttachment_;
    juce::Rectangle<int> velocityBounds_, rollBounds_;
    std::array<juce::Rectangle<int>, 3> curveBounds_{};
};

class EffectsPanel : public juce::Component {
public:
    explicit EffectsPanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;

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
    Knob reverbSize_;
    Knob reverbDecay_;
    Knob reverbDamp_;
    Knob reverbGate_;
    Knob reverbMix_;
    Knob echoMix_;
    Choice reverseMode_;
    Knob reverseMix_;
    SectionLockButton exciterLock_;
    SectionLockButton dynamicsLock_;
    SectionLockButton spaceLock_;
    juce::ParameterAttachment reverseAttachment_;
    std::array<juce::Rectangle<int>, 3> sectionBounds_{};
};

// Shows output peaks in dBFS, holds each for 1 s, and latches a clip light until a click.
class PeakMeter final : public juce::Component, public juce::SettableTooltipClient, private juce::Timer {
public:
    explicit PeakMeter(std::atomic<float>& peak);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    // The loudest sample since the last read.
    void pushPeak(float linearPeak, double nowMs);
    [[nodiscard]] float heldDb() const noexcept { return heldDb_; }
    [[nodiscard]] bool isClipped() const noexcept { return clipped_; }

private:
    static constexpr float kFloorDb = -60.0f;
    void timerCallback() override;

    std::atomic<float>& peak_;
    float levelDb_ = kFloorDb;
    float heldDb_ = kFloorDb;
    double heldAtMs_ = 0.0;
    double lastMs_ = 0.0;
    bool clipped_ = false;
};

/** Output EQ (dual-layer 3-band), stereo width, gain, and limiter on the main Hat page. */
class OutputPanel : public juce::Component {
public:
    explicit OutputPanel(OpenHatterProcessor& processor);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    DualEqKnob eqLow_;
    DualEqKnob eqMid_;
    DualEqKnob eqHigh_;
    Knob masterWidth_;
    Knob outputGain_;
    Knob limiterCeiling_;
    Knob limiterRelease_;
    Toggle limiterEnabled_;
    PeakMeter peakMeter_;
    SectionLockButton outputLock_;
    juce::ParameterAttachment limiterAttachment_;
};

} // namespace openhatter::ui
