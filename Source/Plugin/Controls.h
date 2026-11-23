#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <vector>

namespace openhatter::ui {

using Apvts = juce::AudioProcessorValueTreeState;

// A labelled control. It draws dim and stays editable while any caller marks it inactive.
class Control : public juce::Component {
public:
    void setTooltip(const juce::String& text);
    // Each caller passes its own howToEnable text, and the tooltip lists every current one.
    void setActive(bool active, const juce::String& howToEnable);

private:
    virtual void showTooltip(const juce::String& text) = 0;

    juce::String tooltip_;
    juce::StringArray inactiveHints_;
};

// Dims the controls while the source parameter is at its minimum, such as a switch that is off or a level at zero.
class DependsOn {
public:
    DependsOn(juce::RangedAudioParameter& source, juce::String howToEnable, std::vector<Control*> controls);

private:
    juce::String howToEnable_;
    std::vector<Control*> controls_;
    juce::ParameterAttachment attachment_;
};

class Knob : public Control {
public:
    Knob(Apvts& state, const juce::String& parameterId, const juce::String& title, juce::Colour accent);
    void resized() override;
    void bind(Apvts& state, const juce::String& parameterId);
    [[nodiscard]] juce::Slider& slider() noexcept { return slider_; }

private:
    void showTooltip(const juce::String& text) override;

    juce::Slider slider_;
    juce::Label title_;
    std::unique_ptr<Apvts::SliderAttachment> attachment_;
};

/** Dual concentric rotary EQ knob: inner cap adjusts dB (-12..+12), outer collar glides Hz smoothly. */
class DualEqKnob final : public juce::Component {
public:
    DualEqKnob(Apvts& state, const juce::String& freqId, const juce::String& dbId,
               const juce::String& title, juce::Colour dbAccent, juce::Colour freqAccent);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void setTooltip(const juce::String& text);
    [[nodiscard]] juce::Slider& slider() noexcept { return dbSlider_; }
    [[nodiscard]] const juce::String& freqId() const noexcept { return freqId_; }
    [[nodiscard]] const juce::String& dbId() const noexcept { return dbId_; }
    [[nodiscard]] juce::Rectangle<int> dbPillBounds() const;
    [[nodiscard]] juce::Rectangle<int> freqPillBounds() const;

private:
    class GainSlider final : public juce::Slider {
    public:
        using juce::Slider::Slider;
        bool hitTest(int x, int y) override;
    };

    // Gives the frequency collar keyboard focus and a spoken value. DualEqKnob keeps the collar's mouse handling.
    class FreqSlider final : public juce::Slider {
    public:
        bool keyPressed(const juce::KeyPress& key) override;
        void paint(juce::Graphics& g) override;
        void focusGained(FocusChangeType) override { repaint(); }
        void focusLost(FocusChangeType) override { repaint(); }
    };

    [[nodiscard]] juce::Rectangle<float> dialBounds() const;
    [[nodiscard]] bool hitsOuterRing(juce::Point<float> pos) const;
    void updateDbReadout();

    Apvts& state_;
    juce::String freqId_;
    juce::String dbId_;
    juce::Colour dbAccent_;
    juce::Colour freqAccent_;
    juce::Label title_;
    juce::Label dbReadout_;
    juce::Label freqEditor_;
    GainSlider dbSlider_;
    std::unique_ptr<Apvts::SliderAttachment> dbAttachment_;
    FreqSlider freqSlider_;
    std::unique_ptr<Apvts::SliderAttachment> freqSliderAttachment_;
    std::unique_ptr<juce::ParameterAttachment> freqAttachment_;
    float freqHz_ = 1000.0f;
    float dragStartNorm_ = 0.5f;
    float dragStartY_ = 0.0f;
    bool draggingFreq_ = false;
    bool hoveringFreq_ = false;
};

class Choice : public Control {
public:
    Choice(Apvts& state, const juce::String& parameterId, const juce::String& title, juce::Colour accent);
    void resized() override;
    void bind(Apvts& state, const juce::String& parameterId);
    [[nodiscard]] juce::ComboBox& box() noexcept { return box_; }

private:
    void showTooltip(const juce::String& text) override;

    juce::ComboBox box_;
    juce::Label title_;
    std::unique_ptr<Apvts::ComboBoxAttachment> attachment_;
};

class Toggle : public juce::Component {
public:
    Toggle(Apvts& state, const juce::String& parameterId, const juce::String& offText, const juce::String& onText, juce::Colour accent);
    void resized() override;
    void setTooltip(const juce::String& text);
    [[nodiscard]] juce::TextButton& button() noexcept { return button_; }

private:
    juce::TextButton button_;
    Apvts::ButtonAttachment attachment_;
};

class SectionLockButton : public juce::Component, private juce::Timer {
public:
    SectionLockButton(const juce::String& sectionName, juce::Colour accent,
                      std::function<bool()> isLocked, std::function<void(bool)> setLocked);
    ~SectionLockButton() override;
    void resized() override;
    void refresh();

private:
    void timerCallback() override;
    juce::TextButton button_;
    juce::String section_;
    std::function<bool()> isLocked_;
    std::function<void(bool)> setLocked_;
};

void layoutRow(juce::Rectangle<int> area, std::initializer_list<juce::Component*> components);

} // namespace openhatter::ui
