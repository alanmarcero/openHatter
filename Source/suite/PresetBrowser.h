#pragma once

#include "PresetLibrary.h"
#include "SuiteStyle.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace suite {

class PresetBrowser : public juce::Component, private juce::ListBoxModel, private juce::Timer {
public:
    PresetBrowser(PresetLibrary& library, Style style);
    ~PresetBrowser() override;

    void setLoadFactory(std::function<void(int index)> callback);
    void setLoadUser(std::function<void(const juce::String& name)> callback);
    void setCaptureState(std::function<juce::ValueTree()> callback);
    void setDismiss(std::function<void()> callback);
    void setSaved(std::function<void(const juce::String& name)> callback);
    void refresh();
    [[nodiscard]] int rowCount() const noexcept { return static_cast<int>(rows_.size()); }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void timerCallback() override;

    void applySearch();
    void loadRow(int row);
    void toggleFavorite(int row);
    void updateActionButtons();
    void disarm();

    PresetLibrary& library_;
    Style style_;
    std::function<void(int)> loadFactory_;
    std::function<void(const juce::String&)> loadUser_;
    std::function<juce::ValueTree()> captureState_;
    std::function<void()> dismiss_;
    std::function<void(const juce::String&)> saved_;
    std::vector<PresetEntry> rows_;
    bool favoritesOnly_ = false;
    // The button that waits for a second click, or null.
    juce::TextButton* armed_ = nullptr;

    juce::TextEditor search_;
    juce::TextEditor name_;
    juce::TextButton favoritesBtn_ { "Favorites" };
    juce::TextButton saveBtn_ { "Save" };
    juce::TextButton renameBtn_ { "Rename" };
    juce::TextButton deleteBtn_ { "Delete" };
    juce::ListBox list_ { "Preset list", this };
    juce::Label status_;
};

} // namespace suite
