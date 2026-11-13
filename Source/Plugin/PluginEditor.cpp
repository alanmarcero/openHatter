#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"
#include "Theme.h"

namespace {
using namespace openhatter::theme;
namespace theme = openhatter::theme;

void styleHeaderButton(juce::TextButton& button, juce::Colour accent) {
    button.setClickingTogglesState(true);
    button.setColour(juce::TextButton::buttonColourId, kKnobBody);
    button.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.24f));
    button.setColour(juce::TextButton::textColourOffId, kTextDim.brighter(0.2f));
    button.setColour(juce::TextButton::textColourOnId, accent);
}
} // namespace

OpenHatterEditor::OpenHatterEditor(OpenHatterProcessor& processorToEdit)
    : AudioProcessorEditor(&processorToEdit),
      processor_(processorToEdit),
      stagePanel_(processorToEdit),
      synthPanel_(processorToEdit),
      effectsPanel_(processorToEdit),
      tooltipWindow_(this, 500) {
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel_);

    const auto& presets = openhatter::presets::factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        presetBox_.addItem(presets[i].name, static_cast<int>(i + 1));
    presetBox_.setTooltip("Choose one of the 48 new trance-focused open-hat presets.");
    presetBox_.onChange = [this] {
        const int selected = presetBox_.getSelectedId() - 1;
        if (selected >= 0 && selected < processor_.getNumPrograms())
            processor_.setCurrentProgram(selected);
    };
    addAndMakeVisible(presetBox_);

    prevPresetBtn_.setTooltip("Previous factory preset");
    prevPresetBtn_.onClick = [this] {
        const int count = processor_.getNumPrograms();
        processor_.setCurrentProgram((processor_.getCurrentProgram() - 1 + count) % count);
        syncPresetBox();
    };
    addAndMakeVisible(prevPresetBtn_);
    nextPresetBtn_.setTooltip("Next factory preset");
    nextPresetBtn_.onClick = [this] {
        const int count = processor_.getNumPrograms();
        processor_.setCurrentProgram((processor_.getCurrentProgram() + 1) % count);
        syncPresetBox();
    };
    addAndMakeVisible(nextPresetBtn_);

    styleHeaderButton(simpleAdvancedBtn_, theme::kAccentAmber);
    simpleAdvancedBtn_.setTooltip("Switch between core controls and the full parameter controls.");
    simpleAdvancedBtn_.onClick = [this] { setAdvancedView(!advanced_); };
    addAndMakeVisible(simpleAdvancedBtn_);

    for (size_t i = 0; i < pageButtons_.size(); ++i) {
        auto& button = pageButtons_[i];
        const auto accent = i == 0 ? theme::kAccentCyan : (i == 1 ? theme::kAccentViolet : theme::kAccentFlame);
        styleHeaderButton(button, accent);
        button.setTooltip(i == 0 ? "Cymbal body, pitch, ADSR and chick transient"
                                 : (i == 1 ? "Pre-chick articulation and air-tail motion"
                                           : "Exciter, dynamics, space and output"));
        button.onClick = [this, i] { setPage(static_cast<int>(i)); };
        addAndMakeVisible(button);
    }

    addAndMakeVisible(stagePanel_);
    addAndMakeVisible(synthPanel_);
    addAndMakeVisible(effectsPanel_);
    syncPresetBox();
    setPage(0);
    setAdvancedView(false);
    setSize(1100, 972);
    setResizable(false, false);
    startTimerHz(20);
}

OpenHatterEditor::~OpenHatterEditor() {
    stopTimer();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void OpenHatterEditor::setAdvancedView(bool advanced) {
    advanced_ = advanced;
    simpleAdvancedBtn_.setButtonText(advanced ? "FULL DETAIL" : "SIMPLE");
    simpleAdvancedBtn_.setToggleState(advanced, juce::dontSendNotification);
    synthPanel_.setAdvancedMode(advanced);
    effectsPanel_.setAdvancedMode(advanced);
    resized();
    repaint();
}

void OpenHatterEditor::setPage(int page) {
    page_ = std::clamp(page, 0, 2);
    synthPanel_.setPage(page_ == 1 ? 1 : 0);
    const bool hatPage = page_ == 0;
    const bool motionPage = page_ == 1;
    stagePanel_.setVisible(hatPage);
    synthPanel_.setVisible(hatPage || motionPage);
    effectsPanel_.setVisible(page_ == 2);
    for (size_t i = 0; i < pageButtons_.size(); ++i)
        pageButtons_[i].setToggleState(static_cast<int>(i) == page_, juce::dontSendNotification);
    resized();
    repaint();
}

void OpenHatterEditor::syncPresetBox() {
    presetBox_.setSelectedId(processor_.getCurrentProgram() + 1, juce::dontSendNotification);
}

void OpenHatterEditor::timerCallback() {
    if (presetBox_.getSelectedId() != processor_.getCurrentProgram() + 1)
        syncPresetBox();
}

void OpenHatterEditor::paint(juce::Graphics& g) {
    g.fillAll(theme::kBackground);
    const auto header = getLocalBounds().removeFromTop(46);
    g.setGradientFill(juce::ColourGradient(juce::Colour(0xff172332), 0.0f, 0.0f,
                                           juce::Colour(0xff0a1018), 0.0f, static_cast<float>(header.getHeight()), false));
    g.fillRect(header);
    g.setColour(theme::kPanelEdge);
    g.drawHorizontalLine(header.getBottom() - 1, 0.0f, static_cast<float>(getWidth()));
    g.setColour(theme::kAccentCyan);
    g.fillRect(0, header.getBottom() - 2, 190, 2);

    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("openHatter", 16, 6, 158, 25, juce::Justification::left, true);
    g.setColour(theme::kAccentAmber);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("TRANCE OPEN-HAT INSTRUMENT", 18, 29, 260, 13, juce::Justification::left, true);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("V" JucePlugin_VersionString "  /  48 NEW CLUB PRESETS", getWidth() - 250, 16, 232, 16,
               juce::Justification::right, true);

    auto controls = getLocalBounds().withTop(48).withHeight(38);
    g.setColour(theme::kPanel.brighter(0.02f));
    g.fillRoundedRectangle(controls.reduced(8, 1).toFloat(), 6.0f);
}

void OpenHatterEditor::resized() {
    constexpr int margin = 12;
    const int rowY = 51;
    const int rowH = 34;
    int x = margin;
    for (auto& pageButton : pageButtons_) {
        pageButton.setBounds(x, rowY, pageButton.getButtonText() == "MOTION" ? 90 : 72, rowH);
        x = pageButton.getRight() + 5;
    }

    const int presetWidth = 336;
    auto presetArea = juce::Rectangle<int>((getWidth() - presetWidth) / 2, rowY, presetWidth, rowH);
    prevPresetBtn_.setBounds(presetArea.removeFromLeft(32).reduced(2, 3));
    presetArea.removeFromLeft(5);
    presetBox_.setBounds(presetArea.removeFromLeft(260).reduced(0, 3));
    presetArea.removeFromLeft(5);
    nextPresetBtn_.setBounds(presetArea.reduced(2, 3));
    simpleAdvancedBtn_.setBounds(getWidth() - margin - 112, rowY, 112, rowH);

    const int contentTop = 96;
    const auto content = getLocalBounds().withTop(contentTop).reduced(margin, 0).withTrimmedBottom(margin);
    if (page_ == 0) {
        auto body = content;
        stagePanel_.setBounds(body.removeFromLeft(278).reduced(0, 2));
        synthPanel_.setBounds(body.reduced(0, 2));
    } else if (page_ == 1) {
        synthPanel_.setBounds(content.reduced(0, 2));
    } else {
        effectsPanel_.setBounds(content.reduced(0, 2));
    }
}
