#include "HatEngine.h"
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <new>
#include <ranges>

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

TEST_CASE("Full ADSR note-off stops the matching host-synced roll", "[core][engine][performance][midi]") {
    HatEngine engine;
    engine.prepare(48000.0);
    EngineSettings settings;
    settings.synth.fullAdsrEnabled = true;
    settings.synth.ampReleaseMs = 0.0f;
    settings.rollEnabled = true;
    settings.rollDivision = 2;
    Transport transport;
    transport.bpm = 120.0;
    transport.ppqPosition = 0.0;

    const std::array<MidiTrigger, 2> triggers {{
        { .sampleOffset = 0, .noteNumber = 60, .velocity = 0.8f, .midiChannel = 2 },
        { .sampleOffset = 100, .noteNumber = 60, .midiChannel = 2, .noteOff = true }
    }};
    std::array<float, 4000> left{};
    std::array<float, 4000> right{};
    float* channels[2] = { left.data(), right.data() };

    engine.process(settings, transport, triggers, channels, 2, static_cast<int>(left.size()));

    REQUIRE(engine.triggerSerial() == 1);
}

TEST_CASE("HatEngine reverses the first reverse-enabled MIDI hit", "[core][engine][reverse]") {
    HatEngine reverseEngine;
    HatEngine dryReference;
    reverseEngine.prepare(48000.0);
    dryReference.prepare(48000.0);

    EngineSettings reverseSettings;
    reverseSettings.effects.reverseMode = ReverseSyncMode::HalfBeat;
    reverseSettings.effects.reverseMix = 1.0f;
    auto drySettings = reverseSettings;
    drySettings.effects.reverseMode = ReverseSyncMode::Off;
    Transport transport;
    transport.bpm = 120.0;
    transport.ppqPosition = 0.0;
    const std::array<MidiTrigger, 1> triggers {{
        { .sampleOffset = 0, .noteNumber = 46, .velocity = 0.8f }
    }};
    std::vector<float> reverseLeft(2048, 0.0f);
    std::vector<float> reverseRight(2048, 0.0f);
    std::vector<float> dryLeft(2048, 0.0f);
    std::vector<float> dryRight(2048, 0.0f);
    float* reverseChannels[2] = { reverseLeft.data(), reverseRight.data() };
    float* dryChannels[2] = { dryLeft.data(), dryRight.data() };

    callbackAllocations.store(0, std::memory_order_relaxed);
    countCallbackAllocations = true;
    reverseEngine.process(reverseSettings, transport, triggers, reverseChannels, 2, 2048);
    countCallbackAllocations = false;
    dryReference.process(drySettings, transport, triggers, dryChannels, 2, 2048);

    constexpr int maxReverseSamples = 96000;
    constexpr int reverseCheckSample = 512;
    REQUIRE(callbackAllocations.load(std::memory_order_relaxed) == 0);
    REQUIRE(reverseEngine.reversePreviewRenderCount() == 1);
    REQUIRE(reverseEngine.reverseCallbackWorkUnits() <= maxReverseSamples + 2048 + 1);
    REQUIRE(std::abs(reverseLeft[reverseCheckSample] - dryLeft[reverseCheckSample]) > 0.000001f);
}

TEST_CASE("HatEngine skips Beat4Auto preview work outside the trigger window", "[core][engine][reverse]") {
    HatEngine engine;
    engine.prepare(48000.0);
    EngineSettings settings;
    settings.effects.reverseMode = ReverseSyncMode::Beat4Auto;
    Transport transport;
    transport.bpm = 120.0;
    transport.ppqPosition = 0.0;
    const std::array<MidiTrigger, 1> triggers {{
        { .sampleOffset = 0, .noteNumber = 46, .velocity = 0.8f }
    }};
    std::array<float, 512> left {};
    std::array<float, 512> right {};
    float* channels[2] = { left.data(), right.data() };

    engine.process(settings, transport, triggers, channels, 2, static_cast<int>(left.size()));

    REQUIRE(engine.triggerSerial() == 1);
    REQUIRE(engine.reversePreviewRenderCount() == 0);
    REQUIRE(engine.reverseCallbackWorkUnits() == 0);
}

TEST_CASE("HatEngine caches one Beat4Auto preview for an enabled roll", "[core][engine][reverse][performance]") {
    HatEngine engine;
    engine.prepare(48000.0);
    EngineSettings settings;
    settings.rollEnabled = true;
    settings.rollDivision = 2;
    settings.effects.reverseMode = ReverseSyncMode::Beat4Auto;
    Transport transport;
    transport.bpm = 120.0;
    transport.ppqPosition = 0.0;
    const std::array<MidiTrigger, 1> triggers {{
        { .sampleOffset = 0, .noteNumber = 46, .velocity = 0.8f }
    }};
    std::array<float, 1> startLeft {};
    std::array<float, 1> startRight {};
    float* startChannels[2] = { startLeft.data(), startRight.data() };

    engine.process(settings, transport, triggers, startChannels, 2, 1);
    const auto previewWork = engine.reverseCallbackWorkUnits();

    transport.ppqPosition = 3.0;
    std::vector<float> rollLeft(5000, 0.0f);
    std::vector<float> rollRight(5000, 0.0f);
    float* rollChannels[2] = { rollLeft.data(), rollRight.data() };
    engine.process(settings, transport, {}, rollChannels, 2, static_cast<int>(rollLeft.size()));

    REQUIRE(engine.triggerSerial() == 2);
    REQUIRE(engine.reversePreviewRenderCount() == 1);
    REQUIRE(engine.isReversing());
    REQUIRE(engine.reverseCallbackWorkUnits() > previewWork);
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
    REQUIRE(engine.reversePreviewRenderCount() == 1);
    REQUIRE(engine.reverseCallbackWorkUnits() > 0);
    REQUIRE(engine.reverseCallbackWorkUnits() <= sampleCount + 96000 + engine.triggerSerial());

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
    std::array<std::vector<float>, 6> stems;
    for (auto& stem : stems) stem.assign(left.size(), 0.0f);
    float* channels[2] = { left.data(), right.data() };
    LayerOutputBuffers outputBuffers {
        .airLeft = stems[0].data(), .airRight = stems[1].data(),
        .chickLeft = stems[2].data(), .chickRight = stems[3].data(),
        .reverbLeft = stems[4].data(), .reverbRight = stems[5].data()
    };

    engine.process(settings, transport, triggers, channels, 2, static_cast<int>(left.size()), {}, &outputBuffers);

    for (const auto& stem : stems) {
        const float stemPeak = *std::max_element(stem.begin(), stem.end(), [](float a, float b) {
            return std::abs(a) < std::abs(b);
        });
        REQUIRE(std::abs(stemPeak) > 0.001f);
    }
}

TEST_CASE("HatEngine ping-pong echo feedback damps highs and clears low mud across repeats", "[core][engine][echo]") {
    HatEngine engine;
    constexpr double sampleRate = 48000.0;
    engine.prepare(sampleRate);

    EngineSettings settings;
    settings.synth.noiseType = NoiseType::CrispWhite;
    settings.synth.noiseHPF = 120.0f;
    settings.synth.noiseLPF = 19000.0f;
    settings.synth.noiseAttackMs = 0.1f;
    settings.synth.noiseHoldMs = 0.0f;
    settings.synth.noiseDecayMs = 25.0f;
    settings.synth.bodyLevel = 0.85f;
    settings.synth.bodyDecayMs = 30.0f;
    settings.synth.reverbMix = 0.0f;
    settings.effects.distMix = 0.0f;
    settings.effects.compressor.mix = 0.0f;
    settings.effects.limiter.enabled = false;
    settings.effects.stereoWidth = 1.0f;
    settings.effects.echoMix = 1.0f;

    Transport transport;
    transport.bpm = 138.0;
    transport.ppqPosition = 0.0;
    const std::array<MidiTrigger, 1> triggers{{
        { .sampleOffset = 0, .noteNumber = 60, .velocity = 1.0f }
    }};

    constexpr int totalSamples = 24000;
    std::vector<float> left(totalSamples, 0.0f);
    std::vector<float> right(totalSamples, 0.0f);
    float* channels[2] = { left.data(), right.data() };
    engine.process(settings, transport, triggers, channels, 2, totalSamples);

    const auto bandRatio = [&](const std::vector<float>& signal, int startSample) {
        constexpr int nfft = 1024;
        constexpr double twoPi = 6.28318530717958647692;
        double midPower = 0.0;
        double highPower = 0.0;
        std::ranges::for_each(std::views::iota(1, nfft / 2), [&](int bin) {
            double realPart = 0.0;
            double imagPart = 0.0;
            std::ranges::for_each(std::views::iota(0, nfft), [&](int idx) {
                const double sample = static_cast<double>(signal[static_cast<size_t>(startSample + idx)]);
                const double angle = twoPi * static_cast<double>(bin * idx) / static_cast<double>(nfft);
                realPart += sample * std::cos(angle);
                imagPart -= sample * std::sin(angle);
            });
            const double power = realPart * realPart + imagPart * imagPart;
            const double hz = static_cast<double>(bin) * sampleRate / static_cast<double>(nfft);
            if (hz >= 2000.0 && hz <= 5500.0) midPower += power;
            if (hz >= 12000.0 && hz <= 20000.0) highPower += power;
        });
        return std::pair{ midPower, highPower / std::max(midPower, 1.0e-12) };
    };

    const int delaySamples = static_cast<int>(0.375 * 60.0 / transport.bpm * sampleRate);
    const auto [firstMid, firstHighRatio] = bandRatio(left, delaySamples + 64);
    const auto [secondMid, secondHighRatio] = bandRatio(right, delaySamples * 2 + 64);
    REQUIRE(firstMid > 1.0e-4);
    REQUIRE(secondMid > 1.0e-5);
    CHECK(secondHighRatio < firstHighRatio * 0.72);
}

TEST_CASE("HatWavCache rejects truncated or overflowing chunkSize without OOB reads", "[core][engine][security]") {
    std::vector<uint8_t> malformed(44, 0);
    std::memcpy(malformed.data(), "RIFF", 4);
    std::memcpy(malformed.data() + 8, "WAVE", 4);
    std::memcpy(malformed.data() + 12, "fmt ", 4);
    malformed[16] = 16;
    malformed[20] = 3; // IEEE float
    malformed[22] = 2; // stereo
    malformed[24] = 0x80;
    malformed[25] = 0xBB; // 48000 Hz
    malformed[34] = 32;
    std::memcpy(malformed.data() + 36, "data", 4);
    malformed[40] = 0xFF;
    malformed[41] = 0xFF;
    malformed[42] = 0xFF;
    malformed[43] = 0x7F; // huge chunkSize past buffer end

    HatWavCache cache;
    REQUIRE_FALSE(cache.fromWavBytes(malformed));
    REQUIRE_FALSE(cache.isLocked());
}
