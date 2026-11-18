#include "HatSynth.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <tuple>
#include <utility>

using namespace openhatter::core;

namespace {
float peak(const std::vector<float>& samples) {
    float value = 0.0f;
    for (const float sample : samples)
        value = std::max(value, std::abs(sample));
    return value;
}
}

TEST_CASE("HatSynth produces valid stereo open hat audio block without NaNs", "[core][synth]") {
    HatSynth synth;
    synth.prepare(48000.0);

    HatSynthSettings settings;
    std::vector<float> left;
    std::vector<float> right;

    synth.renderBlock(left, right, settings, 1.0f);

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
    settings.reverbDecayMs = 50.0f;
    settings.reverbGateMs = 40.0f;

    synth.trigger(settings, 1.0f);
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

TEST_CASE("HatSynth renders across all NoiseTypes and ChickTypes without errors", "[core][synth]") {
    HatSynth synth;
    synth.prepare(48000.0);

    const NoiseType noises[] = { NoiseType::CrispWhite, NoiseType::PinkSilk, NoiseType::VelvetShimmer, NoiseType::LoFiCrunch };
    const ChickType chicks[] = { ChickType::CrispStick, ChickType::PedalFoot, ChickType::LaserTick };

    for (auto noise : noises) {
        for (auto chick : chicks) {
            HatSynthSettings settings;
            settings.noiseType = noise;
            settings.chickType = chick;
            settings.chickLevel = 0.5f;

            std::vector<float> left, right;
            synth.renderBlock(left, right, settings, 1.0f);

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

TEST_CASE("HatSynth air layer is unpitched, so every MIDI note gets the same bright tail", "[core][synth][air]") {
    HatSynthSettings settings;
    settings.noiseHPF = 1650.0f;
    settings.noiseLPF = 17100.0f;
    settings.noiseDecayMs = 184.0f;
    settings.noiseLevel = 0.9f;
    settings.chickLevel = 0.0f;
    settings.preChickLevel = 0.0f;
    settings.reverbMix = 0.0f;

    const auto renderAir = [&settings](int note, double rate) {
        HatSynth synth;
        synth.prepare(rate);
        synth.triggerMidi(settings, 1.0f, note);
        std::vector<float> air;
        for (int i = 0; i < static_cast<int>(rate * 0.4); ++i)
            air.push_back(synth.processSampleLayers(settings).air.left);
        return air;
    };
    const auto tailEnergy = [](const std::vector<float>& air, double rate) {
        double energy = 0.0;
        for (size_t i = static_cast<size_t>(rate * 0.06); i < static_cast<size_t>(rate * 0.18); ++i)
            energy += static_cast<double>(air[i]) * air[i];
        return energy;
    };

    for (const double rate : { 44100.0, 48000.0, 96000.0 }) {
        const auto reference = renderAir(60, rate);
        const double referenceEnergy = tailEnergy(reference, rate);
        REQUIRE(referenceEnergy > 5.0);
        for (const int note : { 0, 24, 36, 42, 46, 84, 108, 120, 127 }) {
            const auto air = renderAir(note, rate);
            CAPTURE(rate, note);
            CHECK(tailEnergy(air, rate) > referenceEnergy * 0.9);
            float difference = 0.0f;
            for (size_t i = 0; i < air.size(); ++i)
                difference = std::max(difference, std::abs(air[i] - reference[i]));
            CHECK(difference < 1.0e-6f);
        }
    }
}

TEST_CASE("HatSynth gives reverb an independent velocity response", "[core][synth][performance]") {
    const auto renderReverbPeak = [](float velocity, float reverbCurve) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseLevel = 1.0f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 1.0f;
        settings.velocityAir = 2.0f;
        settings.velocityReverb = reverbCurve;
        synth.trigger(settings, velocity);
        float maximum = 0.0f;
        while (synth.isPlaying()) {
            const auto layers = synth.processSampleLayers(settings);
            maximum = std::max(maximum, std::max(std::abs(layers.reverb.left), std::abs(layers.reverb.right)));
        }
        return maximum;
    };

    const float soft = renderReverbPeak(0.5f, 1.0f);
    const float full = renderReverbPeak(1.0f, 1.0f);
    REQUIRE(soft > 0.0f);
    REQUIRE(soft / full > 0.4f);
    REQUIRE(soft / full < 0.6f);

    const float softReverbCurve = renderReverbPeak(0.5f, 2.0f);
    const float fullReverbCurve = renderReverbPeak(1.0f, 2.0f);
    REQUIRE(softReverbCurve / fullReverbCurve > 0.2f);
    REQUIRE(softReverbCurve / fullReverbCurve < 0.3f);
}

TEST_CASE("Strike partials above Nyquist leave only the broadband contact", "[core][synth][strike]") {
    for (const double rate : { 32000.0, 44100.0, 48000.0, 96000.0 }) {
        for (const bool held : { false, true }) {
            for (const auto type : { ChickType::CrispStick, ChickType::PedalFoot, ChickType::LaserTick }) {
                HatSynthSettings settings;
                settings.chickType = type;
                settings.chickFreqHz = 10000;
                settings.preChickJitter = 0;
                settings.fullAdsrEnabled = held;
                HatSynth lower, upper;
                lower.prepare(rate);
                upper.prepare(rate);
                lower.triggerMidi(settings, 1, 108);
                auto reference = settings;
                reference.chickType = type == ChickType::LaserTick ? ChickType::PedalFoot : ChickType::LaserTick;
                upper.triggerMidi(reference, 1, 120);
                float difference = 0, contactPeak = 0;
                for (int i = 0; i < static_cast<int>(rate * .020); ++i) {
                    const float a = lower.processSampleLayers(settings).chick.left;
                    const float b = upper.processSampleLayers(reference).chick.left;
                    if (i > static_cast<int>(rate * .0002))
                        difference = std::max(difference, std::abs(a - b));
                    contactPeak = std::max(contactPeak, std::abs(a));
                }
                CAPTURE(rate, held, static_cast<int>(type));
                CHECK(difference < 1.0e-6f);
                CHECK(contactPeak > .05f);
            }
        }
    }
}

TEST_CASE("Laser strike glides down to the played pitch from above", "[core][synth][strike]") {
    for (const double rate : { 32000.0, 48000.0, 96000.0 }) {
        for (const int note : { 48, 60 }) {
            for (const bool held : { false, true }) {
                HatSynthSettings settings;
                settings.chickType = ChickType::LaserTick;
                settings.chickFreqHz = 6800.0f;
                settings.preChickJitter = 0;
                settings.fullAdsrEnabled = held;
                HatSynth played, ultrasonic;
                played.prepare(rate);
                ultrasonic.prepare(rate);
                played.triggerMidi(settings, 1, note);
                ultrasonic.triggerMidi(settings, 1, 127);
                int crossings = 0;
                float previous = 0;
                for (int i = 0; i < static_cast<int>(rate * .006); ++i) {
                    // Both stems share the same broadband strike. The reference tone is above Nyquist.
                    const float tonal = played.processSampleLayers(settings).chick.left
                        - ultrasonic.processSampleLayers(settings).chick.left;
                    if (i >= static_cast<int>(rate * .003) && previous < 0 && tonal >= 0)
                        ++crossings;
                    previous = tonal;
                }
                const float baseHz = settings.chickFreqHz * midiNoteToHz(note) / midiNoteToHz(60);
                const float observed = static_cast<float>(crossings) / .003f;
                CAPTURE(rate, note, held, observed);
                CHECK(observed >= baseHz);
                CHECK(observed <= baseHz * 1.30f);
            }
        }
    }
}
