#include "Effects.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <ranges>
#include <vector>

using namespace openhatter::core;

TEST_CASE("DistortionShaper saturates predictably across all types", "[core][effects]") {
    DistortionShaper shaper;
    const double sampleRate = 48000.0;

    const std::array<DistortionType, 4> types = {
        DistortionType::Tape,
        DistortionType::Tube,
        DistortionType::HardClip,
        DistortionType::Wavefold
    };

    for (auto type : types) {
        shaper.reset();
        const float outLow = shaper.processSample(0.5f, type, 0.0f, 0.0f, sampleRate);
        REQUIRE(std::abs(outLow - 0.5f) < 0.001f);

        const float outDriven = shaper.processSample(0.5f, type, 0.8f, 0.0f, sampleRate);
        REQUIRE_FALSE(std::isnan(outDriven));
        REQUIRE(std::abs(outDriven) <= 1.5f);
    }
}

TEST_CASE("StereoCompressor applies gain reduction on loud transients", "[core][effects]") {
    StereoCompressor comp;
    comp.prepare(48000.0);

    CompressorSettings settings;
    settings.thresholdDb = -12.0f;
    settings.ratio = 6.0f;
    settings.attackMs = 0.5f;
    settings.releaseMs = 50.0f;
    settings.makeupDb = 0.0f;
    settings.mix = 1.0f;

    float sampleL = 1.0f;
    float sampleR = 1.0f;
    for (int i = 0; i < 500; ++i) {
        sampleL = 1.0f;
        sampleR = 1.0f;
        comp.process(sampleL, sampleR, settings);
    }

    REQUIRE(sampleL < 0.6f);
    REQUIRE(sampleR < 0.6f);
    REQUIRE(sampleL > 0.01f);
}

TEST_CASE("BrickwallLimiter prevents signal from exceeding ceiling", "[core][effects]") {
    BrickwallLimiter limiter;
    limiter.prepare(48000.0);

    LimiterSettings settings;
    settings.ceilingDb = -0.5f;
    settings.enabled = true;

    const float ceilingLin = std::pow(10.0f, -0.5f / 20.0f);

    for (int i = 0; i < 200; ++i) {
        float sampleL = 4.0f;
        float sampleR = 4.0f;
        limiter.process(sampleL, sampleR, settings);

        REQUIRE(std::abs(sampleL) <= (ceilingLin + 0.001f));
        REQUIRE(std::abs(sampleR) <= (ceilingLin + 0.001f));
    }
}

TEST_CASE("Reverse sync timing targets are exact", "[core][effects]") {
    const double ppq = 3.25;
    const double beatsPerBar = 4.0;

    const double targetBeat4 = reverseTargetPpq(ReverseSyncMode::Beat4Auto, ppq, beatsPerBar);
    REQUIRE(targetBeat4 == 4.0);

    const double targetBeatEnd = reverseTargetPpq(ReverseSyncMode::BeatEnd, ppq, beatsPerBar);
    REQUIRE(targetBeatEnd == 4.0);

    const double targetHalfBeat = reverseTargetPpq(ReverseSyncMode::HalfBeat, ppq, beatsPerBar);
    REQUIRE(targetHalfBeat == 3.5);
}

TEST_CASE("DistortionShaper 4x oversampling suppresses foldback aliasing on high sizzle", "[core][effects]") {
    constexpr double sampleRate = 48000.0;
    constexpr int settleSamples = 512;
    constexpr int windowSamples = 1024;
    constexpr double twoPi = 6.28318530717958647692;

    const auto toneMagnitudeAt = [&](const std::vector<float>& signal, double freqHz) {
        double realPart = 0.0;
        double imagPart = 0.0;
        std::ranges::for_each(std::views::iota(0, windowSamples), [&](int idx) {
            const double window = 0.5 - 0.5 * std::cos(twoPi * static_cast<double>(idx) / static_cast<double>(windowSamples - 1));
            const double sample = static_cast<double>(signal[static_cast<size_t>(settleSamples + idx)]) * window;
            const double phase = twoPi * freqHz * static_cast<double>(idx) / sampleRate;
            realPart += sample * std::cos(phase);
            imagPart -= sample * std::sin(phase);
        });
        return std::sqrt(realPart * realPart + imagPart * imagPart);
    };

    const std::array<DistortionType, 4> types{{
        DistortionType::Tape, DistortionType::Tube, DistortionType::HardClip, DistortionType::Wavefold
    }};

    std::ranges::for_each(types, [&](DistortionType type) {
        DistortionShaper shaper;
        shaper.reset();
        std::vector<float> output(settleSamples + windowSamples, 0.0f);
        std::ranges::for_each(std::views::iota(0, settleSamples + windowSamples), [&](int idx) {
            const float input = 0.75f * static_cast<float>(std::sin(twoPi * 13000.0 * static_cast<double>(idx) / sampleRate));
            output[static_cast<size_t>(idx)] = shaper.processSample(input, type, 0.85f, 0.0f, sampleRate);
        });
        const double fundamental = toneMagnitudeAt(output, 13000.0);
        const double thirdHarmonicAlias = toneMagnitudeAt(output, 9000.0);
        CAPTURE(static_cast<int>(type), fundamental, thirdHarmonicAlias);
        CHECK(fundamental > 10.0);
        CHECK(thirdHarmonicAlias < fundamental * 0.02);
    });
}

