#include "Controls.h"
#include "Theme.h"

namespace openhatter::ui {

Knob::Knob(Apvts& state, const juce::String& parameterId, const juce::String& title, juce::Colour accent) {
    slider_.setName(title);
    slider_.setComponentID(parameterId);
    slider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 68, 20);
    slider_.setColour(juce::Slider::rotarySliderFillColourId, accent);
    addAndMakeVisible(slider_);

    title_.setText(title, juce::dontSendNotification);
    title_.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    title_.setJustificationType(juce::Justification::centred);
    title_.setColour(juce::Label::textColourId, theme::kTextDim);
    addAndMakeVisible(title_);

    bind(state, parameterId);
}

void Knob::bind(Apvts& state, const juce::String& parameterId) {
    if (auto* param = state.getParameter(parameterId)) {
        slider_.setTextValueSuffix(param->getLabel().isEmpty() ? juce::String{} : " " + param->getLabel());
        slider_.setDoubleClickReturnValue(true, param->convertFrom0to1(param->getDefaultValue()));
    }
    attachment_ = std::make_unique<Apvts::SliderAttachment>(state, parameterId, slider_);
}

void Knob::resized() {
    auto area = getLocalBounds().withSizeKeepingCentre(getWidth(), std::min(getHeight(), 135));
    title_.setBounds(area.removeFromTop(19));
    slider_.setBounds(area);
}

void Knob::setTooltip(const juce::String& text) {
    slider_.setTooltip(text);
    title_.setTooltip(text);
}

Choice::Choice(Apvts& state, const juce::String& parameterId, const juce::String& title, juce::Colour accent) {
    box_.setName(title);
    box_.setComponentID(parameterId);
    box_.setColour(juce::ComboBox::arrowColourId, accent);
    box_.setColour(juce::ComboBox::backgroundColourId, theme::kBackground);
    box_.setColour(juce::ComboBox::outlineColourId, theme::kPanelEdge);
    addAndMakeVisible(box_);

    title_.setText(title, juce::dontSendNotification);
    title_.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    title_.setJustificationType(juce::Justification::centred);
    title_.setColour(juce::Label::textColourId, theme::kTextDim);
    addAndMakeVisible(title_);

    bind(state, parameterId);
}

void Choice::bind(Apvts& state, const juce::String& parameterId) {
    attachment_.reset();
    box_.clear(juce::dontSendNotification);
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(state.getParameter(parameterId))) {
        box_.addItemList(param->choices, 1);
        attachment_ = std::make_unique<Apvts::ComboBoxAttachment>(state, parameterId, box_);
    }
}

void Choice::resized() {
    auto area = getLocalBounds().reduced(5, 0).withSizeKeepingCentre(getWidth() - 10, std::min(getHeight(), 62));
    title_.setBounds(area.removeFromTop(20));
    box_.setBounds(area.reduced(1, 3));
}

void Choice::setTooltip(const juce::String& text) {
    box_.setTooltip(text);
    title_.setTooltip(text);
}

Toggle::Toggle(Apvts& state, const juce::String& parameterId, const juce::String& offText,
               const juce::String& onText, juce::Colour accent)
    : attachment_(state, parameterId, button_) {
    button_.setComponentID(parameterId);
    button_.onStateChange = [this, offText, onText] {
        button_.setButtonText(button_.getToggleState() ? onText : offText);
    };
    button_.onStateChange();
    button_.setClickingTogglesState(true);
    button_.setColour(juce::TextButton::buttonOnColourId, accent);
    addAndMakeVisible(button_);
}

void Toggle::resized() {
    button_.setBounds(getLocalBounds().reduced(2, 0).withHeight(38).withCentre(getLocalBounds().getCentre()));
}

void Toggle::setTooltip(const juce::String& text) {
    button_.setTooltip(text);
}

SectionLockButton::SectionLockButton(const juce::String& sectionName, juce::Colour accent,
                                     std::function<bool()> isLocked, std::function<void(bool)> setLocked)
    : section_(sectionName), isLocked_(std::move(isLocked)), setLocked_(std::move(setLocked)) {
    button_.setToggleable(true);
    button_.setColour(juce::TextButton::buttonOnColourId, accent);
    button_.setColour(juce::TextButton::textColourOffId, accent);
    button_.onClick = [this] {
        setLocked_(!isLocked_());
        refresh();
    };
    addAndMakeVisible(button_);
    refresh();
    startTimerHz(8);
}

void SectionLockButton::resized() {
    button_.setBounds(getLocalBounds());
}

void SectionLockButton::refresh() {
    const bool locked = isLocked_();
    button_.setToggleState(locked, juce::dontSendNotification);
    button_.setButtonText(locked ? "Unlock" : "Lock");
    button_.setTitle(button_.getButtonText() + " " + section_);
    button_.setTooltip(locked ? "Allow presets to change " + section_ + "."
                              : "Keep " + section_ + " settings when changing presets.");
}

void SectionLockButton::timerCallback() {
    if (button_.getToggleState() != isLocked_()) refresh();
}

void layoutRow(juce::Rectangle<int> area, std::initializer_list<juce::Component*> components) {
    if (components.size() == 0) return;
    const int count = static_cast<int>(components.size());
    const int itemWidth = area.getWidth() / count;
    int idx = 0;
    for (auto* c : components) {
        if (c != nullptr) {
            c->setBounds(area.getX() + idx * itemWidth, area.getY(), itemWidth, area.getHeight());
        }
        idx++;
    }
}

} // namespace openhatter::ui
