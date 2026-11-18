#pragma once

#include "EdmLookAndFeel.h"
#include "Panels.h"
#include "PluginProcessor.h"

class OpenHatterEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit OpenHatterEditor(OpenHatterProcessor& processorToEdit);
    ~OpenHatterEditor() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    enum class Page { hat, envelope, performance, effects };
    void setPage(Page page);
    void timerCallback() override;
    void syncPresetBox();

    OpenHatterProcessor& processor_;
    openhatter::ui::EdmLookAndFeel lookAndFeel_;
    juce::ComboBox presetBox_;
    juce::TextButton prevPresetBtn_{ "Previous" }, nextPresetBtn_{ "Next" }, auditionBtn_{ "Preview hat" };
    juce::TextButton hatTab_{ "Hat" }, envelopeTab_{ "Envelope" }, performanceTab_{ "Performance" }, effectsTab_{ "Effects" };
    juce::TextButton detailBtn_{ "Detailed controls" };
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
