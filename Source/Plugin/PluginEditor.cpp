#include "PluginEditor.h"
#include "Parameters.h"
#include "Presets.h"
#include "SuiteStyle.h"
#include "Theme.h"
#include <algorithm>
#include <vector>

namespace theme = openhatter::theme;

namespace {

suite::Style editorStyle() {
    return {
        theme::kBackground, theme::kPanel, theme::kPanelEdge, theme::kText, theme::kTextDim,
        theme::kAccentAqua, theme::kAccentBrass
    };
}

} // namespace

OpenHatterEditor::OpenHatterEditor(OpenHatterProcessor& processorToEdit)
    : AudioProcessorEditor(&processorToEdit), processor_(processorToEdit),
      presetLibrary_(suite::PresetLibrary::defaultDirectory("openHatter")),
      presetBrowser_(presetLibrary_, editorStyle()),
      dragBtn_([this] { return processor_.exportHatWav(); }),
      midiPanel_(processorToEdit), synthPanel_(processorToEdit), outputPanel_(processorToEdit),
      envelopePanel_(processorToEdit), performancePanel_(processorToEdit), effectsPanel_(processorToEdit),
      tooltipWindow_(this, 500) {
    setLookAndFeel(&lookAndFeel_);
    setWantsKeyboardFocus(true);
    content_.setBounds(0, 0, OpenHatterProcessor::kEditorWidth, OpenHatterProcessor::kEditorHeight);
    addAndMakeVisible(content_);
    std::vector<juce::String> factoryNames;
    const auto& presets = openhatter::presets::factoryPresets();
    factoryNames.reserve(presets.size());
    juce::String family;
    juce::PopupMenu sounds;
    for (size_t i = 0; i < presets.size(); ++i) {
        const juce::String name(presets[i].name);
        factoryNames.push_back(name);
        const auto nextFamily = name.upToFirstOccurrenceOf(" / ", false, false);
        if (nextFamily != family && sounds.getNumItems() > 0) {
            presetBox_.getRootMenu()->addSubMenu(family, sounds);
            sounds.clear();
        }
        family = nextFamily;
        sounds.addItem(static_cast<int>(i + 1), name);
    }
    presetBox_.getRootMenu()->addSubMenu(family, sounds);
    presetLibrary_.setFactoryNames(std::move(factoryNames));
    presetBrowser_.refresh();
    presetBox_.setName("Factory preset");
    presetBox_.setTooltip("Choose a factory hat across six dance styles.");
    prevPresetBtn_.setTooltip("Load the previous factory hat.");
    nextPresetBtn_.setTooltip("Load the next factory hat.");
    hatTab_.setTooltip("Shape the air tail and stick strike.");
    envelopeTab_.setTooltip("Choose one-shot or held notes and shape the amplitude envelope.");
    performanceTab_.setTooltip("Edit velocity, MIDI choke and rolls.");
    effectsTab_.setTooltip("Edit the exciter, dynamics and space.");
    detailBtn_.setTooltip("Show or hide the air filter and bounce settings.");
    undoBtn_.setTooltip("Undo the last sound change.");
    redoBtn_.setTooltip("Redo the last undone sound change.");
    slotABtn_.setTooltip("Hear slot A.");
    slotBBtn_.setTooltip("Hear slot B.");
    copyBtn_.setTooltip("Copy the sound you hear into the other A/B slot.");
    lockBtn_.setTooltip("Keep a rendered hit. Drag exports that hit until you unlock.");
    dragBtn_.setTooltip("Drag the locked hit, or a fresh render, out as a WAV file.");
    presetsBtn_.setTooltip("Browse factory and user presets, search, and star favorites.");
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
    undoBtn_.onClick = [this] { processor_.undo(); refreshHistoryButtons(); };
    redoBtn_.onClick = [this] { processor_.redo(); refreshHistoryButtons(); };
    slotABtn_.onClick = [this] { processor_.selectAbSlot(0); refreshHistoryButtons(); };
    slotBBtn_.onClick = [this] { processor_.selectAbSlot(1); refreshHistoryButtons(); };
    copyBtn_.onClick = [this] { processor_.copyAbToOther(); };
    presetsBtn_.onClick = [this] { showPresetBrowser(!presetBrowser_.isVisible()); };
    lockBtn_.setClickingTogglesState(true);
    lockAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.parameters, openhatter::params::kLockIn, lockBtn_);

    presetBrowser_.setLoadFactory([this](int index) {
        processor_.setCurrentProgram(index);
        syncPresetBox();
    });
    presetBrowser_.setLoadUser([this](const juce::String& name) {
        const auto tree = presetLibrary_.readUser(name);
        if (tree.isValid())
            processor_.loadUserPreset(tree, name);
        syncPresetBox();
    });
    presetBrowser_.setCaptureState([this] { return processor_.currentPresetTree(); });
    presetBrowser_.setDismiss([this] { showPresetBrowser(false); });

    hatTab_.onClick = [this] { showPresetBrowser(false); setPage(Page::hat); };
    envelopeTab_.onClick = [this] { showPresetBrowser(false); setPage(Page::envelope); };
    performanceTab_.onClick = [this] { showPresetBrowser(false); setPage(Page::performance); };
    effectsTab_.onClick = [this] { showPresetBrowser(false); setPage(Page::effects); };
    detailBtn_.setClickingTogglesState(true);
    detailBtn_.onClick = [this] {
        synthPanel_.setAdvancedMode(detailBtn_.getToggleState());
        setPage(page_);
    };
    for (auto* button : { &hatTab_, &envelopeTab_, &performanceTab_, &effectsTab_, &detailBtn_,
                          &slotABtn_, &slotBBtn_, &lockBtn_, &presetsBtn_ }) {
        button->setToggleable(true);
        button->setColour(juce::TextButton::buttonOnColourId, theme::kAccentAqua);
    }
    slotBBtn_.setColour(juce::TextButton::buttonOnColourId, theme::kAccentBrass);
    for (juce::Component* child : std::initializer_list<juce::Component*>{
             &presetBox_, &prevPresetBtn_, &nextPresetBtn_, &auditionBtn_,
             &undoBtn_, &redoBtn_, &slotABtn_, &slotBBtn_, &copyBtn_, &lockBtn_, &dragBtn_, &presetsBtn_,
             &hatTab_, &envelopeTab_, &performanceTab_, &effectsTab_, &detailBtn_,
             &midiPanel_, &synthPanel_, &outputPanel_, &envelopePanel_, &performancePanel_, &effectsPanel_ })
        content_.addAndMakeVisible(child);
    content_.addChildComponent(presetBrowser_);
    const auto restoredSize = processor_.editorSize();
    setPage(Page::hat);
    syncPresetBox();
    setResizeLimits(OpenHatterProcessor::kEditorMinWidth, OpenHatterProcessor::kEditorMinHeight,
                    OpenHatterProcessor::kEditorMaxWidth, OpenHatterProcessor::kEditorMaxHeight);
    const double designWidth = OpenHatterProcessor::kEditorWidth;
    const double designHeight = OpenHatterProcessor::kEditorHeight;
    getConstrainer()->setFixedAspectRatio(designWidth / designHeight);
    setResizable(true, true);
    // Sessions from the free-resize versions can hold any aspect ratio.
    const double scale = std::min(restoredSize.x / designWidth, restoredSize.y / designHeight);
    setSize(juce::roundToInt(designWidth * scale), juce::roundToInt(designHeight * scale));
    refreshHistoryButtons();
    startTimerHz(15);
}

OpenHatterEditor::~OpenHatterEditor() {
    stopTimer();
    setLookAndFeel(nullptr);
}

void OpenHatterEditor::syncPresetBox() {
    if (processor_.hasLegacySound() || processor_.activeUserPreset().isNotEmpty())
        presetBox_.setSelectedId(0, juce::dontSendNotification);
    else
        presetBox_.setSelectedId(processor_.getCurrentProgram() + 1, juce::dontSendNotification);
}

void OpenHatterEditor::showPresetBrowser(bool show) {
    presetBrowser_.setVisible(show);
    presetsBtn_.setToggleState(show, juce::dontSendNotification);
    if (show)
        presetBrowser_.refresh();
    setPage(page_);
}

void OpenHatterEditor::setPage(Page page) {
    page_ = page;
    const bool browser = presetBrowser_.isVisible();
    const bool hat = page == Page::hat && !browser;
    for (auto [tab, selected] : { std::pair{ &hatTab_, page == Page::hat }, { &envelopeTab_, page == Page::envelope },
                                  { &performanceTab_, page == Page::performance }, { &effectsTab_, page == Page::effects } })
        tab->setToggleState(selected, juce::dontSendNotification);
    detailBtn_.setVisible(hat);
    midiPanel_.setVisible(hat && !detailBtn_.getToggleState());
    synthPanel_.setVisible(hat);
    outputPanel_.setVisible(hat);
    envelopePanel_.setVisible(page == Page::envelope && !browser);
    performancePanel_.setVisible(page == Page::performance && !browser);
    effectsPanel_.setVisible(page == Page::effects && !browser);
    presetBrowser_.toFront(false);
    layoutContent();
}

void OpenHatterEditor::refreshHistoryButtons() {
    undoBtn_.setEnabled(processor_.canUndo());
    redoBtn_.setEnabled(processor_.canRedo());
    slotABtn_.setToggleState(processor_.abSlot() == 0, juce::dontSendNotification);
    slotBBtn_.setToggleState(processor_.abSlot() == 1, juce::dontSendNotification);
}

void OpenHatterEditor::timerCallback() {
    syncPresetBox();
    processor_.serviceLockIn();
    refreshHistoryButtons();
    const bool locked = processor_.parameters.getRawParameterValue(openhatter::params::kLockIn)->load() > 0.5f;
    const auto lockText = locked ? juce::String("Locked") : juce::String("Lock in");
    if (lockBtn_.getButtonText() != lockText)
        lockBtn_.setButtonText(lockText);
    const auto size = processor_.editorSize();
    if (size.x > 0 && (size.x != getWidth() || size.y != getHeight()))
        setSize(size.x, size.y);
}

bool OpenHatterEditor::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::escapeKey && presetBrowser_.isVisible()) {
        showPresetBrowser(false);
        return true;
    }
    const auto code = juce::CharacterFunctions::toUpperCase(static_cast<juce::juce_wchar>(key.getKeyCode()));
    if (key.getModifiers().isCommandDown() && code == 'Z') {
        if (key.getModifiers().isShiftDown())
            processor_.redo();
        else
            processor_.undo();
        refreshHistoryButtons();
        return true;
    }
    return juce::AudioProcessorEditor::keyPressed(key);
}

void OpenHatterEditor::paint(juce::Graphics& g) {
    g.fillAll(theme::kBackground);
    g.addTransform(content_.getTransform());
    g.setColour(theme::kPanel);
    g.fillRect(0, 0, content_.getWidth(), 136);
    g.setColour(theme::kAccentBrass);
    for (int i = 0; i < 3; ++i)
        g.drawEllipse(22.0f, 27.0f + static_cast<float>(i) * 6.0f, 39.0f, 14.0f, 1.8f);
    g.setColour(theme::kText);
    g.setFont(juce::FontOptions(31.0f, juce::Font::bold));
    g.drawText("openHatter", 72, 13, 178, 38, juce::Justification::left);
    g.setColour(theme::kTextDim);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText("v" JucePlugin_VersionString, 250, 22, 56, 22, juce::Justification::left);
    g.setFont(juce::FontOptions(14.0f));
    g.drawText("Bronze & air hi-hat synth", 74, 49, 210, 22, juce::Justification::left);
    g.drawText("Factory preset", 309, 8, 220, 19, juce::Justification::left);
}

void OpenHatterEditor::resized() {
    const float scale = std::min(static_cast<float>(getWidth()) / static_cast<float>(content_.getWidth()),
                                 static_cast<float>(getHeight()) / static_cast<float>(content_.getHeight()));
    const float left = (static_cast<float>(getWidth()) - static_cast<float>(content_.getWidth()) * scale) * 0.5f;
    const float top = (static_cast<float>(getHeight()) - static_cast<float>(content_.getHeight()) * scale) * 0.5f;
    content_.setTransform(juce::AffineTransform::scale(scale).translated(left, top));
    processor_.setEditorSize(getWidth(), getHeight());
    juce::AudioProcessorEditor::resized();
}

void OpenHatterEditor::layoutContent() {
    const int width = content_.getWidth();
    presetBox_.setBounds(308, 33, 350, 38);
    prevPresetBtn_.setBounds(667, 33, 82, 38);
    nextPresetBtn_.setBounds(753, 33, 65, 38);
    auditionBtn_.setBounds(width - 155, 25, 133, 43);

    auto suite = juce::Rectangle<int>(22, 92, std::max(0, width - 44), 38);
    juce::Component* buttons[] = {
        &undoBtn_, &redoBtn_, &slotABtn_, &slotBBtn_, &copyBtn_, &lockBtn_, &dragBtn_, &presetsBtn_
    };
    constexpr int count = 8;
    constexpr int gap = 6;
    const int available = suite.getWidth() - gap * (count - 1);
    const int buttonW = std::max(44, std::min(150, available / count));
    const int used = buttonW * count + gap * (count - 1);
    int x = suite.getX() + std::max(0, (suite.getWidth() - used) / 2);
    for (auto* button : buttons) {
        button->setBounds(x, suite.getY(), buttonW, suite.getHeight());
        x += buttonW + gap;
    }

    hatTab_.setBounds(22, 140, 83, 38);
    envelopeTab_.setBounds(109, 140, 112, 38);
    performanceTab_.setBounds(225, 140, 130, 38);
    effectsTab_.setBounds(359, 140, 92, 38);
    detailBtn_.setBounds(width - 177, 140, 155, 38);
    auto body = content_.getLocalBounds().reduced(22, 0).withTrimmedTop(186).withTrimmedBottom(20);
    presetBrowser_.setBounds(body);
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
