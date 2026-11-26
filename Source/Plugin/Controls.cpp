#include "Controls.h"
#include "Parameters.h"
#include "Theme.h"

#include <algorithm>
#include <cmath>

namespace openhatter::ui {

void Control::setTooltip(const juce::String& text) {
    tooltip_ = text;
    showTooltip((tooltip_ + " " + inactiveHints_.joinIntoString(" ")).trim());
}

void Control::setActive(bool active, const juce::String& howToEnable) {
    if (active)
        inactiveHints_.removeString(howToEnable);
    if (!active)
        inactiveHints_.addIfNotAlreadyThere(howToEnable);
    setAlpha(inactiveHints_.isEmpty() ? 1.0f : theme::kInactiveControlAlpha);
    showTooltip((tooltip_ + " " + inactiveHints_.joinIntoString(" ")).trim());
}

DependsOn::DependsOn(juce::RangedAudioParameter& source, juce::String howToEnable, std::vector<Control*> controls)
    : howToEnable_(std::move(howToEnable)), controls_(std::move(controls)),
      attachment_(source, [this, &source](float value) {
          const bool active = source.convertTo0to1(value) > 0.0f;
          std::ranges::for_each(controls_, [&](Control* control) { control->setActive(active, howToEnable_); });
      }) {
    attachment_.sendInitialUpdate();
}

Knob::Knob(Apvts& state, const juce::String& parameterId, const juce::String& title, juce::Colour accent) {
    slider_.setName(title);
    slider_.setWantsKeyboardFocus(true);
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
    if (auto* param = state.getParameter(parameterId))
        slider_.setTitle(param->getName(64));
    attachment_ = std::make_unique<Apvts::SliderAttachment>(state, parameterId, slider_);
}

void Knob::resized() {
    auto area = getLocalBounds().withSizeKeepingCentre(getWidth(), std::min(getHeight(), 135));
    title_.setBounds(area.removeFromTop(19));
    slider_.setBounds(area);
}

void Knob::showTooltip(const juce::String& text) {
    slider_.setTooltip(text);
    title_.setTooltip(text);
}

bool DualEqKnob::GainSlider::hitTest(int x, int y) {
    const auto centre = getLocalBounds().toFloat().getCentre();
    const float radius = static_cast<float>(std::min(getWidth(), getHeight())) * 0.5f;
    return juce::Point<float>(static_cast<float>(x), static_cast<float>(y)).getDistanceFrom(centre) <= radius;
}

bool DualEqKnob::FreqSlider::keyPressed(const juce::KeyPress& key) {
    const int code = key.getKeyCode();
    const bool raise = code == juce::KeyPress::upKey || code == juce::KeyPress::rightKey;
    const bool lower = code == juce::KeyPress::downKey || code == juce::KeyPress::leftKey;
    if (!raise && !lower)
        return false;
    // A step is one semitone across the 2-octave collar, or one octave with Shift.
    const double step = key.getModifiers().isShiftDown() ? (12.0 / 24.0) : (1.0 / 24.0);
    const double proportion = std::clamp(valueToProportionOfLength(getValue()) + (raise ? step : -step), 0.0, 1.0);
    const ScopedDragNotification gesture(*this);
    setValue(proportionOfLengthToValue(proportion), juce::sendNotificationSync);
    return true;
}

void DualEqKnob::FreqSlider::paint(juce::Graphics& g) {
    if (!hasKeyboardFocus(false))
        return;
    g.setColour(theme::kAccentCyan);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.75f), 4.0f, 1.5f);
}

DualEqKnob::DualEqKnob(Apvts& state, const juce::String& freqId, const juce::String& dbId,
                       const juce::String& title, juce::Colour dbAccent, juce::Colour freqAccent)
    : state_(state), freqId_(freqId), dbId_(dbId), dbAccent_(dbAccent), freqAccent_(freqAccent) {
    title_.setText(title, juce::dontSendNotification);
    title_.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    title_.setJustificationType(juce::Justification::centred);
    title_.setColour(juce::Label::textColourId, theme::kTextDim);
    addAndMakeVisible(title_);

    dbSlider_.setName(title + " Gain");
    dbSlider_.setTitle(title + " gain");
    dbSlider_.setWantsKeyboardFocus(true);
    dbSlider_.setComponentID(dbId);
    dbSlider_.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    dbSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    dbSlider_.setColour(juce::Slider::rotarySliderFillColourId, dbAccent_);
    if (auto* dbParam = state.getParameter(dbId)) {
        dbSlider_.setDoubleClickReturnValue(true, dbParam->convertFrom0to1(dbParam->getDefaultValue()));
    }
    dbAttachment_ = std::make_unique<Apvts::SliderAttachment>(state, dbId, dbSlider_);
    dbSlider_.onValueChange = [this] { updateDbReadout(); };
    addAndMakeVisible(dbSlider_);

    freqSlider_.setName(title + " Freq");
    freqSlider_.setTitle(title + " frequency");
    freqSlider_.setComponentID(freqId);
    freqSlider_.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    freqSlider_.setWantsKeyboardFocus(true);
    freqSlider_.setInterceptsMouseClicks(false, false);
    freqSliderAttachment_ = std::make_unique<Apvts::SliderAttachment>(state, freqId, freqSlider_);
    addAndMakeVisible(freqSlider_);

    dbReadout_.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    dbReadout_.setJustificationType(juce::Justification::centred);
    dbReadout_.setColour(juce::Label::textColourId, theme::kText);
    dbReadout_.setColour(juce::Label::backgroundColourId, theme::kBackground);
    dbReadout_.setEditable(false, true, false);
    dbReadout_.onTextChange = [this] {
        auto* dbParam = state_.getParameter(dbId_);
        if (dbParam == nullptr) return;
        const float value = dbParam->convertFrom0to1(dbParam->getValueForText(dbReadout_.getText()));
        dbSlider_.setValue(value, juce::sendNotificationSync);
    };
    addAndMakeVisible(dbReadout_);

    freqEditor_.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    freqEditor_.setJustificationType(juce::Justification::centred);
    freqEditor_.setColour(juce::Label::textColourId, theme::kText);
    freqEditor_.setColour(juce::Label::backgroundColourId, theme::kBackground);
    freqEditor_.setColour(juce::Label::outlineColourId, freqAccent_);
    freqEditor_.setEditable(false, false, false);
    freqEditor_.setInterceptsMouseClicks(false, false);
    freqEditor_.onTextChange = [this] {
        auto* freqParam = state_.getParameter(freqId_);
        if (freqParam == nullptr || freqAttachment_ == nullptr) return;
        const float value = freqParam->convertFrom0to1(freqParam->getValueForText(freqEditor_.getText()));
        freqHz_ = value;
        freqAttachment_->setValueAsCompleteGesture(value);
        repaint();
    };
    freqEditor_.onEditorHide = [this] { freqEditor_.setVisible(false); };
    addChildComponent(freqEditor_);

    if (auto* freqParam = state.getParameter(freqId)) {
        freqHz_ = freqParam->convertFrom0to1(freqParam->getValue());
        freqAttachment_ = std::make_unique<juce::ParameterAttachment>(*freqParam, [this](float value) {
            freqHz_ = value;
            repaint();
        });
        freqAttachment_->sendInitialUpdate();
    }
    updateDbReadout();
}

void DualEqKnob::updateDbReadout() {
    dbReadout_.setText(dbSlider_.getTextFromValue(dbSlider_.getValue()), juce::dontSendNotification);
}

juce::Rectangle<float> DualEqKnob::dialBounds() const {
    const auto middle = getLocalBounds().withTrimmedTop(18);
    const float size = static_cast<float>(std::min(72, std::min(middle.getWidth() - 74, middle.getHeight())));
    return juce::Rectangle<float>(static_cast<float>(middle.getX()) + 2.0f,
                                  middle.toFloat().getCentreY() - size * 0.5f,
                                  size, size);
}

juce::Rectangle<int> DualEqKnob::dbPillBounds() const {
    const auto middle = getLocalBounds().withTrimmedTop(18);
    const int left = juce::roundToInt(dialBounds().getRight()) + 6;
    const int width = std::max(0, middle.getRight() - left - 2);
    const int cy = middle.getCentreY();
    return { left, cy - 22, width, 20 };
}

juce::Rectangle<int> DualEqKnob::freqPillBounds() const {
    const auto middle = getLocalBounds().withTrimmedTop(18);
    const int left = juce::roundToInt(dialBounds().getRight()) + 6;
    const int width = std::max(0, middle.getRight() - left - 2);
    const int cy = middle.getCentreY();
    return { left, cy + 2, width, 20 };
}

bool DualEqKnob::hitsOuterRing(juce::Point<float> pos) const {
    const auto dial = dialBounds();
    const float dist = pos.getDistanceFrom(dial.getCentre());
    const float outerR = dial.getWidth() * 0.5f + 3.0f;
    const float innerR = dial.getWidth() * 0.28f;
    return (dist >= innerR && dist <= outerR) || freqPillBounds().toFloat().contains(pos);
}

void DualEqKnob::paint(juce::Graphics& g) {
    const auto dial = dialBounds();
    const auto centre = dial.getCentre();
    const float outerR = dial.getWidth() * 0.5f - 2.0f;
    constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    constexpr float endAngle = juce::MathConstants<float>::pi * 2.75f;

    auto* freqParam = state_.getParameter(freqId_);
    const float norm = freqParam != nullptr ? freqParam->convertTo0to1(freqHz_) : 0.5f;
    const float angle = startAngle + norm * (endAngle - startAngle);
    const auto ringColour = (hoveringFreq_ || draggingFreq_) ? freqAccent_.brighter(0.25f) : freqAccent_;

    juce::Path base;
    base.addCentredArc(centre.x, centre.y, outerR, outerR, 0.0f, startAngle, endAngle, true);
    g.setColour(theme::kPanelEdge);
    g.strokePath(base, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc(centre.x, centre.y, outerR, outerR, 0.0f, startAngle, angle, true);
    g.setColour(ringColour);
    g.strokePath(value, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto [minMidi, maxMidi] = openhatter::params::eqBandMidiLimits(freqId_);
    const int semitoneSpan = std::max(1, maxMidi - minMidi);
    g.setColour(theme::kTextDim.withAlpha(0.55f));
    for (int step = 0; step <= semitoneSpan; ++step) {
        const float markNorm = static_cast<float>(step) / static_cast<float>(semitoneSpan);
        const float markAngle = startAngle + markNorm * (endAngle - startAngle);
        const bool octaveRoot = ((minMidi + step) % 12) == 9;
        const float innerTick = outerR - (octaveRoot ? 4.0f : 2.0f);
        const float outerTick = outerR + (octaveRoot ? 4.0f : 2.0f);
        g.drawLine(centre.x + innerTick * std::sin(markAngle), centre.y - innerTick * std::cos(markAngle),
                   centre.x + outerTick * std::sin(markAngle), centre.y - outerTick * std::cos(markAngle),
                   octaveRoot ? 1.2f : 0.75f);
    }

    const float dotX = centre.x + outerR * std::sin(angle);
    const float dotY = centre.y - outerR * std::cos(angle);
    g.setColour(theme::kText);
    g.fillEllipse(dotX - 2.3f, dotY - 2.3f, 4.6f, 4.6f);

    const auto dbPill = dbPillBounds();
    const auto freqPill = freqPillBounds().toFloat();
    g.setColour(theme::kBackground.brighter(hoveringFreq_ || draggingFreq_ ? 0.1f : 0.0f));
    g.fillRoundedRectangle(freqPill, 4.0f);
    g.setColour(ringColour.withAlpha(hoveringFreq_ || draggingFreq_ ? 0.9f : 0.45f));
    g.drawRoundedRectangle(freqPill.reduced(0.5f), 4.0f, 1.0f);
    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(11.5f, juce::Font::bold));
    if (freqParam != nullptr)
        g.drawFittedText(freqParam->getText(norm, 0), freqPill.toNearestInt(), juce::Justification::centred, 1);

    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("dB", dbPill.getX(), dbPill.getY() - 15, dbPill.getWidth(), 14, juce::Justification::centred);
    g.drawText("Note", freqPill.toNearestInt().getX(), freqPill.toNearestInt().getBottom() + 1,
               freqPill.toNearestInt().getWidth(), 14, juce::Justification::centred);
}

void DualEqKnob::resized() {
    auto area = getLocalBounds();
    title_.setBounds(area.removeFromTop(18));
    const auto dial = dialBounds();
    const float innerSize = std::max(46.0f, dial.getWidth() * 0.70f);
    dbSlider_.setBounds(juce::Rectangle<float>(innerSize, innerSize).withCentre(dial.getCentre()).toNearestInt());
    dbReadout_.setBounds(dbPillBounds());
    freqSlider_.setBounds(freqPillBounds());
    freqEditor_.setBounds(freqPillBounds());
}

void DualEqKnob::mouseMove(const juce::MouseEvent& event) {
    const bool over = hitsOuterRing(event.position);
    if (over != hoveringFreq_) {
        hoveringFreq_ = over;
        repaint();
    }
}

void DualEqKnob::mouseExit(const juce::MouseEvent&) {
    if (!hoveringFreq_) return;
    hoveringFreq_ = false;
    repaint();
}

void DualEqKnob::mouseDown(const juce::MouseEvent& event) {
    if (event.mods.isPopupMenu()) {
        if (!freqPillBounds().toFloat().contains(event.position) || freqAttachment_ == nullptr) return;
        auto* param = state_.getParameter(freqId_);
        if (param == nullptr) return;
        const auto [minMidi, maxMidi] = openhatter::params::eqBandMidiLimits(freqId_);
        const int current = suite::eq::nearestMidi(freqHz_, minMidi, maxMidi);
        auto menu = suite::eq::noteMenu(minMidi, maxMidi, current, 1);
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this, minMidi, maxMidi](int itemId) {
                const auto midi = suite::eq::midiFromNoteItem(itemId, 1, minMidi, maxMidi);
                if (!midi || freqAttachment_ == nullptr) return;
                const float hz = suite::eq::noteToHz(*midi);
                freqHz_ = hz;
                freqAttachment_->setValueAsCompleteGesture(hz);
                repaint();
            });
        return;
    }
    if (!hitsOuterRing(event.position)) return;
    auto* param = state_.getParameter(freqId_);
    if (param == nullptr || freqAttachment_ == nullptr) return;
    freqSlider_.grabKeyboardFocus();
    draggingFreq_ = true;
    dragStartNorm_ = param->convertTo0to1(freqHz_);
    dragStartY_ = event.position.y;
    freqAttachment_->beginGesture();
    repaint();
}

void DualEqKnob::mouseDrag(const juce::MouseEvent& event) {
    if (!draggingFreq_ || freqAttachment_ == nullptr) return;
    auto* param = state_.getParameter(freqId_);
    if (param == nullptr) return;
    const float norm = std::clamp(dragStartNorm_ + (dragStartY_ - event.position.y) / 140.0f, 0.0f, 1.0f);
    const float nextHz = param->convertFrom0to1(norm);
    freqHz_ = nextHz;
    freqAttachment_->setValueAsPartOfGesture(nextHz);
    repaint();
}

void DualEqKnob::mouseUp(const juce::MouseEvent&) {
    if (!draggingFreq_ || freqAttachment_ == nullptr) return;
    draggingFreq_ = false;
    freqAttachment_->endGesture();
    repaint();
}

void DualEqKnob::mouseDoubleClick(const juce::MouseEvent& event) {
    if (freqPillBounds().toFloat().contains(event.position)) {
        if (auto* param = state_.getParameter(freqId_)) {
            const float norm = param->convertTo0to1(freqHz_);
            freqEditor_.setBounds(freqPillBounds());
            freqEditor_.setText(param->getText(norm, 0), juce::dontSendNotification);
            freqEditor_.setVisible(true);
            freqEditor_.showEditor();
        }
        return;
    }
    if (!hitsOuterRing(event.position) || freqAttachment_ == nullptr) return;
    auto* param = state_.getParameter(freqId_);
    if (param == nullptr) return;
    const float defaultHz = param->convertFrom0to1(param->getDefaultValue());
    freqHz_ = defaultHz;
    freqAttachment_->setValueAsCompleteGesture(defaultHz);
    repaint();
}

void DualEqKnob::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    if (!hitsOuterRing(event.position) || freqAttachment_ == nullptr) return;
    auto* param = state_.getParameter(freqId_);
    if (param == nullptr || wheel.deltaY == 0.0f) return;
    const auto [minMidi, maxMidi] = openhatter::params::eqBandMidiLimits(freqId_);
    const float semitoneStep = 1.0f / static_cast<float>(std::max(1, maxMidi - minMidi));
    const float direction = wheel.deltaY > 0.0f ? 1.0f : -1.0f;
    const float nextNorm = std::clamp(param->convertTo0to1(freqHz_) + direction * semitoneStep, 0.0f, 1.0f);
    const float nextHz = param->convertFrom0to1(nextNorm);
    freqHz_ = nextHz;
    freqAttachment_->setValueAsCompleteGesture(nextHz);
    repaint();
}

void DualEqKnob::setTooltip(const juce::String& text) {
    dbSlider_.setTooltip(text);
    freqSlider_.setTooltip(text);
    dbReadout_.setTooltip(text);
    title_.setTooltip(text);
}

Choice::Choice(Apvts& state, const juce::String& parameterId, const juce::String& title, juce::Colour accent) {
    box_.setName(title);
    box_.setComponentID(parameterId);
    box_.setWantsKeyboardFocus(true);
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
        box_.setTitle(param->getName(64));
        attachment_ = std::make_unique<Apvts::ComboBoxAttachment>(state, parameterId, box_);
    }
}

void Choice::resized() {
    auto area = getLocalBounds().reduced(5, 0).withSizeKeepingCentre(getWidth() - 10, std::min(getHeight(), 64));
    title_.setBounds(area.removeFromTop(20));
    box_.setBounds(area.reduced(1, 3));
}

void Choice::showTooltip(const juce::String& text) {
    box_.setTooltip(text);
    title_.setTooltip(text);
}

Toggle::Toggle(Apvts& state, const juce::String& parameterId, const juce::String& offText,
               const juce::String& onText, juce::Colour accent)
    : attachment_(state, parameterId, button_) {
    button_.setComponentID(parameterId);
    button_.setWantsKeyboardFocus(true);
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

SectionLockButton::~SectionLockButton() {
    stopTimer();
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
