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

[[nodiscard]] juce::String directParamId(const juce::Component& component,
                                         const juce::AudioProcessorValueTreeState& apvts) {
    const auto& props = component.getProperties();
    if (props.contains("lockParamId")) {
        const auto id = props["lockParamId"].toString();
        if (apvts.getParameter(id) != nullptr)
            return id;
    }
    const auto id = component.getComponentID();
    if (id.isNotEmpty() && apvts.getParameter(id) != nullptr)
        return id;
    return {};
}

[[nodiscard]] std::vector<juce::String> resolveLockTargets(juce::Component* target,
                                                           juce::Component& root,
                                                           const juce::AudioProcessorValueTreeState& apvts) {
    for (auto* current = target; current != nullptr && current != &root; current = current->getParentComponent()) {
        if (auto* eq = dynamic_cast<openhatter::ui::DualEqKnob*>(current))
            return { eq->dbId(), eq->freqId() };
        if (const auto id = directParamId(*current, apvts); id.isNotEmpty())
            return { id };
        if (auto* knob = dynamic_cast<openhatter::ui::Knob*>(current))
            return { knob->slider().getComponentID() };
        if (auto* choice = dynamic_cast<openhatter::ui::Choice*>(current))
            return { choice->box().getComponentID() };
        if (auto* toggle = dynamic_cast<openhatter::ui::Toggle*>(current))
            return { toggle->button().getComponentID() };
    }
    return {};
}

struct LockMenuSpec {
    juce::PopupMenu menu;
    std::vector<juce::String> itemParams;
    int unlockAllId = 0;
};

[[nodiscard]] LockMenuSpec buildLockMenuSpec(const std::vector<juce::String>& targets,
                                             OpenHatterProcessor& processor) {
    LockMenuSpec spec;
    for (const auto& paramId : targets) {
        auto* param = processor.parameters.getParameter(paramId);
        if (param == nullptr)
            continue;
        const bool locked = processor.isParamLocked(paramId);
        spec.itemParams.push_back(paramId);
        spec.menu.addItem(static_cast<int>(spec.itemParams.size()),
                          (locked ? "Unlock " : "Lock ") + param->getName(64));
    }
    if (!spec.itemParams.empty() && processor.hasAnyLock()) {
        spec.unlockAllId = static_cast<int>(spec.itemParams.size()) + 1;
        spec.menu.addSeparator();
        spec.menu.addItem(spec.unlockAllId, "Unlock all");
    }
    return spec;
}

void drawPadlock(juce::Graphics& g, juce::Rectangle<float> box, juce::Colour colour = theme::kAccentBrass) {
    const float w = box.getWidth();
    const float h = box.getHeight();
    const float cx = box.getCentreX();
    const juce::Rectangle<float> body(cx - w * 0.34f, box.getY() + h * 0.44f, w * 0.68f, h * 0.48f);
    const float shackleR = w * 0.22f;
    juce::Path shackle;
    shackle.addArc(cx - shackleR, box.getY() + h * 0.14f, shackleR * 2.0f, shackleR * 2.0f,
                   juce::MathConstants<float>::pi * 1.0f, juce::MathConstants<float>::pi * 2.0f, true);
    g.setColour(theme::kBackground.withAlpha(0.88f));
    g.fillRoundedRectangle(box, 3.0f);
    g.setColour(colour);
    g.strokePath(shackle, juce::PathStrokeType(1.6f));
    g.fillRoundedRectangle(body, 2.0f);
    g.setColour(theme::kBackground);
    g.fillEllipse(cx - 1.3f, body.getCentreY() - 1.3f, 2.6f, 2.6f);
}

[[nodiscard]] bool hasParamInTree(const juce::Component& parent,
                                  const juce::String& paramId,
                                  const juce::AudioProcessorValueTreeState& apvts) {
    if (auto* eq = dynamic_cast<const openhatter::ui::DualEqKnob*>(&parent))
        return eq->dbId() == paramId || eq->freqId() == paramId;
    if (directParamId(parent, apvts) == paramId)
        return true;
    for (auto* child : parent.getChildren()) {
        if (hasParamInTree(*child, paramId, apvts))
            return true;
    }
    return false;
}

} // namespace

OpenHatterEditor::LockOverlay::LockOverlay(OpenHatterProcessor& processor, juce::Component& root)
    : processor_(processor), root_(root) {
    setInterceptsMouseClicks(true, false);
}

std::vector<OpenHatterEditor::LockOverlay::Badge> OpenHatterEditor::LockOverlay::collectBadges() const {
    std::vector<Badge> badges;
    const auto walk = [&](auto&& self, juce::Component& parent) -> void {
        for (auto* child : parent.getChildren()) {
            if (child == this || !child->isVisible())
                continue;
            if (auto* eq = dynamic_cast<openhatter::ui::DualEqKnob*>(child)) {
                if (processor_.isParamLocked(eq->dbId())) {
                    const auto pill = getLocalArea(eq, eq->dbPillBounds()).toFloat();
                    badges.push_back({ { pill.getRight() - 11.0f, pill.getY() - 3.0f, 12.0f, 12.0f },
                                       eq->dbId(), theme::kAccentBrass });
                }
                if (processor_.isParamLocked(eq->freqId())) {
                    const auto pill = getLocalArea(eq, eq->freqPillBounds()).toFloat();
                    badges.push_back({ { pill.getRight() - 11.0f, pill.getY() - 3.0f, 12.0f, 12.0f },
                                       eq->freqId(), theme::kAccentAqua });
                }
                continue;
            }
            if (const auto id = directParamId(*child, processor_.parameters); id.isNotEmpty()) {
                if (processor_.isParamLocked(id)) {
                    auto* host = child;
                    if (auto* parentKnob = dynamic_cast<openhatter::ui::Knob*>(child->getParentComponent()))
                        host = parentKnob;
                    else if (auto* parentChoice = dynamic_cast<openhatter::ui::Choice*>(child->getParentComponent()))
                        host = parentChoice;
                    const auto area = getLocalArea(host, host->getLocalBounds()).toFloat();
                    juce::Rectangle<float> box{ area.getRight() - 16.0f, area.getY() + 2.0f, 14.0f, 14.0f };
                    if (dynamic_cast<juce::ComboBox*>(host) != nullptr)
                        box.setX(area.getRight() - 30.0f);
                    else if (dynamic_cast<juce::Button*>(host) != nullptr)
                        box = { area.getRight() - 19.0f, area.getY() + 4.0f, 14.0f, 14.0f };
                    badges.push_back({ box, id, theme::kAccentBrass });
                }
                continue;
            }
            self(self, *child);
        }
    };
    walk(walk, root_);
    return badges;
}

void OpenHatterEditor::LockOverlay::paint(juce::Graphics& g) {
    for (const auto& badge : collectBadges())
        drawPadlock(g, badge.bounds, badge.colour);
}

bool OpenHatterEditor::LockOverlay::hitTest(int x, int y) {
    const auto pos = juce::Point<int>(x, y).toFloat();
    for (const auto& badge : collectBadges()) {
        if (badge.bounds.expanded(2.0f).contains(pos))
            return true;
    }
    return false;
}

void OpenHatterEditor::LockOverlay::mouseMove(const juce::MouseEvent& event) {
    for (const auto& badge : collectBadges()) {
        if (badge.bounds.expanded(2.0f).contains(event.position)) {
            if (auto* param = processor_.parameters.getParameter(badge.paramId))
                setTooltip("Locked: " + param->getName(64) + " (click to unlock)");
            return;
        }
    }
    setTooltip({});
}

void OpenHatterEditor::LockOverlay::mouseDown(const juce::MouseEvent& event) {
    for (const auto& badge : collectBadges()) {
        if (!badge.bounds.expanded(2.0f).contains(event.position))
            continue;
        processor_.setParamLocked(badge.paramId, false);
        setTooltip({});
        repaint();
        return;
    }
}

OpenHatterEditor::LockMouseListener::LockMouseListener(OpenHatterProcessor& processor,
                                                       juce::Component& root,
                                                       LockOverlay& overlay)
    : processor_(processor), root_(root), overlay_(overlay) {}

void OpenHatterEditor::LockMouseListener::mouseDown(const juce::MouseEvent& event) {
    if (!event.mods.isPopupMenu() || event.eventComponent == nullptr)
        return;
    const auto targets = resolveLockTargets(event.eventComponent, root_, processor_.parameters);
    if (targets.empty())
        return;

    auto spec = buildLockMenuSpec(targets, processor_);
    if (spec.itemParams.empty())
        return;

    spec.menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(event.eventComponent),
                            [this, itemParams = spec.itemParams, unlockAllId = spec.unlockAllId](int result) {
        if (result <= 0)
            return;
        if (result == unlockAllId)
            processor_.unlockAllLocks();
        else if (static_cast<size_t>(result) <= itemParams.size()) {
            const auto& id = itemParams[static_cast<size_t>(result - 1)];
            processor_.setParamLocked(id, !processor_.isParamLocked(id));
        }
        overlay_.repaint();
    });
}

OpenHatterEditor::OpenHatterEditor(OpenHatterProcessor& processorToEdit)
    : AudioProcessorEditor(&processorToEdit), processor_(processorToEdit),
      presetLibrary_(suite::PresetLibrary::defaultDirectory("openHatter")),
      presetBrowser_(presetLibrary_, editorStyle()),
      dragBtn_([this] { return processor_.exportHatWav(); }),
      midiPanel_(processorToEdit), synthPanel_(processorToEdit), outputPanel_(processorToEdit),
      envelopePanel_(processorToEdit), performancePanel_(processorToEdit), effectsPanel_(processorToEdit),
      lockOverlay_(processorToEdit, content_),
      lockMouseListener_(processorToEdit, content_, lockOverlay_),
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
    presetBox_.setName("Preset");
    presetBox_.setTitle("Preset");
    prevPresetBtn_.setName("Previous");
    nextPresetBtn_.setName("Next");
    prevPresetBtn_.setTitle("Previous preset");
    nextPresetBtn_.setTitle("Next preset");
    slotABtn_.setTitle("Slot A");
    slotBBtn_.setTitle("Slot B");
    dragBtn_.setTitle("Drag WAV");
    presetBox_.setTooltip("Choose a factory hat. The box names the loaded factory or user preset, with * after an edit.");
    prevPresetBtn_.setTooltip("Load the previous preset.");
    nextPresetBtn_.setTooltip("Load the next preset.");
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
        presetBrowser_.disarm();
        if (presetBox_.getSelectedId() > 0)
            processor_.setCurrentProgram(presetBox_.getSelectedId() - 1);
    };
    prevPresetBtn_.onClick = [this] { stepPreset(-1); };
    nextPresetBtn_.onClick = [this] { stepPreset(1); };
    auditionBtn_.setTooltip("Play a one-shot on the last MIDI pitch, or middle C before the first note.");
    auditionBtn_.setColour(juce::TextButton::buttonColourId, theme::kAccentBrass);
    auditionBtn_.setColour(juce::TextButton::textColourOffId, theme::kBackground);
    auditionBtn_.onClick = [this] { processor_.triggerAudition(); };
    undoBtn_.onClick = [this] { processor_.undo(); refreshHistoryButtons(); };
    redoBtn_.onClick = [this] { processor_.redo(); refreshHistoryButtons(); };
    slotABtn_.onClick = [this] { presetBrowser_.disarm(); processor_.selectAbSlot(0); refreshHistoryButtons(); };
    slotBBtn_.onClick = [this] { presetBrowser_.disarm(); processor_.selectAbSlot(1); refreshHistoryButtons(); };
    copyBtn_.onClick = [this] { presetBrowser_.disarm(); processor_.copyAbToOther(); };
    presetsBtn_.onClick = [this] { showPresetBrowser(!presetBrowser_.isVisible()); };
    lockBtn_.setClickingTogglesState(true);
    lockBtn_.getProperties().set("lockParamId", openhatter::params::kLockIn);
    presetBox_.getProperties().set("lockParamId", openhatter::params::kPreset);
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
    presetBrowser_.setSaved([this](const juce::String& name) {
        processor_.setLoadedPreset(name);
        syncPresetBox();
    });

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
    content_.addAndMakeVisible(lockOverlay_);
    content_.addMouseListener(&lockMouseListener_, true);
    const auto restoredSize = processor_.editorSize();
    setPage(Page::hat);
    syncPresetBox();
    setResizeLimits(OpenHatterProcessor::kEditorMinWidth, OpenHatterProcessor::kEditorMinHeight,
                    OpenHatterProcessor::kEditorMaxWidth, OpenHatterProcessor::kEditorMaxHeight);
    getConstrainer()->setFixedAspectRatio(static_cast<double>(OpenHatterProcessor::kEditorWidth)
                                          / OpenHatterProcessor::kEditorHeight);
    setResizable(true, true);
    setSize(restoredSize.x, restoredSize.y);
    refreshHistoryButtons();
    startTimerHz(15);
}

OpenHatterEditor::~OpenHatterEditor() {
    stopTimer();
    content_.removeMouseListener(&lockMouseListener_);
    setLookAndFeel(nullptr);
}

bool OpenHatterEditor::canLockParameterInUi(const juce::String& paramId) const {
    return hasParamInTree(content_, paramId, processor_.parameters);
}

juce::PopupMenu OpenHatterEditor::buildLockMenuForParam(const juce::String& paramId) {
    return buildLockMenuSpec({ paramId }, processor_).menu;
}

void OpenHatterEditor::syncPresetBox() {
    const auto name = processor_.presetDisplayName();
    if (name.isEmpty()) {
        presetBox_.setSelectedId(0, juce::dontSendNotification);
        return;
    }
    if (presetBox_.getText() != name)
        presetBox_.setText(name, juce::dontSendNotification);
}

void OpenHatterEditor::stepPreset(int delta) {
    presetBrowser_.disarm();
    const auto& rows = presetBrowser_.rows();
    if (rows.empty()) {
        const int count = processor_.getNumPrograms();
        const int current = processor_.hasLegacySound() ? 0 : processor_.getCurrentProgram();
        processor_.setCurrentProgram((current + delta + count) % count);
        syncPresetBox();
        return;
    }
    const auto activeUser = processor_.activeUserPreset();
    const int currentFactory = processor_.getCurrentProgram();
    const bool legacy = processor_.hasLegacySound();
    int currentIdx = -1;
    for (size_t i = 0; i < rows.size(); ++i) {
        const auto& entry = rows[i];
        if (activeUser.isNotEmpty()) {
            if (!entry.factory && entry.name == activeUser) {
                currentIdx = static_cast<int>(i);
                break;
            }
        } else if (!legacy && entry.factory && entry.factoryIndex == currentFactory) {
            currentIdx = static_cast<int>(i);
            break;
        }
    }
    const int count = static_cast<int>(rows.size());
    const int nextIdx = currentIdx >= 0 ? (currentIdx + delta + count) % count : (delta > 0 ? 0 : count - 1);
    const auto& target = rows[static_cast<size_t>(nextIdx)];
    if (target.factory) {
        processor_.setCurrentProgram(target.factoryIndex);
    } else {
        const auto tree = presetLibrary_.readUser(target.name);
        if (tree.isValid())
            processor_.loadUserPreset(tree, target.name);
    }
    syncPresetBox();
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
    lockOverlay_.toFront(false);
    presetBrowser_.toFront(false);
    layoutContent();
}

void OpenHatterEditor::refreshHistoryButtons() {
    undoBtn_.setEnabled(processor_.canUndo());
    redoBtn_.setEnabled(processor_.canRedo());
    slotABtn_.setToggleState(processor_.abSlot() == 0, juce::dontSendNotification);
    slotBBtn_.setToggleState(processor_.abSlot() == 1, juce::dontSendNotification);
    copyBtn_.setTitle(processor_.abSlot() == 0 ? "Copy A to B" : "Copy B to A");
}

void OpenHatterEditor::timerCallback() {
    syncPresetBox();
    processor_.serviceLockIn();
    refreshHistoryButtons();
    const bool locked = processor_.parameters.getRawParameterValue(openhatter::params::kLockIn)->load() > 0.5f;
    const auto lockText = locked ? juce::String("Locked") : juce::String("Lock in");
    if (lockBtn_.getButtonText() != lockText)
        lockBtn_.setButtonText(lockText);
    // A host can force another aspect ratio. Resize only when the stored size changed, as after a session load.
    const auto size = processor_.editorSize();
    if (size != OpenHatterProcessor::fitEditorSize(getWidth(), getHeight()))
        setSize(size.x, size.y);
}

bool OpenHatterEditor::keyPressed(const juce::KeyPress& key) {
    if (key == juce::KeyPress::escapeKey) {
        if (performancePanel_.cancelActiveLearn())
            return true;
        if (presetBrowser_.isVisible()) {
            showPresetBrowser(false);
            return true;
        }
    }
    const auto code = juce::CharacterFunctions::toUpperCase(static_cast<juce::juce_wchar>(key.getKeyCode()));
    if (key.getModifiers().isCommandDown() && code == 'L') {
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent()) {
            const auto targets = resolveLockTargets(focused, content_, processor_.parameters);
            if (!targets.empty()) {
                for (const auto& id : targets)
                    processor_.setParamLocked(id, !processor_.isParamLocked(id));
                lockOverlay_.repaint();
                return true;
            }
        }
    }
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
    g.setColour(theme::kText.withAlpha(0.92f));
    g.setFont(juce::FontOptions(15.5f, juce::Font::bold));
    g.drawText("Preset", 309, 7, 220, 22, juce::Justification::left);
}

void OpenHatterEditor::resized() {
    content_.setTransform(juce::RectanglePlacement().getTransformToFit(content_.getLocalBounds().toFloat(),
                                                                       getLocalBounds().toFloat()));
    processor_.setEditorSize(getWidth(), getHeight());
    juce::AudioProcessorEditor::resized();
}

void OpenHatterEditor::layoutContent() {
    const int width = content_.getWidth();
    presetBox_.setBounds(308, 33, 422, 38);
    prevPresetBtn_.setBounds(736, 32, 40, 40);
    nextPresetBtn_.setBounds(780, 32, 40, 40);
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
    lockOverlay_.setBounds(content_.getLocalBounds());
}
