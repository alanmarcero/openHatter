#include "HatSynth.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace openhatter::core;

TEST_CASE("noteToHz calculation is musically accurate", "[core][synth]") {
    const float a4 = noteToHz(9, 4, 0.0f);
    REQUIRE(std::abs(a4 - 440.0f) < 0.1f);

    const float aSharp4 = noteToHz(10, 4, 0.0f);
    const float a4Plus100 = noteToHz(9, 4, 100.0f);
    REQUIRE(std::abs(aSharp4 - a4Plus100) < 0.2f);
}

TEST_CASE("HatSynth produces valid stereo open hat audio block without NaNs", "[core][synth]") {
    HatSynth synth;
    synth.prepare(48000.0);

    HatSynthSettings settings;
    std::vector<float> left;
    std::vector<float> right;

    synth.renderBlock(left, right, settings, 1.0f, 0);

    REQUIRE(!left.empty());
    REQUIRE(left.size() == right.size());

    float maxPeak = 0.0f;
    for (size_t i = 0; i < left.size(); ++i) {
        REQUIRE_FALSE(std::isnan(left[i]));
        REQUIRE_FALSE(std::isnan(right[i]));
        REQUIRE_FALSE(std::isinf(left[i]));
        REQUIRE_FALSE(std::isinf(right[i]));
        maxPeak = std::max(maxPeak, std::max(std::abs(left[i]), std::abs(right[i])));
    }

    REQUIRE(maxPeak > 0.05f);
}

TEST_CASE("HatSynth finishes and stops playing after duration", "[core][synth]") {
    HatSynth synth;
    synth.prepare(48000.0);

    HatSynthSettings settings;
    settings.noiseDecayMs = 30.0f;
    settings.toneDecayMs = 30.0f;
    settings.reverbDecayMs = 50.0f;
    settings.reverbGateMs = 40.0f;

    synth.trigger(settings, 1.0f, 0);
    REQUIRE(synth.isPlaying());

    int maxSamples = 48000;
    while (synth.isPlaying() && --maxSamples > 0) {
        [[maybe_unused]] const auto s = synth.processSample(settings);
    }

    REQUIRE_FALSE(synth.isPlaying());
    REQUIRE(synth.getEnvelopeProgress() >= 1.0f);
}

TEST_CASE("GatedStereoReverb respects choke gate cutoff", "[core][synth]") {
    GatedStereoReverb reverb;
    reverb.prepare(48000.0);

    [[maybe_unused]] const auto initFrame = reverb.process(1.0f, 1.0f, 0.5f, 500.0f, 0.2f, 50.0f, 48000.0);

    float maxTail = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        const auto f = reverb.process(0.0f, 0.0f, 0.5f, 500.0f, 0.2f, 50.0f, 48000.0);
        maxTail = std::max(maxTail, std::max(std::abs(f.left), std::abs(f.right)));
    }
    REQUIRE(maxTail > 0.01f);

    StereoFrame gatedFrame = { 0.0f, 0.0f };
    for (int i = 0; i < 3000; ++i) {
        gatedFrame = reverb.process(0.0f, 0.0f, 0.5f, 500.0f, 0.2f, 50.0f, 48000.0);
    }

    REQUIRE(gatedFrame.left == 0.0f);
    REQUIRE(gatedFrame.right == 0.0f);
}

TEST_CASE("HatSynth renders across all MetalModes and ChickTypes without errors", "[core][synth]") {
    HatSynth synth;
    synth.prepare(48000.0);

    const MetalMode modes[] = { MetalMode::Classic909, MetalMode::AnjunaSilk, MetalMode::UpliftingFMBell, MetalMode::Analog808Crisp };
    const ChickType chicks[] = { ChickType::CrispStick, ChickType::PedalFoot, ChickType::LaserTick };

    for (auto mode : modes) {
        for (auto chick : chicks) {
            HatSynthSettings settings;
            settings.metalMode = mode;
            settings.chickType = chick;
            settings.toneLevel = 0.8f;
            settings.chickLevel = 0.5f;

            std::vector<float> left, right;
            synth.renderBlock(left, right, settings, 1.0f, 0);

            REQUIRE(!left.empty());
            REQUIRE(left.size() == right.size());

            float peak = 0.0f;
            for (size_t i = 0; i < left.size(); ++i) {
                REQUIRE_FALSE(std::isnan(left[i]));
                REQUIRE_FALSE(std::isnan(right[i]));
                peak = std::max(peak, std::max(std::abs(left[i]), std::abs(right[i])));
            }
            REQUIRE(peak > 0.05f);
        }
    }
}

TEST_CASE("HatSynth metallic pitch responds to note and octave settings", "[core][synth]") {
    HatSynth synth;
    synth.prepare(48000.0);

    HatSynthSettings lowPitchSettings;
    lowPitchSettings.toneLevel = 1.0f;
    lowPitchSettings.noiseLevel = 0.0f;
    lowPitchSettings.chickLevel = 0.0f;
    lowPitchSettings.reverbMix = 0.0f;
    lowPitchSettings.preChickCount = 1;
    lowPitchSettings.note = 0;
    lowPitchSettings.octave = 4;
    lowPitchSettings.fineCents = 0.0f;

    HatSynthSettings highPitchSettings = lowPitchSettings;
    highPitchSettings.note = 6;
    highPitchSettings.octave = 7;

    std::vector<float> leftLow, rightLow;
    synth.renderBlock(leftLow, rightLow, lowPitchSettings, 1.0f, 0);

    std::vector<float> leftHigh, rightHigh;
    synth.renderBlock(leftHigh, rightHigh, highPitchSettings, 1.0f, 0);

    REQUIRE(!leftLow.empty());
    REQUIRE(!leftHigh.empty());

    float diffSum = 0.0f;
    const size_t len = std::min(leftLow.size(), leftHigh.size());
    for (size_t i = 0; i < len; ++i) {
        diffSum += std::abs(leftLow[i] - leftHigh[i]);
    }
    REQUIRE(diffSum > 1.0f);
}
