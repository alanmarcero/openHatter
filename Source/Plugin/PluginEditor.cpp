#include "PluginEditor.h"
#include "BinaryData.h"
#include "Parameters.h"
#include "Presets.h"
#include "Theme.h"

OpenHatterEditor::OpenHatterEditor(OpenHatterProcessor& processorToEdit)
    : AudioProcessorEditor(&processorToEdit),
      processor_(processorToEdit),
      stagePanel_(processorToEdit),
      synthPanel_(processorToEdit),
      effectsPanel_(processorToEdit),
      tooltipWindow_(this, 500) {
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel_);

    faceplate_ = juce::ImageCache::getFromMemory(
        BinaryData::openhatterfaceplate_png, BinaryData::openhatterfaceplate_pngSize);

    const auto& presets = openhatter::presets::factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i) {
        presetBox_.addItem(presets[i].name, static_cast<int>(i + 1));
    }
    presetBox_.onChange = [this]() {
        const int selected = presetBox_.getSelectedId() - 1;
        if (selected >= 0 && selected < processor_.getNumPrograms()) {
            processor_.setCurrentProgram(selected);
        }
    };
    syncPresetBox();
    addAndMakeVisible(presetBox_);

    prevPresetBtn_.onClick = [this]() {
        const int current = processor_.getCurrentProgram();
        const int prev = (current - 1 + processor_.getNumPrograms()) % processor_.getNumPrograms();
        processor_.setCurrentProgram(prev);
        syncPresetBox();
    };
    addAndMakeVisible(prevPresetBtn_);

    nextPresetBtn_.onClick = [this]() {
        const int current = processor_.getCurrentProgram();
        const int next = (current + 1) % processor_.getNumPrograms();
        processor_.setCurrentProgram(next);
        syncPresetBox();
    };
    addAndMakeVisible(nextPresetBtn_);

    viewToggleBtn_.setButtonText(synthPanel_.isAdvancedMode() ? "ADVANCED" : "SIMPLE");
    viewToggleBtn_.setColour(juce::TextButton::buttonColourId, openhatter::theme::kKnobBody);
    viewToggleBtn_.setColour(juce::TextButton::textColourOffId, openhatter::theme::kAccentCyan);
    viewToggleBtn_.setTooltip("Toggle between clean Simple mode and deep Advanced tweak controls.");
    viewToggleBtn_.onClick = [this]() {
        setAdvancedView(!synthPanel_.isAdvancedMode());
    };
    addAndMakeVisible(viewToggleBtn_);

    addAndMakeVisible(stagePanel_);
    addAndMakeVisible(synthPanel_);
    addAndMakeVisible(effectsPanel_);

    setSize(1100, 740);
    setResizable(false, false);
    startTimerHz(20);
}

OpenHatterEditor::~OpenHatterEditor() {
    stopTimer();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void OpenHatterEditor::setAdvancedView(bool advanced) {
    syncPresetBox();
    synthPanel_.setAdvancedMode(advanced);
    effectsPanel_.setAdvancedMode(advanced);
    viewToggleBtn_.setButtonText(advanced ? "ADVANCED" : "SIMPLE");
    viewToggleBtn_.setColour(juce::TextButton::textColourOffId,
        advanced ? openhatter::theme::kAccentAmber : openhatter::theme::kAccentCyan);
    resized();
    repaint();
}

void OpenHatterEditor::syncPresetBox() {
    presetBox_.setSelectedId(processor_.getCurrentProgram() + 1, juce::dontSendNotification);
}

void OpenHatterEditor::timerCallback() {
    if (presetBox_.getSelectedId() != processor_.getCurrentProgram() + 1) {
        syncPresetBox();
    }
}

void OpenHatterEditor::paint(juce::Graphics& g) {
    g.fillAll(openhatter::theme::kBackground);

    if (faceplate_.isValid()) {
        const juce::Graphics::ScopedSaveState state(g);
        g.setOpacity(0.45f);
        g.drawImageWithin(faceplate_, 0, 0, getWidth(), getHeight(),
                          juce::RectanglePlacement::stretchToFit);
    }

    const auto headerArea = getLocalBounds().removeFromTop(44);
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff111822), 0.0f, 0.0f,
        juce::Colour(0xff090e15), 0.0f, static_cast<float>(headerArea.getHeight()), false));
    g.fillRect(headerArea);
    g.setColour(openhatter::theme::kPanelEdge);
    g.drawHorizontalLine(headerArea.getBottom() - 1, 0.0f, static_cast<float>(getWidth()));
    g.setColour(openhatter::theme::kAccentAmber.withAlpha(0.65f));
    g.drawHorizontalLine(headerArea.getBottom() - 1, 12.0f, 260.0f);

    g.setColour(openhatter::theme::kAccentCyan);
    g.setFont(juce::FontOptions(20.0f, juce::Font::bold));
    g.drawText("openHatter", 16, headerArea.getY() + 10, 140, 24, juce::Justification::left, true);

    g.setColour(openhatter::theme::kAccentAmber);
    g.setFont(juce::FontOptions(13.0f, juce::Font::italic));
    g.drawText("Hat on. Off-beat.", 150, headerArea.getY() + 13, 140, 20, juce::Justification::left, true);

    g.setColour(openhatter::theme::kTextDim.brighter(0.25f));
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("v1.0.0 | TUNABLE OPEN HI-HAT SYNTH", getWidth() - 260, headerArea.getY() + 13, 244, 20, juce::Justification::right, true);
}

void OpenHatterEditor::resized() {
    auto area = getLocalBounds().reduced(10, 8);

    auto header = area.removeFromTop(38);
    const int presetW = 240;
    auto presetArea = header.withX(header.getCentreX() - presetW / 2).withWidth(presetW);
    prevPresetBtn_.setBounds(presetArea.getX() - 28, presetArea.getY() + 4, 24, 26);
    presetBox_.setBounds(presetArea.withHeight(26).translated(0, 4));
    nextPresetBtn_.setBounds(presetArea.getRight() + 4, presetArea.getY() + 4, 24, 26);
    viewToggleBtn_.setBounds(nextPresetBtn_.getRight() + 12, presetArea.getY() + 4, 86, 26);

    const int stageWidth = 330;
    stagePanel_.setBounds(area.removeFromLeft(stageWidth).reduced(2, 2));

    auto rightArea = area.reduced(2, 2);
    const float synthRatio = synthPanel_.isAdvancedMode() ? 0.58f : 0.52f;
    const int synthHeight = static_cast<int>(static_cast<float>(rightArea.getHeight()) * synthRatio);
    synthPanel_.setBounds(rightArea.removeFromTop(synthHeight).reduced(0, 2));
    effectsPanel_.setBounds(rightArea.reduced(0, 2));
}
