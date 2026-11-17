#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace openhatter::ui {

using Apvts = juce::AudioProcessorValueTreeState;

class Knob : public juce::Component {
public:
    Knob(Apvts& state, const juce::String& parameterId, const juce::String& title, juce::Colour accent);
    void resized() override;
    void bind(Apvts& state, const juce::String& parameterId);
    void setTooltip(const juce::String& text);
    [[nodiscard]] juce::Slider& slider() noexcept { return slider_; }

private:
    juce::Slider slider_;
    juce::Label title_;
    std::unique_ptr<Apvts::SliderAttachment> attachment_;
};

class Choice : public juce::Component {
public:
    Choice(Apvts& state, const juce::String& parameterId, const juce::String& title, juce::Colour accent);
    void resized() override;
    void bind(Apvts& state, const juce::String& parameterId);
    void setTooltip(const juce::String& text);
    [[nodiscard]] juce::ComboBox& box() noexcept { return box_; }

private:
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
