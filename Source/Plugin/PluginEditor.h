#pragma once

#include "EdmLookAndFeel.h"
#include "Panels.h"
#include "PluginProcessor.h"
#include "PresetBrowser.h"
#include "PresetLibrary.h"
#include "SuiteBar.h"
#include <memory>

class OpenHatterEditor : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit OpenHatterEditor(OpenHatterProcessor& processorToEdit);
    ~OpenHatterEditor() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    [[nodiscard]] bool canLockParameterInUi(const juce::String& paramId) const;
    [[nodiscard]] juce::PopupMenu buildLockMenuForParam(const juce::String& paramId);

private:
    class LockOverlay final : public juce::Component, public juce::SettableTooltipClient {
    public:
        LockOverlay(OpenHatterProcessor& processor, juce::Component& root);
        void paint(juce::Graphics& g) override;
        bool hitTest(int x, int y) override;
        void mouseMove(const juce::MouseEvent& event) override;
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        struct Badge {
            juce::Rectangle<float> bounds;
            juce::String paramId;
            juce::Colour colour;
        };
        [[nodiscard]] std::vector<Badge> collectBadges() const;

        OpenHatterProcessor& processor_;
        juce::Component& root_;
    };

    class LockMouseListener final : public juce::MouseListener {
    public:
        LockMouseListener(OpenHatterProcessor& processor, juce::Component& root, LockOverlay& overlay);
        void mouseDown(const juce::MouseEvent& event) override;

    private:
        OpenHatterProcessor& processor_;
        juce::Component& root_;
        LockOverlay& overlay_;
    };

    enum class Page { hat, envelope, performance, effects };
    void setPage(Page page);
    void layoutContent();
    void timerCallback() override;
    void syncPresetBox();
    void stepPreset(int delta);
    void refreshHistoryButtons();
    void showPresetBrowser(bool show);

    OpenHatterProcessor& processor_;
    openhatter::ui::EdmLookAndFeel lookAndFeel_;
    // The editor scales this component, which holds every control at design size.
    juce::Component content_;
    suite::PresetLibrary presetLibrary_;
    suite::PresetBrowser presetBrowser_;
    juce::ComboBox presetBox_;
    juce::TextButton prevPresetBtn_{ "<" }, nextPresetBtn_{ ">" }, auditionBtn_{ "Preview hat" };
    juce::TextButton hatTab_{ "Hat" }, envelopeTab_{ "Envelope" }, performanceTab_{ "Performance" }, effectsTab_{ "Effects" };
    juce::TextButton detailBtn_{ "Detailed controls" };
    suite::SuiteBar suiteBar_;
    juce::TextButton lockBtn_{ "Lock in" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> lockAttachment_;
    openhatter::ui::MidiMonitorPanel midiPanel_;
    openhatter::ui::HatSynthPanel synthPanel_;
    openhatter::ui::OutputPanel outputPanel_;
    openhatter::ui::EnvelopePanel envelopePanel_;
    openhatter::ui::PerformancePanel performancePanel_;
    openhatter::ui::EffectsPanel effectsPanel_;
    LockOverlay lockOverlay_;
    LockMouseListener lockMouseListener_;
    juce::TooltipWindow tooltipWindow_;
    Page page_ = Page::hat;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OpenHatterEditor)
};
