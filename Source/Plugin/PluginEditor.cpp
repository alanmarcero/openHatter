#include "PluginEditor.h"
#include "Presets.h"
#include "Theme.h"
#include <algorithm>

namespace {
namespace theme = openhatter::theme;

void styleHeaderButton(juce::TextButton& button, juce::Colour accent) {
    button.setColour(juce::TextButton::buttonColourId, theme::kKnobBody);
    button.setColour(juce::TextButton::buttonOnColourId, accent);
    button.setColour(juce::TextButton::textColourOffId, accent);
    button.setColour(juce::TextButton::textColourOnId, accent.brighter(0.2f));
}
} // namespace

OpenHatterEditor::OpenHatterEditor(OpenHatterProcessor& processorToEdit)
    : AudioProcessorEditor(&processorToEdit),
      processor_(processorToEdit),
      midiPanel_(processorToEdit),
      synthPanel_(processorToEdit),
      performancePanel_(processorToEdit),
      effectsPanel_(processorToEdit),
      tooltipWindow_(this, 500) {
    juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel_);

    const auto& presets = openhatter::presets::factoryPresets();
    for (size_t i = 0; i < presets.size(); ++i)
        presetBox_.addItem(presets[i].name, static_cast<int>(i + 1));
    presetBox_.setTooltip("Browse 48 presets in six styles. MIDI input always sets the played pitch and octave.");
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

    styleHeaderButton(auditionBtn_, theme::kAccentBrass);
    auditionBtn_.setTooltip("Play a short preview using the last MIDI note received, or middle C if no note has arrived.");
    auditionBtn_.onClick = [this] { processor_.triggerAudition(); };
    addAndMakeVisible(auditionBtn_);

    styleHeaderButton(pageToggleBtn_, theme::kAccentAqua);
    pageToggleBtn_.setTooltip("Switch between sound design and performance controls.");
    pageToggleBtn_.onClick = [this] { setPage(1 - page_); };
    addAndMakeVisible(pageToggleBtn_);

    addAndMakeVisible(midiPanel_);
    addAndMakeVisible(synthPanel_);
    addAndMakeVisible(performancePanel_);
    addAndMakeVisible(effectsPanel_);
    setPage(0);
    syncPresetBox();
    setSize(1360, 972);
    setResizable(false, false);
    startTimerHz(15);
}

OpenHatterEditor::~OpenHatterEditor() {
    stopTimer();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void OpenHatterEditor::syncPresetBox() {
    presetBox_.setSelectedId(processor_.getCurrentProgram() + 1, juce::dontSendNotification);
}

void OpenHatterEditor::setPage(int page) {
    page_ = std::clamp(page, 0, 1);
    const bool soundPage = page_ == 0;
    midiPanel_.setVisible(soundPage);
    synthPanel_.setVisible(soundPage);
    effectsPanel_.setVisible(soundPage);
    performancePanel_.setVisible(!soundPage);
    pageToggleBtn_.setButtonText(soundPage ? "PERFORMANCE CONTROLS" : "SOUND DESIGN CONTROLS");
    resized();
    repaint();
}

void OpenHatterEditor::timerCallback() {
    if (presetBox_.getSelectedId() != processor_.getCurrentProgram() + 1)
        syncPresetBox();
}

void OpenHatterEditor::paint(juce::Graphics& g) {
    g.fillAll(theme::kBackground);
    const auto header = getLocalBounds().removeFromTop(50);
    g.setGradientFill(juce::ColourGradient(theme::kPanel.brighter(0.08f), 0.0f, 0.0f,
                                           theme::kBackground, 0.0f, static_cast<float>(header.getHeight()), false));
    g.fillRect(header);
    g.setColour(theme::kPanelEdge);
    g.drawHorizontalLine(header.getBottom() - 1, 0.0f, static_cast<float>(getWidth()));
    g.setColour(theme::kAccentBrass);
    g.fillRect(0, header.getBottom() - 2, 192, 2);

    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("openHatter", 18, 4, 174, 28, juce::Justification::left, true);
    g.setColour(theme::kAccentSteel);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("MIDI-TRACKED CYMBAL SYNTH", 20, 32, 280, 14, juce::Justification::left, true);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("V" JucePlugin_VersionString "  /  48 FACTORY PRESETS", getWidth() - 300, 16, 282, 18,
               juce::Justification::right, true);

    const auto presetBar = juce::Rectangle<int>(12, 52, getWidth() - 24, 46).toFloat();
    g.setColour(theme::kPanel);
    g.fillRoundedRectangle(presetBar, 6.0f);
    g.setColour(theme::kPanelEdge);
    g.drawRoundedRectangle(presetBar.reduced(0.5f), 6.0f, 1.0f);
}

void OpenHatterEditor::resized() {
    constexpr int margin = 12;
    const int rowY = 56;
    prevPresetBtn_.setBounds(20, rowY, 148, 38);
    presetBox_.setBounds(178, rowY, 630, 38);
    nextPresetBtn_.setBounds(818, rowY, 142, 38);
    pageToggleBtn_.setBounds(970, rowY, 222, 38);
    auditionBtn_.setBounds(getWidth() - 180, rowY, 160, 38);

    if (page_ == 0) {
        midiPanel_.setBounds(margin, 100, getWidth() - margin * 2, 148);
        synthPanel_.setBounds(margin, 258, getWidth() - margin * 2, 390);
        effectsPanel_.setBounds(margin, 658, getWidth() - margin * 2, getHeight() - 670);
    } else {
        performancePanel_.setBounds(margin, 100, getWidth() - margin * 2, getHeight() - 112);
    }
}
