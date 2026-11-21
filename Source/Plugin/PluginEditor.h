#pragma once

#include "EdmLookAndFeel.h"
#include "Panels.h"
#include "PluginProcessor.h"
#include "PresetBrowser.h"
#include "PresetLibrary.h"
#include "WavDragButton.h"
#include <memory>

class OpenHatterEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit OpenHatterEditor(OpenHatterProcessor& processorToEdit);
    ~OpenHatterEditor() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    enum class Page { hat, envelope, performance, effects };
    void setPage(Page page);
    void layoutContent();
    void timerCallback() override;
    void syncPresetBox();
    void refreshHistoryButtons();
    void showPresetBrowser(bool show);

    OpenHatterProcessor& processor_;
    openhatter::ui::EdmLookAndFeel lookAndFeel_;
    // Holds every control at the design size. The editor scales it as one unit.
    juce::Component content_;
    suite::PresetLibrary presetLibrary_;
    suite::PresetBrowser presetBrowser_;
    juce::ComboBox presetBox_;
    juce::TextButton prevPresetBtn_{ "Previous" }, nextPresetBtn_{ "Next" }, auditionBtn_{ "Preview hat" };
    juce::TextButton hatTab_{ "Hat" }, envelopeTab_{ "Envelope" }, performanceTab_{ "Performance" }, effectsTab_{ "Effects" };
    juce::TextButton detailBtn_{ "Detailed controls" };
    juce::TextButton undoBtn_{ "Undo" }, redoBtn_{ "Redo" }, slotABtn_{ "A" }, slotBBtn_{ "B" }, copyBtn_{ "Copy" };
    juce::TextButton lockBtn_{ "Lock in" }, presetsBtn_{ "Presets" };
    suite::WavDragButton dragBtn_;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lockAttachment_;
    openhatter::ui::MidiMonitorPanel midiPanel_;
    openhatter::ui::HatSynthPanel synthPanel_;
    openhatter::ui::OutputPanel outputPanel_;
    openhatter::ui::EnvelopePanel envelopePanel_;
    openhatter::ui::PerformancePanel performancePanel_;
    openhatter::ui::EffectsPanel effectsPanel_;
    juce::TooltipWindow tooltipWindow_;
    Page page_ = Page::hat;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenHatterEditor)
};
