#include "PluginEditor.h"
#include "Presets.h"
#include "Theme.h"

namespace theme = openhatter::theme;

OpenHatterEditor::OpenHatterEditor(OpenHatterProcessor& processorToEdit)
    : AudioProcessorEditor(&processorToEdit), processor_(processorToEdit),
      midiPanel_(processorToEdit), synthPanel_(processorToEdit), outputPanel_(processorToEdit),
      envelopePanel_(processorToEdit), performancePanel_(processorToEdit), effectsPanel_(processorToEdit),
      tooltipWindow_(this, 500) {
    setLookAndFeel(&lookAndFeel_);
    const auto& presets = openhatter::presets::factoryPresets();
    juce::String family;
    juce::PopupMenu sounds;
    for (size_t i = 0; i < presets.size(); ++i) {
        const juce::String name(presets[i].name);
        const auto nextFamily = name.upToFirstOccurrenceOf(" / ", false, false);
        if (nextFamily != family && sounds.getNumItems() > 0) {
            presetBox_.getRootMenu()->addSubMenu(family, sounds);
            sounds.clear();
        }
        family = nextFamily;
        sounds.addItem(static_cast<int>(i + 1), name);
    }
    presetBox_.getRootMenu()->addSubMenu(family, sounds);
    presetBox_.setName("Factory preset");
    presetBox_.setTooltip("Choose a factory hat across six dance styles.");
    prevPresetBtn_.setTooltip("Load the previous factory hat.");
    nextPresetBtn_.setTooltip("Load the next factory hat.");
    hatTab_.setTooltip("Shape the air tail and stick strike.");
    envelopeTab_.setTooltip("Choose one-shot or held notes and shape the amplitude envelope.");
    performanceTab_.setTooltip("Edit velocity, MIDI choke and rolls.");
    effectsTab_.setTooltip("Edit the exciter, dynamics and space.");
    detailBtn_.setTooltip("Show or hide the air filter and bounce settings.");
    presetBox_.setTextWhenNothingSelected("Custom sound");
    presetBox_.onChange = [this] {
        if (presetBox_.getSelectedId() > 0)
            processor_.setCurrentProgram(presetBox_.getSelectedId() - 1);
    };
    prevPresetBtn_.onClick = [this] {
        const int count = processor_.getNumPrograms();
        const int current = processor_.hasLegacySound() ? 0 : processor_.getCurrentProgram();
        processor_.setCurrentProgram((current + count - 1) % count);
        syncPresetBox();
    };
    nextPresetBtn_.onClick = [this] {
        const int next = processor_.hasLegacySound() ? 0 : (processor_.getCurrentProgram() + 1) % processor_.getNumPrograms();
        processor_.setCurrentProgram(next);
        syncPresetBox();
    };
    auditionBtn_.setTooltip("Play a one-shot on the last MIDI pitch, or middle C before the first note.");
    auditionBtn_.setColour(juce::TextButton::buttonColourId, theme::kAccentBrass);
    auditionBtn_.setColour(juce::TextButton::textColourOffId, theme::kBackground);
    auditionBtn_.onClick = [this] { processor_.triggerAudition(); };

    hatTab_.onClick = [this] { setPage(Page::hat); };
    envelopeTab_.onClick = [this] { setPage(Page::envelope); };
    performanceTab_.onClick = [this] { setPage(Page::performance); };
    effectsTab_.onClick = [this] { setPage(Page::effects); };
    detailBtn_.setClickingTogglesState(true);
    detailBtn_.onClick = [this] {
        synthPanel_.setAdvancedMode(detailBtn_.getToggleState());
        setPage(page_);
    };
    for (auto* button : { &hatTab_, &envelopeTab_, &performanceTab_, &effectsTab_, &detailBtn_ }) {
        button->setToggleable(true);
        button->setColour(juce::TextButton::buttonOnColourId, theme::kAccentAqua);
    }
    for (juce::Component* child : std::initializer_list<juce::Component*>{
             &presetBox_, &prevPresetBtn_, &nextPresetBtn_, &auditionBtn_, &hatTab_, &envelopeTab_,
             &performanceTab_, &effectsTab_, &detailBtn_, &midiPanel_, &synthPanel_, &outputPanel_,
             &envelopePanel_, &performancePanel_, &effectsPanel_ })
        addAndMakeVisible(child);
    setPage(Page::hat);
    syncPresetBox();
    setSize(1060, 826);
    startTimerHz(15);
}

OpenHatterEditor::~OpenHatterEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void OpenHatterEditor::syncPresetBox() {
    presetBox_.setSelectedId(processor_.hasLegacySound() ? 0 : processor_.getCurrentProgram() + 1,
                             juce::dontSendNotification);
}

void OpenHatterEditor::setPage(Page page) {
    page_ = page;
    const bool hat = page == Page::hat;
    for (auto [tab, selected] : { std::pair{ &hatTab_, hat }, { &envelopeTab_, page == Page::envelope },
                                  { &performanceTab_, page == Page::performance }, { &effectsTab_, page == Page::effects } })
        tab->setToggleState(selected, juce::dontSendNotification);
    detailBtn_.setVisible(hat);
    midiPanel_.setVisible(hat && !detailBtn_.getToggleState());
    synthPanel_.setVisible(hat);
    outputPanel_.setVisible(hat);
    envelopePanel_.setVisible(page == Page::envelope);
    performancePanel_.setVisible(page == Page::performance);
    effectsPanel_.setVisible(page == Page::effects);
    resized();
}

void OpenHatterEditor::timerCallback() { syncPresetBox(); }

void OpenHatterEditor::paint(juce::Graphics& g) {
    g.fillAll(theme::kBackground);
    g.setColour(theme::kPanel);
    g.fillRect(0, 0, getWidth(), 86);
    g.setColour(theme::kAccentBrass);
    for (int i = 0; i < 3; ++i)
        g.drawEllipse(22.0f, 27.0f + static_cast<float>(i) * 6.0f, 39.0f, 14.0f, 1.8f);
    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(31.0f, juce::Font::bold));
    g.drawText("openHatter", 72, 13, 178, 38, juce::Justification::left);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("v" JucePlugin_VersionString, 250, 22, 48, 22, juce::Justification::left);
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("Pitched hi-hat synthesizer", 74, 49, 200, 22, juce::Justification::left);
    g.drawText("Factory preset", 309, 8, 220, 19, juce::Justification::left);
}

void OpenHatterEditor::resized() {
    presetBox_.setBounds(308, 33, 350, 38);
    prevPresetBtn_.setBounds(667, 33, 82, 38);
    nextPresetBtn_.setBounds(753, 33, 65, 38);
    auditionBtn_.setBounds(getWidth() - 155, 25, 133, 43);
    hatTab_.setBounds(22, 98, 83, 38);
    envelopeTab_.setBounds(109, 98, 112, 38);
    performanceTab_.setBounds(225, 98, 130, 38);
    effectsTab_.setBounds(359, 98, 92, 38);
    detailBtn_.setBounds(getWidth() - 177, 98, 155, 38);
    auto body = getLocalBounds().reduced(22, 0).withTrimmedTop(150).withTrimmedBottom(18);
    envelopePanel_.setBounds(body);
    effectsPanel_.setBounds(body);
    performancePanel_.setBounds(body);
    if (!detailBtn_.getToggleState()) {
        midiPanel_.setBounds(body.removeFromTop(164));
        body.removeFromTop(10);
    }
    outputPanel_.setBounds(body.removeFromBottom(150));
    body.removeFromBottom(10);
    synthPanel_.setBounds(body);
}
