#include "AbCompare.h"
#include "PresetLibrary.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("A/B slots keep independent copies", "[suite][ab]") {
    suite::AbCompare compare;
    juce::MemoryBlock first("alpha", 5);
    juce::MemoryBlock second("beta", 4);
    compare.prime(first);
    CHECK(compare.active() == 0);
    CHECK(compare.slot(0) == first);
    CHECK(compare.slot(1) == first);

    compare.storeActive(second);
    compare.copyActiveToInactive();
    CHECK(compare.slot(1) == second);
    compare.setActive(1);
    CHECK(compare.active() == 1);
    compare.storeActive(first);
    CHECK(compare.slot(0) == second);
    CHECK(compare.slot(1) == first);
}

TEST_CASE("User presets save, search, favorite, rename and delete without touching factory names", "[suite][presets]") {
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("openhatter-presets-" + juce::Uuid().toString());
    root.deleteRecursively();
    suite::PresetLibrary library(root);
    const juce::String factory = "Uplifting / Euphoria Open";
    library.setFactoryNames({ factory, "Rolling / Heavy Floor Open" });

    juce::ValueTree state("PARAMETERS");
    auto child = juce::ValueTree("PARAM");
    child.setProperty("id", "noise_decay", nullptr);
    child.setProperty("value", 240.0f, nullptr);
    state.appendChild(child, nullptr);

    juce::String error;
    CHECK_FALSE(library.saveUser(factory, state, error));
    CHECK(error.contains("read-only"));
    CHECK_FALSE(library.deleteUser(factory, error));
    CHECK_FALSE(library.renameUser(factory, "Other", error));
    CHECK(library.saveUser("Room Hat", state, error));
    CHECK(error.isEmpty());
    CHECK(root.getChildFile("Room Hat.xml").existsAsFile());

    auto found = library.entries("room", false);
    REQUIRE(found.size() == 1);
    CHECK_FALSE(found[0].factory);
    CHECK(found[0].name == "Room Hat");
    CHECK(library.entries("Euphoria", false).size() == 1);
    CHECK(library.entries("missing-style", false).empty());

    REQUIRE(library.setFavorite(factory, true, error));
    REQUIRE(library.setFavorite("Room Hat", true, error));
    auto favorites = library.entries("", true);
    REQUIRE(favorites.size() == 2);
    CHECK(favorites[0].factory);
    CHECK(favorites[1].name == "Room Hat");

    suite::PresetLibrary reopened(root);
    reopened.setFactoryNames({ factory });
    CHECK(reopened.isFavorite("Room Hat"));
    CHECK(reopened.isFavorite(factory));
    auto loaded = reopened.readUser("Room Hat");
    REQUIRE(loaded.isValid());
    CHECK(static_cast<float>(loaded.getChild(0).getProperty("value")) == 240.0f);

    REQUIRE(reopened.renameUser("Room Hat", "Renamed Hat", error));
    CHECK_FALSE(reopened.readUser("Room Hat").isValid());
    CHECK(reopened.readUser("Renamed Hat").isValid());
    CHECK(reopened.isFavorite("Renamed Hat"));
    CHECK_FALSE(reopened.renameUser("Renamed Hat", factory, error));
    REQUIRE(reopened.deleteUser("Renamed Hat", error));
    CHECK(reopened.entries("Renamed", false).empty());
    CHECK(reopened.entries("Euphoria", false).size() == 1);

    const auto documents = suite::PresetLibrary::defaultDirectory("openHatter");
    CHECK(documents.getFileName() == "Presets");
    CHECK(documents.getParentDirectory().getFileName() == "openHatter");
    CHECK(documents.getParentDirectory().getParentDirectory().getFileName() == "Alan Marcero");
    root.deleteRecursively();
}
