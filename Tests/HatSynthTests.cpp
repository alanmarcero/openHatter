#include "HatSynth.h"
#include <catch2/catch_test_macros.hpp>
#include <cmath>

using namespace openhatter::core;

namespace {
float peak(const std::vector<float>& samples) {
    float value = 0.0f;
    for (const float sample : samples)
        value = std::max(value, std::abs(sample));
    return value;
}
}

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

TEST_CASE("HatSynth filtered air follows an octave shift in MIDI pitch", "[core][synth][pitch]") {
    HatSynth lowNote;
    HatSynth highNote;
    lowNote.prepare(48000.0);
    highNote.prepare(48000.0);
    HatSynthSettings settings;
    settings.toneLevel = 0.0f;
    settings.noiseLevel = 1.0f;
    settings.noiseHPF = 800.0f;
    settings.noiseLPF = 12000.0f;
    settings.noiseAttackMs = 0.0f;
    settings.noiseHoldMs = 0.0f;
    settings.chickLevel = 0.0f;
    settings.preChickLevel = 0.0f;
    settings.reverbMix = 0.0f;
    lowNote.triggerMidi(settings, 1.0f, 60);
    highNote.triggerMidi(settings, 1.0f, 72);

    static_cast<void>(lowNote.processSampleLayers(settings));
    static_cast<void>(highNote.processSampleLayers(settings));
    const float lowAir = lowNote.processSampleLayers(settings).air.left;
    const float highAir = highNote.processSampleLayers(settings).air.left;

    REQUIRE(std::abs(lowAir - highAir) > 1.0e-5f);
}

TEST_CASE("HatSynth metallic modes follow an octave shift in MIDI pitch", "[core][synth][pitch]") {
    HatSynth lowNote;
    HatSynth highNote;
    lowNote.prepare(48000.0);
    highNote.prepare(48000.0);
    HatSynthSettings settings;
    settings.toneLevel = 1.0f;
    settings.noiseLevel = 0.0f;
    settings.chickLevel = 0.0f;
    settings.preChickLevel = 0.0f;
    settings.reverbMix = 0.0f;
    lowNote.triggerMidi(settings, 1.0f, 60);
    highNote.triggerMidi(settings, 1.0f, 72);

    static_cast<void>(lowNote.processSampleLayers(settings));
    static_cast<void>(highNote.processSampleLayers(settings));
    const float lowMetal = lowNote.processSampleLayers(settings).metal.left;
    const float highMetal = highNote.processSampleLayers(settings).metal.left;

    REQUIRE(std::abs(lowMetal - highMetal) > 1.0e-5f);
}

TEST_CASE("HatSynth independently applies metal velocity curve and resonator tuning", "[core][synth][performance]") {
    HatSynthSettings settings;
    settings.toneLevel = 1.0f;
    settings.noiseLevel = 0.0f;
    settings.chickLevel = 0.0f;
    settings.preChickLevel = 0.0f;
    settings.reverbMix = 0.0f;

    HatSynth linearSynth;
    linearSynth.prepare(48000.0);
    std::vector<float> linearLeft, linearRight;
    linearSynth.renderBlock(linearLeft, linearRight, settings, 0.5f, 0);

    settings.velocityMetal = 2.0f;
    HatSynth curvedSynth;
    curvedSynth.prepare(48000.0);
    std::vector<float> curvedLeft, curvedRight;
    curvedSynth.renderBlock(curvedLeft, curvedRight, settings, 0.5f, 0);
    REQUIRE(peak(linearLeft) > 0.0f);
    REQUIRE(peak(curvedLeft) < peak(linearLeft) * 0.8f);

    settings.velocityMetal = 1.0f;
    HatSynth tunedSynth;
    tunedSynth.prepare(48000.0);
    std::vector<float> tunedLeft, tunedRight;
    settings.resonatorTuneSemitones[0] = 12.0f;
    tunedSynth.renderBlock(tunedLeft, tunedRight, settings, 1.0f, 0);

    float difference = 0.0f;
    for (size_t i = 0; i < std::min(linearLeft.size(), tunedLeft.size()); ++i)
        difference += std::abs(linearLeft[i] * 2.0f - tunedLeft[i]);
    REQUIRE(difference > 0.1f);
}

TEST_CASE("HatSynth resonator mute removes the metallic layer", "[core][synth][performance]") {
    HatSynthSettings settings;
    settings.toneLevel = 1.0f;
    settings.noiseLevel = 0.0f;
    settings.chickLevel = 0.0f;
    settings.preChickLevel = 0.0f;
    settings.reverbMix = 0.0f;
    settings.resonatorEnabled.fill(false);

    const auto renderMetalPeak = [&settings] {
        HatSynth synth;
        synth.prepare(48000.0);
        synth.trigger(settings, 1.0f, 0);
        float maximum = 0.0f;
        while (synth.isPlaying()) {
            const auto layers = synth.processSampleLayers(settings);
            maximum = std::max(maximum, std::max(std::abs(layers.metal.left), std::abs(layers.metal.right)));
        }
        return maximum;
    };
    REQUIRE(renderMetalPeak() < 1.0e-6f);

    settings.resonatorEnabled[0] = true;
    REQUIRE(renderMetalPeak() > 0.01f);
}

TEST_CASE("HatSynth gives reverb an independent velocity response", "[core][synth][performance]") {
    const auto renderReverbPeak = [](float velocity, float reverbCurve) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.toneLevel = 1.0f;
        settings.noiseLevel = 0.0f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 1.0f;
        settings.velocityMetal = 2.0f;
        settings.velocityReverb = reverbCurve;
        synth.trigger(settings, velocity, 0);
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
