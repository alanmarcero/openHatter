#pragma once

#include "EdmLookAndFeel.h"
#include "Panels.h"
#include "PluginProcessor.h"
#include <array>

class OpenHatterEditor : public juce::AudioProcessorEditor,
                         private juce::Timer {
public:
    explicit OpenHatterEditor(OpenHatterProcessor& processorToEdit);
    ~OpenHatterEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void setAdvancedView(bool advanced);
    void setPage(int page);
    [[nodiscard]] int getPage() const noexcept { return page_; }
    [[nodiscard]] bool isAdvancedView() const noexcept { return advanced_; }

private:
    void timerCallback() override;
    void syncPresetBox();

    OpenHatterProcessor& processor_;
    openhatter::ui::EdmLookAndFeel lookAndFeel_;
    juce::ComboBox presetBox_;
    juce::TextButton prevPresetBtn_{ "<" };
    juce::TextButton nextPresetBtn_{ ">" };
    juce::TextButton simpleAdvancedBtn_{ "SIMPLE" };
    std::array<juce::TextButton, 3> pageButtons_ { juce::TextButton("HAT"),
                                                   juce::TextButton("MOTION"),
                                                   juce::TextButton("FX") };

    openhatter::ui::HatterStagePanel stagePanel_;
    openhatter::ui::HatSynthPanel synthPanel_;
    openhatter::ui::EffectsPanel effectsPanel_;
    juce::TooltipWindow tooltipWindow_;
    int page_ = 0;
    bool advanced_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenHatterEditor)
};
