#include "PluginEditor.h"
#include "Presets.h"
#include "Theme.h"

namespace {
namespace theme = openhatter::theme;

void styleHeaderButton(juce::TextButton& button, juce::Colour accent) {
    button.setColour(juce::TextButton::buttonColourId, theme::kKnobBody);
    button.setColour(juce::TextButton::buttonOnColourId, accent.withAlpha(0.22f));
    button.setColour(juce::TextButton::textColourOffId, accent);
    button.setColour(juce::TextButton::textColourOnId, accent.brighter(0.2f));
}
} // namespace

OpenHatterEditor::OpenHatterEditor(OpenHatterProcessor& processorToEdit)
    : AudioProcessorEditor(&processorToEdit),
      processor_(processorToEdit),
      midiPanel_(processorToEdit),
      synthPanel_(processorToEdit),
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

    addAndMakeVisible(midiPanel_);
    addAndMakeVisible(synthPanel_);
    addAndMakeVisible(effectsPanel_);
    syncPresetBox();
    setSize(1100, 972);
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

void OpenHatterEditor::timerCallback() {
    if (presetBox_.getSelectedId() != processor_.getCurrentProgram() + 1)
        syncPresetBox();
}

void OpenHatterEditor::paint(juce::Graphics& g) {
    g.fillAll(theme::kBackground);
    const auto header = getLocalBounds().removeFromTop(46);
    g.setGradientFill(juce::ColourGradient(theme::kPanel.brighter(0.08f), 0.0f, 0.0f,
                                           theme::kBackground, 0.0f, static_cast<float>(header.getHeight()), false));
    g.fillRect(header);
    g.setColour(theme::kPanelEdge);
    g.drawHorizontalLine(header.getBottom() - 1, 0.0f, static_cast<float>(getWidth()));
    g.setColour(theme::kAccentBrass);
    g.fillRect(0, header.getBottom() - 2, 192, 2);

    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("openHatter", 16, 5, 158, 26, juce::Justification::left, true);
    g.setColour(theme::kAccentAqua);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("MIDI-TRACKED CYMBAL SYNTH", 18, 29, 260, 13, juce::Justification::left, true);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    g.drawText("V" JucePlugin_VersionString "  /  48 FACTORY PRESETS", getWidth() - 250, 15, 232, 16,
               juce::Justification::right, true);

    const auto presetBar = juce::Rectangle<int>(12, 51, getWidth() - 24, 39).toFloat();
    g.setColour(theme::kPanel);
    g.fillRoundedRectangle(presetBar, 6.0f);
    g.setColour(theme::kPanelEdge);
    g.drawRoundedRectangle(presetBar.reduced(0.5f), 6.0f, 1.0f);
}

void OpenHatterEditor::resized() {
    constexpr int margin = 12;
    const int rowY = 54;
    prevPresetBtn_.setBounds(24, rowY, 34, 32);
    presetBox_.setBounds(64, rowY + 2, 560, 28);
    nextPresetBtn_.setBounds(630, rowY, 34, 32);
    auditionBtn_.setBounds(getWidth() - 162, rowY, 138, 32);

    midiPanel_.setBounds(margin, 100, getWidth() - margin * 2, 148);
    synthPanel_.setBounds(margin, 258, getWidth() - margin * 2, 390);
    effectsPanel_.setBounds(margin, 658, getWidth() - margin * 2, getHeight() - 670);
}
