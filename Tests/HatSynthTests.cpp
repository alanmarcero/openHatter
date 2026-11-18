#include "HatSynth.h"
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <ranges>
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

    const std::array<NoiseType, 8> noises {{
        NoiseType::CrispWhite, NoiseType::PinkSilk, NoiseType::VelvetShimmer, NoiseType::LoFiCrunch,
        NoiseType::AnalogMetal, NoiseType::BronzeCymbal, NoiseType::SupersawSizzle, NoiseType::CombWash
    }};
    const std::array<ChickType, 3> chicks {{
        ChickType::CrispStick, ChickType::PedalFoot, ChickType::LaserTick
    }};

    std::ranges::for_each(noises, [&](NoiseType noise) {
        std::ranges::for_each(chicks, [&](ChickType chick) {
            HatSynthSettings settings;
            settings.noiseType = noise;
            settings.chickType = chick;
            settings.chickLevel = 0.5f;

            std::vector<float> left, right;
            synth.renderBlock(left, right, settings, 1.0f);

            REQUIRE(!left.empty());
            REQUIRE(left.size() == right.size());

            float maxPeak = 0.0f;
            std::ranges::for_each(std::views::iota(size_t{0}, left.size()), [&](size_t i) {
                REQUIRE_FALSE(std::isnan(left[i]));
                REQUIRE_FALSE(std::isnan(right[i]));
                maxPeak = std::max(maxPeak, std::max(std::abs(left[i]), std::abs(right[i])));
            });
            REQUIRE(maxPeak > 0.05f);
        });
    });
}

TEST_CASE("Bronze body and hiss saturation add low-mid body weight and sustained tails", "[core][synth][body]") {
    const auto renderAir = [](float bodyLevel, float hissDrive, float decayMs) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseType = NoiseType::BronzeCymbal;
        settings.noiseHPF = 360.0f;
        settings.noiseDecayMs = decayMs;
        settings.bodyLevel = bodyLevel;
        settings.bodyTone = 0.45f;
        settings.bodyDecayMs = 240.0f;
        settings.hissDrive = hissDrive;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 0.0f;
        synth.trigger(settings, 1.0f);
        std::vector<float> out;
        while (synth.isPlaying())
            out.push_back(synth.processSampleLayers(settings).air.left);
        return out;
    };

    const auto thin = renderAir(0.0f, 0.0f, 500.0f);
    const auto thick = renderAir(0.75f, 0.45f, 900.0f);
    double thinEnergy = 0.0;
    double thickEnergy = 0.0;
    std::ranges::for_each(std::views::iota(size_t{0}, std::min(thin.size(), thick.size())), [&](size_t i) {
        thinEnergy += static_cast<double>(thin[i]) * thin[i];
        thickEnergy += static_cast<double>(thick[i]) * thick[i];
    });
    REQUIRE(thick.size() > 40000);
    REQUIRE(thickEnergy > thinEnergy * 1.4);
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

namespace {
double spectralCentroid(const std::vector<float>& audio, double sampleRate, size_t start) {
    constexpr int nfft = 1024;
    double weighted = 0.0;
    double weight = 0.0;
    for (int k = 1; k < nfft / 2; k += 4) {
        double re = 0.0;
        double im = 0.0;
        for (int n = 0; n < nfft; ++n) {
            const double angle = 2.0 * 3.14159265358979323846 * k * n / static_cast<double>(nfft);
            const float sample = audio[start + static_cast<size_t>(n)];
            re += sample * std::cos(angle);
            im -= sample * std::sin(angle);
        }
        const double mag = std::sqrt(re * re + im * im);
        weighted += (k * sampleRate / nfft) * mag;
        weight += mag;
    }
    return weight > 0.0 ? weighted / weight : 0.0;
}
}

TEST_CASE("Hat textures stay in the hat band without DC or runaway peaks", "[core][synth][tone]") {
    for (int type = 0; type < 8; ++type) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseType = static_cast<NoiseType>(type);
        settings.noiseDecayMs = 280.0f;
        settings.noiseCurve = -0.5f;
        settings.noiseHPF = 600.0f;
        settings.noiseLPF = 17000.0f;
        settings.noiseLevel = 0.9f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.bodyLevel = 0.0f;
        settings.reverbMix = 0.0f;
        settings.hissDrive = 0.0f;
        synth.trigger(settings, 1.0f);
        std::vector<float> audio;
        double sum = 0.0;
        float peak = 0.0f;
        while (synth.isPlaying()) {
            const float sample = synth.processSample(settings).left;
            REQUIRE(std::isfinite(sample));
            audio.push_back(sample);
            sum += sample;
            peak = std::max(peak, std::abs(sample));
        }
        CAPTURE(type);
        CHECK(peak > 0.05f);
        CHECK(peak < 1.6f);
        CHECK(std::abs(sum / static_cast<double>(audio.size())) < 0.003);
    }

    const auto render = [](NoiseType type) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseType = type;
        settings.noiseDecayMs = 320.0f;
        settings.noiseCurve = -0.45f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.bodyLevel = 0.0f;
        settings.reverbMix = 0.0f;
        settings.hissDrive = 0.0f;
        settings.noiseLevel = 0.9f;
        synth.trigger(settings, 1.0f);
        std::vector<float> audio;
        while (synth.isPlaying()) audio.push_back(synth.processSample(settings).left);
        return audio;
    };
    const auto crisp = render(NoiseType::CrispWhite);
    const auto metal = render(NoiseType::AnalogMetal);
    const auto bronze = render(NoiseType::BronzeCymbal);
    // A 909 hat is a bright resonant noise. The 808 metal sits lower, but still
    // above the old sub-2 kHz buzz.
    CHECK(spectralCentroid(crisp, 48000.0, 800) > 6800.0);
    CHECK(spectralCentroid(metal, 48000.0, 800) > 5200.0);
    const double bronzeAttack = spectralCentroid(bronze, 48000.0, 400);
    const double bronzeTail = spectralCentroid(bronze, 48000.0, 48000 / 5);
    CHECK(bronzeAttack > bronzeTail + 80.0);
}

TEST_CASE("A closed decay finishes quickly and an open decay keeps ringing", "[core][synth][envelope]") {
    const auto audibleMs = [](float decayMs, float curve) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseDecayMs = decayMs;
        settings.noiseCurve = curve;
        settings.noiseAttackMs = 0.2f;
        settings.noiseHoldMs = 0.0f;
        settings.bodyLevel = 0.0f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 0.0f;
        synth.trigger(settings, 1.0f);
        std::vector<float> audio;
        while (synth.isPlaying()) audio.push_back(synth.processSample(settings).left);
        float peak = 0.0f;
        for (float sample : audio) peak = std::max(peak, std::abs(sample));
        int last = 0;
        for (int i = 0; i < static_cast<int>(audio.size()); ++i)
            if (std::abs(audio[static_cast<size_t>(i)]) > peak * 0.01f) last = i;
        return last * 1000.0 / 48000.0;
    };
    const double closed = audibleMs(60.0f, -0.9f);
    const double open = audibleMs(500.0f, -0.45f);
    CHECK(closed > 30.0);
    CHECK(closed < 130.0);
    CHECK(open > 340.0);
}
