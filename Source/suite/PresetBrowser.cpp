#include "PresetBrowser.h"
#include <algorithm>

namespace suite {

namespace {

void styleField(juce::TextEditor& field, const Style& style, const juce::String& name, const juce::String& tip) {
    field.setName(name);
    field.setTooltip(tip);
    field.setMultiLine(false);
    field.setReturnKeyStartsNewLine(false);
    field.setColour(juce::TextEditor::backgroundColourId, style.background);
    field.setColour(juce::TextEditor::textColourId, style.text);
    field.setColour(juce::TextEditor::outlineColourId, style.edge);
    field.setColour(juce::TextEditor::focusedOutlineColourId, style.accent);
    field.setFont(juce::FontOptions(15.0f));
    field.setIndents(8, 4);
}

} // namespace

PresetBrowser::PresetBrowser(PresetLibrary& library, Style style)
    : library_(library), style_(style) {
    styleField(search_, style_, "Preset search", "Filter factory and user presets by name.");
    search_.setTextToShowWhenEmpty("Search presets", style_.textDim);
    search_.onTextChange = [this] { applySearch(); };
    styleField(name_, style_, "Preset name", "Name used when saving or renaming a user preset.");
    name_.setTextToShowWhenEmpty("Preset name", style_.textDim);
    const auto dismiss = [this] {
        if (dismiss_)
            dismiss_();
    };
    search_.onEscapeKey = dismiss;
    name_.onEscapeKey = dismiss;

    for (auto* button : { &favoritesBtn_, &saveBtn_, &renameBtn_, &deleteBtn_ }) {
        button->setColour(juce::TextButton::buttonOnColourId, style_.accent);
        addAndMakeVisible(button);
    }
    favoritesBtn_.setClickingTogglesState(true);
    favoritesBtn_.setTooltip("Show only starred presets.");
    favoritesBtn_.onClick = [this] {
        favoritesOnly_ = favoritesBtn_.getToggleState();
        applySearch();
    };
    saveBtn_.setTooltip("Save the current sound as a user preset. Factory names stay read-only.");
    saveBtn_.onClick = [this] {
        if (captureState_ == nullptr)
            return;
        juce::String error;
        if (!library_.saveUser(name_.getText().trim(), captureState_(), error))
            status_.setText(error, juce::dontSendNotification);
        else
            status_.setText("Saved " + name_.getText().trim(), juce::dontSendNotification);
        refresh();
    };
    renameBtn_.setTooltip("Rename the selected user preset.");
    renameBtn_.onClick = [this] {
        const int row = list_.getSelectedRow();
        if (!juce::isPositiveAndBelow(row, static_cast<int>(rows_.size())) || rows_[static_cast<size_t>(row)].factory)
            return;
        juce::String error;
        const auto from = rows_[static_cast<size_t>(row)].name;
        const auto to = name_.getText().trim();
        if (!library_.renameUser(from, to, error))
            status_.setText(error, juce::dontSendNotification);
        else
            status_.setText("Renamed to " + to, juce::dontSendNotification);
        refresh();
    };
    deleteBtn_.setTooltip("Delete the selected user preset.");
    deleteBtn_.onClick = [this] {
        const int row = list_.getSelectedRow();
        if (!juce::isPositiveAndBelow(row, static_cast<int>(rows_.size())) || rows_[static_cast<size_t>(row)].factory)
            return;
        juce::String error;
        if (!library_.deleteUser(rows_[static_cast<size_t>(row)].name, error))
            status_.setText(error, juce::dontSendNotification);
        else {
            status_.setText("Deleted preset.", juce::dontSendNotification);
            name_.clear();
        }
        refresh();
    };

    list_.setName("Preset list");
    list_.setColour(juce::ListBox::backgroundColourId, style_.panel);
    list_.setColour(juce::ListBox::outlineColourId, style_.edge);
    list_.setOutlineThickness(1);
    list_.setRowHeight(38);
    status_.setColour(juce::Label::textColourId, style_.textDim);
    status_.setFont(juce::FontOptions(14.0f));
    status_.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(search_);
    addAndMakeVisible(name_);
    addAndMakeVisible(list_);
    addAndMakeVisible(status_);
    refresh();
}

PresetBrowser::~PresetBrowser() { list_.setModel(nullptr); }

void PresetBrowser::setLoadFactory(std::function<void(int)> callback) { loadFactory_ = std::move(callback); }
void PresetBrowser::setLoadUser(std::function<void(const juce::String&)> callback) { loadUser_ = std::move(callback); }
void PresetBrowser::setCaptureState(std::function<juce::ValueTree()> callback) { captureState_ = std::move(callback); }
void PresetBrowser::setDismiss(std::function<void()> callback) { dismiss_ = std::move(callback); }

void PresetBrowser::refresh() {
    const auto selectedName = juce::isPositiveAndBelow(list_.getSelectedRow(), static_cast<int>(rows_.size()))
        ? rows_[static_cast<size_t>(list_.getSelectedRow())].name
        : juce::String();
    rows_ = library_.entries(search_.getText().trim(), favoritesOnly_);
    list_.updateContent();
    const auto it = std::find_if(rows_.begin(), rows_.end(), [&](const PresetEntry& entry) {
        return entry.name == selectedName;
    });
    if (it != rows_.end())
        list_.selectRow(static_cast<int>(std::distance(rows_.begin(), it)), juce::dontSendNotification);
    updateActionButtons();
    repaint();
}

void PresetBrowser::visibilityChanged() {
    if (isVisible())
        refresh();
}

void PresetBrowser::paint(juce::Graphics& g) {
    g.setColour(style_.panel);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);
    g.setColour(style_.edge);
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 10.0f, 1.0f);
    g.setColour(style_.text);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("Presets", 16, 10, 180, 28, juce::Justification::centredLeft);
}

void PresetBrowser::resized() {
    auto area = getLocalBounds().reduced(14);
    area.removeFromTop(34);
    auto searchRow = area.removeFromTop(38);
    favoritesBtn_.setBounds(searchRow.removeFromRight(124));
    searchRow.removeFromRight(8);
    search_.setBounds(searchRow);
    area.removeFromTop(8);
    auto bottom = area.removeFromBottom(38);
    deleteBtn_.setBounds(bottom.removeFromRight(90));
    bottom.removeFromRight(8);
    renameBtn_.setBounds(bottom.removeFromRight(96));
    bottom.removeFromRight(8);
    saveBtn_.setBounds(bottom.removeFromRight(76));
    bottom.removeFromRight(8);
    name_.setBounds(bottom);
    area.removeFromBottom(6);
    status_.setBounds(area.removeFromBottom(22));
    area.removeFromBottom(6);
    list_.setBounds(area);
}

int PresetBrowser::getNumRows() { return static_cast<int>(rows_.size()); }

void PresetBrowser::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool selected) {
    if (!juce::isPositiveAndBelow(row, static_cast<int>(rows_.size())))
        return;
    const auto& entry = rows_[static_cast<size_t>(row)];
    g.setColour(selected ? style_.edge : juce::Colours::transparentBlack);
    g.fillRect(0, 0, width, height);
    const auto star = juce::Rectangle<float>(10.0f, (height - 18.0f) * 0.5f, 18.0f, 18.0f);
    juce::Path starPath;
    starPath.addStar(star.getCentre(), 5, 4.0f, 8.5f, 0.0f);
    g.setColour(entry.favorite ? style_.accentAlt : style_.textDim.withAlpha(0.7f));
    g.fillPath(starPath);
    g.setColour(style_.text);
    g.setFont(juce::FontOptions(15.0f));
    g.drawText(entry.name, 36, 0, width - 130, height, juce::Justification::centredLeft);
    g.setColour(style_.textDim);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.drawText(entry.factory ? "Factory" : "User", width - 88, 0, 76, height, juce::Justification::centredRight);
}

void PresetBrowser::listBoxItemClicked(int row, const juce::MouseEvent& event) {
    if (event.x <= 36)
        toggleFavorite(row);
    else
        loadRow(row);
}

void PresetBrowser::selectedRowsChanged(int) { updateActionButtons(); }

void PresetBrowser::applySearch() { refresh(); }

void PresetBrowser::loadRow(int row) {
    if (!juce::isPositiveAndBelow(row, static_cast<int>(rows_.size())))
        return;
    const auto& entry = rows_[static_cast<size_t>(row)];
    if (entry.factory) {
        if (loadFactory_)
            loadFactory_(entry.factoryIndex);
        name_.clear();
    } else {
        if (loadUser_)
            loadUser_(entry.name);
        name_.setText(entry.name, juce::dontSendNotification);
    }
    status_.setText("Loaded " + entry.name, juce::dontSendNotification);
    updateActionButtons();
}

void PresetBrowser::toggleFavorite(int row) {
    if (!juce::isPositiveAndBelow(row, static_cast<int>(rows_.size())))
        return;
    const auto& entry = rows_[static_cast<size_t>(row)];
    juce::String error;
    if (!library_.setFavorite(entry.name, !entry.favorite, error))
        status_.setText(error, juce::dontSendNotification);
    refresh();
}

void PresetBrowser::updateActionButtons() {
    const int row = list_.getSelectedRow();
    const bool user = juce::isPositiveAndBelow(row, static_cast<int>(rows_.size()))
        && !rows_[static_cast<size_t>(row)].factory;
    renameBtn_.setEnabled(user);
    deleteBtn_.setEnabled(user);
}

} // namespace suite
