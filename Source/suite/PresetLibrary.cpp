#include "PresetLibrary.h"
#include <algorithm>

namespace suite {

namespace {

constexpr const char* kFavoritesFile = "favorites.txt";

[[nodiscard]] bool matches(const juce::String& name, const juce::String& query) {
    return query.isEmpty() || name.containsIgnoreCase(query);
}

} // namespace

PresetLibrary::PresetLibrary(juce::File directory) : directory_(std::move(directory)) {}

juce::File PresetLibrary::defaultDirectory(const juce::String& pluginName) {
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("Alan Marcero")
        .getChildFile(pluginName)
        .getChildFile("Presets");
}

void PresetLibrary::setFactoryNames(std::vector<juce::String> names) {
    factoryNames_ = std::move(names);
}

bool PresetLibrary::validName(const juce::String& name) {
    const auto trimmed = name.trim();
    return trimmed.isNotEmpty() && trimmed == name
        && !name.containsAnyOf("\\/:*?\"<>|") && name != "." && name != "..";
}

juce::File PresetLibrary::fileFor(const juce::String& name) const {
    return directory_.getChildFile(name + ".xml");
}

bool PresetLibrary::isFactoryName(const juce::String& name) const {
    return std::any_of(factoryNames_.begin(), factoryNames_.end(), [&](const juce::String& factory) {
        return factory.equalsIgnoreCase(name);
    });
}

void PresetLibrary::loadFavorites() const {
    if (favoritesLoaded_)
        return;
    favoritesLoaded_ = true;
    favorites_.clear();
    const auto file = directory_.getChildFile(kFavoritesFile);
    if (!file.existsAsFile())
        return;
    for (auto line : juce::StringArray::fromLines(file.loadFileAsString())) {
        line = line.trim();
        if (line.isNotEmpty())
            favorites_.push_back(line);
    }
}

bool PresetLibrary::writeFavorites(juce::String& error) const {
    if (!directory_.createDirectory()) {
        error = "Couldn't create the presets folder.";
        return false;
    }
    juce::String text;
    for (const auto& name : favorites_)
        text << name << '\n';
    if (!directory_.getChildFile(kFavoritesFile).replaceWithText(text)) {
        error = "Couldn't save favorites.";
        return false;
    }
    return true;
}

bool PresetLibrary::isFavorite(const juce::String& name) const {
    loadFavorites();
    return std::any_of(favorites_.begin(), favorites_.end(), [&](const juce::String& favorite) {
        return favorite == name;
    });
}

std::vector<PresetEntry> PresetLibrary::entries(const juce::String& query, bool favoritesOnly) const {
    loadFavorites();
    std::vector<PresetEntry> result;
    for (size_t i = 0; i < factoryNames_.size(); ++i) {
        const auto& name = factoryNames_[i];
        const bool favorite = isFavorite(name);
        if (!matches(name, query) || (favoritesOnly && !favorite))
            continue;
        result.push_back({ name, true, static_cast<int>(i), favorite });
    }
    std::vector<PresetEntry> users;
    if (directory_.isDirectory()) {
        for (const auto& file : directory_.findChildFiles(juce::File::findFiles, false, "*.xml")) {
            const auto name = file.getFileNameWithoutExtension();
            if (!validName(name) || isFactoryName(name))
                continue;
            const bool favorite = isFavorite(name);
            if (!matches(name, query) || (favoritesOnly && !favorite))
                continue;
            users.push_back({ name, false, -1, favorite });
        }
    }
    std::sort(users.begin(), users.end(), [](const PresetEntry& a, const PresetEntry& b) {
        return a.name.compareIgnoreCase(b.name) < 0;
    });
    result.insert(result.end(), users.begin(), users.end());
    return result;
}

bool PresetLibrary::saveUser(const juce::String& name, const juce::ValueTree& state, juce::String& error) {
    if (isFactoryName(name)) {
        error = "Factory presets stay read-only.";
        return false;
    }
    if (!validName(name)) {
        error = "Use a name without slashes or other path characters.";
        return false;
    }
    if (!state.isValid()) {
        error = "Nothing to save.";
        return false;
    }
    if (!directory_.createDirectory()) {
        error = "Couldn't create the presets folder.";
        return false;
    }
    const auto xml = state.createXml();
    if (xml == nullptr || !xml->writeTo(fileFor(name))) {
        error = "Couldn't write the preset file.";
        return false;
    }
    error.clear();
    return true;
}

bool PresetLibrary::renameUser(const juce::String& from, const juce::String& to, juce::String& error) {
    if (isFactoryName(from) || isFactoryName(to)) {
        error = "Factory presets stay read-only.";
        return false;
    }
    if (!validName(to)) {
        error = "Use a name without slashes or other path characters.";
        return false;
    }
    const auto source = fileFor(from);
    if (!source.existsAsFile()) {
        error = "That user preset is not on disk.";
        return false;
    }
    if (from != to && fileFor(to).existsAsFile()) {
        error = "A user preset with that name already exists.";
        return false;
    }
    if (from != to && !source.moveFileTo(fileFor(to))) {
        error = "Couldn't rename the preset file.";
        return false;
    }
    loadFavorites();
    bool changed = false;
    for (auto& favorite : favorites_) {
        if (favorite == from) {
            favorite = to;
            changed = true;
        }
    }
    if (changed && !writeFavorites(error))
        return false;
    error.clear();
    return true;
}

bool PresetLibrary::deleteUser(const juce::String& name, juce::String& error) {
    if (isFactoryName(name)) {
        error = "Factory presets stay read-only.";
        return false;
    }
    const auto file = fileFor(name);
    if (!file.existsAsFile()) {
        error = "That user preset is not on disk.";
        return false;
    }
    if (!file.deleteFile()) {
        error = "Couldn't delete the preset file.";
        return false;
    }
    loadFavorites();
    const auto before = favorites_.size();
    favorites_.erase(std::remove(favorites_.begin(), favorites_.end(), name), favorites_.end());
    if (favorites_.size() != before && !writeFavorites(error))
        return false;
    error.clear();
    return true;
}

bool PresetLibrary::setFavorite(const juce::String& name, bool favorite, juce::String& error) {
    if (name.isEmpty() || name.containsAnyOf("\r\n")) {
        error = "That preset name can't be marked.";
        return false;
    }
    loadFavorites();
    const auto it = std::find(favorites_.begin(), favorites_.end(), name);
    if (favorite && it == favorites_.end())
        favorites_.push_back(name);
    if (!favorite && it != favorites_.end())
        favorites_.erase(it);
    return writeFavorites(error);
}

juce::ValueTree PresetLibrary::readUser(const juce::String& name) const {
    if (isFactoryName(name))
        return {};
    const auto file = fileFor(name);
    if (!file.existsAsFile())
        return {};
    const auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return {};
    return juce::ValueTree::fromXml(*xml);
}

} // namespace suite
