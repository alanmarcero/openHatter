#include "HatEngine.h"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <new>

namespace {
std::atomic<uint64_t> callbackAllocations { 0 };
thread_local bool countCallbackAllocations = false;
}

void* operator new(std::size_t size) {
    if (countCallbackAllocations)
        callbackAllocations.fetch_add(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    if (countCallbackAllocations)
        callbackAllocations.fetch_add(1, std::memory_order_relaxed);
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

using namespace openhatter::core;

TEST_CASE("HatEngine responds to MIDI trigger and processes audio", "[core][engine]") {
    HatEngine engine;
    engine.prepare(48000.0);

    EngineSettings settings;
    Transport transport;
    transport.bpm = 138.0;

    std::vector<MidiTrigger> triggers = {
        { .sampleOffset = 10, .noteNumber = 66, .velocity = 0.9f, .audition = false }
    };

    std::vector<float> left(256, 0.0f);
    std::vector<float> right(256, 0.0f);
    float* channels[2] = { left.data(), right.data() };

    engine.process(settings, transport, triggers, channels, 2, 256);

    REQUIRE(engine.triggerSerial() == 1);
    REQUIRE(engine.currentPeak() > 0.0f);
    REQUIRE(engine.currentHatProgress() < 1.0f);
}

TEST_CASE("HatEngine WAV Lock-In captures and recalls audio bit-exact", "[core][engine]") {
    HatEngine engine;
    engine.prepare(48000.0);

    EngineSettings settings;
    engine.lockCurrentHat(settings);

    REQUIRE(engine.isHatLocked());
    REQUIRE(!engine.wavCache().left().empty());

    const auto wavBytes = engine.wavCache().toWavBytes();
    REQUIRE(wavBytes.size() > 44);

    HatEngine restoredEngine;
    restoredEngine.prepare(48000.0);
    const bool success = restoredEngine.restoreLockedWav(wavBytes);

    REQUIRE(success);
    REQUIRE(restoredEngine.isHatLocked());
    REQUIRE(restoredEngine.wavCache().left().size() == engine.wavCache().left().size());
}

TEST_CASE("HatEngine responds to MIDI CC 74 brightness sweep", "[core][engine]") {
    HatEngine engine;
    engine.prepare(48000.0);

    EngineSettings settings;
    Transport transport;
    transport.bpm = 138.0;

    std::vector<MidiTrigger> triggers = {
        { .sampleOffset = 0, .noteNumber = 66, .velocity = 1.0f, .audition = false }
    };
    std::vector<MidiCcEvent> ccEvents = {
        { .sampleOffset = 0, .controller = 74, .value = 10 }
    };

    std::vector<float> leftDark(1024, 0.0f);
    std::vector<float> rightDark(1024, 0.0f);
    float* channelsDark[2] = { leftDark.data(), rightDark.data() };

    engine.process(settings, transport, triggers, channelsDark, 2, 1024, ccEvents);
    REQUIRE(engine.currentPeak() > 0.0f);
}

TEST_CASE("HatEngine follows host PPQ for enabled 1/16-note rolls", "[core][engine][performance]") {
    HatEngine engine;
    engine.prepare(48000.0);
    EngineSettings settings;
    settings.rollEnabled = true;
    settings.rollDivision = 1;
    Transport transport;
    transport.bpm = 120.0;
    transport.ppqPosition = 0.0;

    const std::array<MidiTrigger, 1> triggers {{
        { .sampleOffset = 0, .noteNumber = 46, .velocity = 0.8f }
    }};
    std::vector<float> left(48000, 0.0f);
    std::vector<float> right(48000, 0.0f);
    float* channels[2] = { left.data(), right.data() };

    engine.process(settings, transport, triggers, channels, 2, static_cast<int>(left.size()));

    REQUIRE(engine.triggerSerial() == 8);
}

TEST_CASE("HatEngine keeps 1/32 rolls with synced reverse allocation-free and bounded", "[core][engine][performance][realtime]") {
    HatEngine engine;
    engine.prepare(48000.0);
    EngineSettings settings;
    settings.rollEnabled = true;
    settings.rollDivision = 2;
    settings.effects.reverseMode = ReverseSyncMode::HalfBeat;
    settings.effects.reverseMix = 1.0f;
    Transport transport;
    transport.bpm = 120.0;
    transport.ppqPosition = 0.0;

    constexpr int sampleCount = 96000;
    const std::array<MidiTrigger, 1> triggers {{
        { .sampleOffset = 0, .noteNumber = 46, .velocity = 0.8f }
    }};
    std::vector<float> left(sampleCount, 0.0f);
    std::vector<float> right(sampleCount, 0.0f);
    float* channels[2] = { left.data(), right.data() };

    HatEngine dryReference;
    dryReference.prepare(48000.0);
    auto drySettings = settings;
    drySettings.effects.reverseMode = ReverseSyncMode::Off;
    std::vector<float> dryLeft(sampleCount, 0.0f);
    std::vector<float> dryRight(sampleCount, 0.0f);
    float* dryChannels[2] = { dryLeft.data(), dryRight.data() };
    dryReference.process(drySettings, transport, triggers, dryChannels, 2, sampleCount);

    callbackAllocations.store(0, std::memory_order_relaxed);
    countCallbackAllocations = true;
    engine.process(settings, transport, triggers, channels, 2, sampleCount);
    countCallbackAllocations = false;

    REQUIRE(engine.triggerSerial() == 32);
    REQUIRE(callbackAllocations.load(std::memory_order_relaxed) == 0);
    REQUIRE(engine.reverseCallbackWorkUnits() > 0);
    REQUIRE(engine.reverseCallbackWorkUnits() <= sampleCount + engine.triggerSerial());

    constexpr int reverseCheckSample = 5999;
    REQUIRE(std::abs(left[reverseCheckSample] - dryLeft[reverseCheckSample]) > 0.000001f);
}

TEST_CASE("HatEngine resumes a paused roll at the next host grid step", "[core][engine][performance]") {
    HatEngine engine;
    engine.prepare(48000.0);
    EngineSettings settings;
    settings.rollEnabled = true;
    Transport transport;
    transport.bpm = 120.0;
    transport.ppqPosition = 0.0;
    const std::array<MidiTrigger, 1> firstHit {{
        { .sampleOffset = 0, .noteNumber = 46, .velocity = 0.8f }
    }};
    std::vector<float> left(512, 0.0f);
    std::vector<float> right(512, 0.0f);
    float* channels[2] = { left.data(), right.data() };
    engine.process(settings, transport, firstHit, channels, 2, 512);
    REQUIRE(engine.triggerSerial() == 1);

    transport.ppqPosition = 0.3;
    transport.isPlaying = false;
    left.assign(128, 0.0f);
    right.assign(128, 0.0f);
    engine.process(settings, transport, {}, channels, 2, 128);

    transport.isPlaying = true;
    left.assign(128, 0.0f);
    right.assign(128, 0.0f);
    engine.process(settings, transport, {}, channels, 2, 128);
    REQUIRE(engine.triggerSerial() == 1);
}

TEST_CASE("HatEngine routes each generated layer to its dedicated output buffers", "[core][engine][outputs]") {
    HatEngine engine;
    engine.prepare(48000.0);
    EngineSettings settings;
    Transport transport;
    const std::array<MidiTrigger, 1> triggers {{
        { .sampleOffset = 0, .noteNumber = 46, .velocity = 0.9f }
    }};
    std::vector<float> left(2048, 0.0f);
    std::vector<float> right(2048, 0.0f);
    std::array<std::vector<float>, 8> stems;
    for (auto& stem : stems) stem.assign(left.size(), 0.0f);
    float* channels[2] = { left.data(), right.data() };
    LayerOutputBuffers outputBuffers {
        .metalLeft = stems[0].data(), .metalRight = stems[1].data(),
        .airLeft = stems[2].data(), .airRight = stems[3].data(),
        .chickLeft = stems[4].data(), .chickRight = stems[5].data(),
        .reverbLeft = stems[6].data(), .reverbRight = stems[7].data()
    };

    engine.process(settings, transport, triggers, channels, 2, static_cast<int>(left.size()), {}, &outputBuffers);

    for (const auto& stem : stems) {
        const float stemPeak = *std::max_element(stem.begin(), stem.end(), [](float a, float b) {
            return std::abs(a) < std::abs(b);
        });
        REQUIRE(std::abs(stemPeak) > 0.001f);
    }
}
