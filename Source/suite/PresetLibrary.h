#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <vector>

namespace suite {

struct PresetEntry {
    juce::String name;
    bool factory = false;
    int factoryIndex = -1;
    bool favorite = false;
};

// Factory presets are read-only. User presets live as XML files in the library directory.
class PresetLibrary {
public:
    explicit PresetLibrary(juce::File directory);

    static juce::File defaultDirectory(const juce::String& pluginName);

    void setFactoryNames(std::vector<juce::String> names);
    [[nodiscard]] const juce::File& directory() const noexcept { return directory_; }

    [[nodiscard]] std::vector<PresetEntry> entries(const juce::String& query, bool favoritesOnly) const;

    bool saveUser(const juce::String& name, const juce::ValueTree& state, juce::String& error);
    bool renameUser(const juce::String& from, const juce::String& to, juce::String& error);
    bool deleteUser(const juce::String& name, juce::String& error);
    bool setFavorite(const juce::String& name, bool favorite, juce::String& error);
    [[nodiscard]] bool isFavorite(const juce::String& name) const;
    [[nodiscard]] juce::ValueTree readUser(const juce::String& name) const;
    [[nodiscard]] bool isFactoryName(const juce::String& name) const;

private:
    [[nodiscard]] static bool validName(const juce::String& name);
    [[nodiscard]] juce::File fileFor(const juce::String& name) const;
    void loadFavorites() const;
    bool writeFavorites(juce::String& error) const;

    juce::File directory_;
    std::vector<juce::String> factoryNames_;
    mutable std::vector<juce::String> favorites_;
    mutable bool favoritesLoaded_ = false;
};

} // namespace suite
