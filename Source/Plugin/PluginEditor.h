#pragma once

#include "EdmLookAndFeel.h"
#include "Panels.h"
#include "PluginProcessor.h"

class OpenHatterEditor : public juce::AudioProcessorEditor,
                         private juce::Timer {
public:
    explicit OpenHatterEditor(OpenHatterProcessor& processorToEdit);
    ~OpenHatterEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void setAdvancedView(bool) {}
    void setPage(int) {}
    [[nodiscard]] int getPage() const noexcept { return 0; }
    [[nodiscard]] bool isAdvancedView() const noexcept { return false; }

private:
    void timerCallback() override;
    void syncPresetBox();

    OpenHatterProcessor& processor_;
    openhatter::ui::EdmLookAndFeel lookAndFeel_;
    juce::ComboBox presetBox_;
    juce::TextButton prevPresetBtn_{ "<" };
    juce::TextButton nextPresetBtn_{ ">" };
    juce::TextButton auditionBtn_{ "AUDITION" };
    openhatter::ui::MidiMonitorPanel midiPanel_;
    openhatter::ui::HatSynthPanel synthPanel_;
    openhatter::ui::EffectsPanel effectsPanel_;
    juce::TooltipWindow tooltipWindow_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenHatterEditor)
};
