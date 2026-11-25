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
    settings.keyTracking = 0.0f;
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
        settings.velocityCurve = reverbCurve;
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

TEST_CASE("Split-band ADAA hiss saturation shapes high sizzle without distorting bronze body punch", "[core][synth][saturation]") {
    const auto renderWithDrive = [](float hissDrive) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseType = NoiseType::CrispWhite;
        settings.noiseHPF = 4800.0f;
        settings.noiseLPF = 18500.0f;
        settings.noiseLevel = 0.9f;
        settings.bodyLevel = 0.85f;
        settings.bodyTone = 0.15f;
        settings.bodyDecayMs = 220.0f;
        settings.hissDrive = hissDrive;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 0.0f;
        synth.trigger(settings, 1.0f);
        std::vector<float> out(4096, 0.0f);
        std::ranges::for_each(std::views::iota(size_t{0}, out.size()), [&](size_t idx) {
            out[idx] = synth.processSampleLayers(settings).air.left;
        });
        return out;
    };

    const auto clean = renderWithDrive(0.0f);
    const auto driven = renderWithDrive(1.0f);

    const auto bandDiffAndRef = [&](double lowHz, double highHz) {
        constexpr int nfft = 1024;
        constexpr size_t offset = 256;
        constexpr double twoPi = 6.28318530717958647692;
        double refPower = 0.0;
        double diffPower = 0.0;
        std::ranges::for_each(std::views::iota(1, nfft / 2), [&](int bin) {
            const double hz = static_cast<double>(bin) * 48000.0 / static_cast<double>(nfft);
            if (hz < lowHz || hz > highHz) return;
            double refRe = 0.0;
            double refIm = 0.0;
            double diffRe = 0.0;
            double diffIm = 0.0;
            std::ranges::for_each(std::views::iota(0, nfft), [&](int n) {
                const double angle = twoPi * static_cast<double>(bin * n) / static_cast<double>(nfft);
                const double cs = std::cos(angle);
                const double sn = std::sin(angle);
                const double cleanVal = static_cast<double>(clean[offset + static_cast<size_t>(n)]);
                const double diffVal = static_cast<double>(driven[offset + static_cast<size_t>(n)]) - cleanVal;
                refRe += cleanVal * cs;
                refIm -= cleanVal * sn;
                diffRe += diffVal * cs;
                diffIm -= diffVal * sn;
            });
            refPower += refRe * refRe + refIm * refIm;
            diffPower += diffRe * diffRe + diffIm * diffIm;
        });
        return std::sqrt(diffPower / std::max(refPower, 1.0e-12));
    };

    const double bodyDistortion = bandDiffAndRef(600.0, 1800.0);
    const double sizzleChange = bandDiffAndRef(6000.0, 18000.0);
    CAPTURE(bodyDistortion, sizzleChange);
    CHECK(sizzleChange > 0.12);
    CHECK(bodyDistortion < 0.08);
}

TEST_CASE("Uncorrelated stereo PRNG and orthogonal allpass diffusion decorrelate L and R across all noise modes", "[core][synth][stereo]") {
    std::ranges::for_each(std::views::iota(0, 8), [](int typeIdx) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseType = static_cast<NoiseType>(typeIdx);
        settings.noiseStereoWidth = 1.0f;
        settings.noiseDecayMs = 300.0f;
        settings.bodyLevel = 0.0f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 0.0f;

        HatSynth cloned;
        cloned.prepare(48000.0);
        cloned.copyRandomStateFrom(synth);

        synth.trigger(settings, 1.0f);
        cloned.trigger(settings, 1.0f);

        double sumLL = 0.0;
        double sumRR = 0.0;
        double sumLR = 0.0;
        float cloneDiff = 0.0f;
        std::ranges::for_each(std::views::iota(0, 4096), [&](int idx) {
            const auto frame = synth.processSampleLayers(settings).air;
            const auto cloneFrame = cloned.processSampleLayers(settings).air;
            cloneDiff = std::max(cloneDiff, std::max(std::abs(frame.left - cloneFrame.left),
                                                     std::abs(frame.right - cloneFrame.right)));
            if (idx >= 128) {
                sumLL += static_cast<double>(frame.left) * frame.left;
                sumRR += static_cast<double>(frame.right) * frame.right;
                sumLR += static_cast<double>(frame.left) * frame.right;
            }
        });
        const double correlation = sumLR / std::max(std::sqrt(sumLL * sumRR), 1.0e-12);
        CAPTURE(typeIdx, correlation, cloneDiff);
        CHECK(cloneDiff < 1.0e-6f);
        CHECK(std::abs(correlation) < 0.20);
    });

    // BronzeCymbal L and R use detuned partial matrices so stereo mid/side energy evolves over the tail.
    HatSynth bronze;
    bronze.prepare(48000.0);
    HatSynthSettings bronzeSettings;
    bronzeSettings.noiseType = NoiseType::BronzeCymbal;
    bronzeSettings.hissDensity = 0.92f;
    bronzeSettings.noiseDecayMs = 900.0f;
    bronzeSettings.noiseStereoWidth = 1.0f;
    bronzeSettings.bodyLevel = 0.0f;
    bronzeSettings.chickLevel = 0.0f;
    bronzeSettings.preChickLevel = 0.0f;
    bronzeSettings.reverbMix = 0.0f;
    bronze.trigger(bronzeSettings, 1.0f);

    std::vector<StereoFrame> frames(8192);
    std::ranges::for_each(std::views::iota(size_t{0}, frames.size()), [&](size_t idx) {
        frames[idx] = bronze.processSampleLayers(bronzeSettings).air;
    });
    const auto binSideRatio = [&](size_t start) {
        constexpr int nfft = 2048;
        constexpr double twoPi = 6.28318530717958647692;
        double maxMid = 0.0;
        int peakBin = 40;
        std::ranges::for_each(std::views::iota(35, 160), [&](int bin) {
            double re = 0.0;
            double im = 0.0;
            std::ranges::for_each(std::views::iota(0, nfft), [&](int n) {
                const double mid = 0.5 * static_cast<double>(frames[start + static_cast<size_t>(n)].left +
                                                             frames[start + static_cast<size_t>(n)].right);
                const double angle = twoPi * static_cast<double>(bin * n) / static_cast<double>(nfft);
                re += mid * std::cos(angle);
                im -= mid * std::sin(angle);
            });
            const double mag = re * re + im * im;
            if (mag > maxMid) {
                maxMid = mag;
                peakBin = bin;
            }
        });
        double midRe = 0.0, midIm = 0.0, sideRe = 0.0, sideIm = 0.0;
        std::ranges::for_each(std::views::iota(0, nfft), [&](int n) {
            const auto fr = frames[start + static_cast<size_t>(n)];
            const double mid = 0.5 * static_cast<double>(fr.left + fr.right);
            const double side = 0.5 * static_cast<double>(fr.left - fr.right);
            const double angle = twoPi * static_cast<double>(peakBin * n) / static_cast<double>(nfft);
            const double cs = std::cos(angle);
            const double sn = std::sin(angle);
            midRe += mid * cs;
            midIm -= mid * sn;
            sideRe += side * cs;
            sideIm -= side * sn;
        });
        const double midPower = midRe * midRe + midIm * midIm;
        const double sidePower = sideRe * sideRe + sideIm * sideIm;
        return sidePower / std::max(midPower + sidePower, 1.0e-12);
    };
    const double earlySideRatio = binSideRatio(512);
    const double lateSideRatio = binSideRatio(5600);
    CAPTURE(earlySideRatio, lateSideRatio);
    CHECK(std::abs(lateSideRatio - earlySideRatio) > 0.03);
}

TEST_CASE("Pre-warped ZDF TPT SVF preserves high-frequency hat band near Nyquist at 44.1 kHz", "[core][synth][svf]") {
    const auto highSkirtRatio = [](double sr, size_t count) {
        HatSynth synth;
        synth.prepare(sr);
        HatSynthSettings settings;
        settings.noiseType = NoiseType::CrispWhite;
        settings.hissColor = 1.0f;
        settings.hissDensity = 1.0f;
        settings.noiseHPF = 11500.0f;
        settings.noiseLPF = 19500.0f;
        settings.noiseDecayMs = 900.0f;
        settings.bodyLevel = 0.0f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 0.0f;
        settings.hissDrive = 0.0f;
        synth.trigger(settings, 1.0f);

        std::vector<float> buf(count, 0.0f);
        std::ranges::for_each(std::views::iota(size_t{0}, count), [&](size_t idx) {
            buf[idx] = synth.processSampleLayers(settings).air.left;
        });

        const int nfft = static_cast<int>(count / 2);
        const size_t offset = count / 4;
        constexpr double twoPi = 6.28318530717958647692;
        double centerPower = 0.0;
        double upperSkirtPower = 0.0;
        std::ranges::for_each(std::views::iota(1, nfft / 2), [&](int bin) {
            const double hz = static_cast<double>(bin) * sr / static_cast<double>(nfft);
            if (hz < 11000.0 || hz > 19200.0) return;
            double re = 0.0;
            double im = 0.0;
            std::ranges::for_each(std::views::iota(0, nfft), [&](int n) {
                const double angle = twoPi * static_cast<double>(bin * n) / static_cast<double>(nfft);
                const double val = static_cast<double>(buf[offset + static_cast<size_t>(n)]);
                re += val * std::cos(angle);
                im -= val * std::sin(angle);
            });
            const double power = re * re + im * im;
            if (hz >= 11500.0 && hz <= 13500.0) centerPower += power;
            if (hz >= 16500.0 && hz <= 19000.0) upperSkirtPower += power;
        });
        return upperSkirtPower / std::max(centerPower, 1.0e-12);
    };

    const double ratio44k = highSkirtRatio(44100.0, 4096);
    const double ratio176k = highSkirtRatio(176400.0, 16384);
    CAPTURE(ratio44k, ratio176k);
    CHECK(ratio44k > ratio176k * 0.42);
}

TEST_CASE("AnalogMetal 4x oversampled PolyBLEP matrix keeps ring-mod sum tones from folding back", "[core][synth][metal]") {
    const auto renderSpectrum = [](double sr, size_t nfft, const HatSynth& seedSource) {
        HatSynth synth;
        synth.prepare(sr);
        synth.copyRandomStateFrom(seedSource);
        HatSynthSettings settings;
        settings.noiseType = NoiseType::AnalogMetal;
        settings.hissColor = 0.95f;
        settings.hissDensity = 0.0f;
        settings.noiseDecayMs = 900.0f;
        settings.noiseHPF = 150.0f;
        settings.noiseLPF = 10000.0f;
        settings.bodyLevel = 0.0f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 0.0f;
        settings.hissDrive = 0.0f;
        synth.trigger(settings, 1.0f);

        const size_t skip = nfft / 16;
        std::ranges::for_each(std::views::iota(size_t{0}, skip), [&](size_t) {
            [[maybe_unused]] const auto warm = synth.processSampleLayers(settings);
        });
        std::vector<float> buf(nfft, 0.0f);
        std::ranges::for_each(std::views::iota(size_t{0}, nfft), [&](size_t idx) {
            buf[idx] = synth.processSampleLayers(settings).air.left;
        });

        // Both renders use 43 Hz bins. Sum 4 bins per band over 2.1..8.6 kHz.
        constexpr double twoPi = 6.28318530717958647692;
        constexpr int binScale = 4;
        constexpr int firstBand = 12;
        constexpr int lastBand = 50;
        std::vector<double> mags(static_cast<size_t>(lastBand - firstBand), 0.0);
        double norm = 0.0;
        std::ranges::for_each(std::views::iota(firstBand * binScale, lastBand * binScale), [&](int bin) {
            double re = 0.0;
            double im = 0.0;
            std::ranges::for_each(std::views::iota(0, static_cast<int>(nfft)), [&](int n) {
                const double window = 0.5 - 0.5 * std::cos(twoPi * static_cast<double>(n) / static_cast<double>(nfft - 1));
                const double angle = twoPi * static_cast<double>(bin * n) / static_cast<double>(nfft);
                const double val = static_cast<double>(buf[static_cast<size_t>(n)]) * window;
                re += val * std::cos(angle);
                im -= val * std::sin(angle);
            });
            const double power = re * re + im * im;
            mags[static_cast<size_t>(bin / binScale - firstBand)] += power;
            norm += power;
        });
        const double invNorm = 1.0 / std::max(std::sqrt(norm), 1.0e-12);
        std::ranges::for_each(mags, [invNorm](double& val) { val = std::sqrt(val) * invNorm; });
        return mags;
    };

    // At 22.05 kHz, 1x ring-mod sum tones fold into this band. The 176.4 kHz render is the alias-free reference.
    HatSynth seed;
    seed.prepare(22050.0);
    const auto specLow = renderSpectrum(22050.0, 512, seed);
    const auto specRef = renderSpectrum(176400.0, 4096, seed);
    double cosineSim = 0.0;
    std::ranges::for_each(std::views::iota(size_t{0}, specLow.size()), [&](size_t band) {
        cosineSim += specLow[band] * specRef[band];
    });
    CAPTURE(cosineSim);
    CHECK(cosineSim > 0.92);
}

TEST_CASE("BronzeCymbal ZDF modal bank damps high partials faster than fundamental modes", "[core][synth][bronze]") {
    HatSynth synth;
    synth.prepare(48000.0);
    HatSynthSettings settings;
    settings.noiseType = NoiseType::BronzeCymbal;
    settings.hissDensity = 0.92f;
    settings.noiseDecayMs = 900.0f;
    settings.noiseHPF = 300.0f;
    settings.noiseLPF = 19000.0f;
    settings.bodyLevel = 0.0f;
    settings.chickLevel = 0.0f;
    settings.preChickLevel = 0.0f;
    settings.reverbMix = 0.0f;
    settings.hissDrive = 0.0f;
    synth.trigger(settings, 1.0f);

    std::vector<float> audio(10000, 0.0f);
    std::ranges::for_each(std::views::iota(size_t{0}, audio.size()), [&](size_t idx) {
        audio[idx] = synth.processSampleLayers(settings).air.left;
    });

    const auto bandPeakiness = [&](size_t start, double minHz, double maxHz) {
        constexpr int nfft = 2048;
        constexpr double twoPi = 6.28318530717958647692;
        std::vector<double> mags;
        std::ranges::for_each(std::views::iota(1, nfft / 2), [&](int bin) {
            const double hz = static_cast<double>(bin) * 48000.0 / static_cast<double>(nfft);
            if (hz < minHz || hz > maxHz) return;
            double re = 0.0;
            double im = 0.0;
            std::ranges::for_each(std::views::iota(0, nfft), [&](int n) {
                const double window = 0.5 - 0.5 * std::cos(twoPi * static_cast<double>(n) / static_cast<double>(nfft - 1));
                const double val = static_cast<double>(audio[start + static_cast<size_t>(n)]) * window;
                const double angle = twoPi * static_cast<double>(bin * n) / static_cast<double>(nfft);
                re += val * std::cos(angle);
                im -= val * std::sin(angle);
            });
            mags.push_back(std::sqrt(re * re + im * im));
        });
        std::sort(mags.begin(), mags.end());
        return mags.back() / (mags[mags.size() / 2] + 1.0e-12);
    };

    const double earlyHighPeakiness = bandPeakiness(240, 5500.0, 13000.0);
    const double lateHighPeakiness = bandPeakiness(4800, 5500.0, 13000.0);
    const double lateLowPeakiness = bandPeakiness(4800, 700.0, 3200.0);
    CAPTURE(earlyHighPeakiness, lateHighPeakiness, lateLowPeakiness);
    CHECK(lateLowPeakiness > 8.0);
    CHECK(lateHighPeakiness < earlyHighPeakiness * 0.55);
}

TEST_CASE("Energy-storing cymbal shell resonators ring with distinct modes on the strike transient", "[core][synth][body]") {
    HatSynth synth;
    synth.prepare(48000.0);
    HatSynthSettings settings;
    settings.noiseLevel = 0.0f;
    settings.bodyLevel = 1.0f;
    settings.bodyTone = 0.45f;
    settings.bodyDecayMs = 280.0f;
    settings.noiseHPF = 120.0f;
    settings.chickLevel = 0.0f;
    settings.preChickLevel = 0.0f;
    settings.reverbMix = 0.0f;
    settings.hissDrive = 0.0f;
    synth.trigger(settings, 1.0f);

    std::vector<float> shell(4096, 0.0f);
    std::ranges::for_each(std::views::iota(size_t{0}, shell.size()), [&](size_t idx) {
        shell[idx] = synth.processSampleLayers(settings).air.left;
    });

    constexpr int nfft = 2048;
    constexpr size_t start = 720;
    constexpr double twoPi = 6.28318530717958647692;
    std::vector<double> mags;
    std::ranges::for_each(std::views::iota(1, nfft / 2), [&](int bin) {
        const double hz = static_cast<double>(bin) * 48000.0 / static_cast<double>(nfft);
        if (hz < 800.0 || hz > 4500.0) return;
        double re = 0.0;
        double im = 0.0;
        std::ranges::for_each(std::views::iota(0, nfft), [&](int n) {
            const double window = 0.5 - 0.5 * std::cos(twoPi * static_cast<double>(n) / static_cast<double>(nfft - 1));
            const double val = static_cast<double>(shell[start + static_cast<size_t>(n)]) * window;
            const double angle = twoPi * static_cast<double>(bin * n) / static_cast<double>(nfft);
            re += val * std::cos(angle);
            im -= val * std::sin(angle);
        });
        mags.push_back(std::sqrt(re * re + im * im));
    });
    std::sort(mags.begin(), mags.end());
    const double shellPeakiness = mags.back() / (mags[mags.size() / 2] + 1.0e-12);
    CAPTURE(shellPeakiness);
    CHECK(shellPeakiness > 15.0);
}

TEST_CASE("CombWash fractional interpolation resolves sub-sample delay shifts cleanly", "[core][synth][comb]") {
    const auto renderComb = [](float color) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseType = NoiseType::CombWash;
        settings.hissColor = color;
        settings.hissDensity = 0.95f;
        settings.bodyLevel = 0.0f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 0.0f;
        settings.hissDrive = 0.0f;
        synth.trigger(settings, 1.0f);
        std::vector<float> out(2048, 0.0f);
        std::ranges::for_each(std::views::iota(size_t{0}, out.size()), [&](size_t idx) {
            out[idx] = synth.processSampleLayers(settings).air.left;
        });
        return out;
    };

    const auto combA = renderComb(0.0010f);
    const auto combB = renderComb(0.0022f);
    double diffSq = 0.0;
    std::ranges::for_each(std::views::iota(size_t{256}, combA.size()), [&](size_t idx) {
        const double delta = static_cast<double>(combA[idx] - combB[idx]);
        diffSq += delta * delta;
    });
    const double rmsDiff = std::sqrt(diffSq / static_cast<double>(combA.size() - 256));
    CAPTURE(rmsDiff);
    CHECK(rmsDiff > 0.004);

    // A short comb burst damps HF faster than low-mid recirculations inside the feedback loop.
    HatSynth dampedComb;
    dampedComb.prepare(48000.0);
    HatSynthSettings burstSettings;
    burstSettings.noiseType = NoiseType::CombWash;
    burstSettings.hissColor = 0.0f;
    burstSettings.hissDensity = 1.0f;
    burstSettings.noiseDecayMs = 20.0f;
    burstSettings.noiseHPF = 200.0f;
    burstSettings.noiseLPF = 19500.0f;
    burstSettings.bodyLevel = 0.0f;
    burstSettings.chickLevel = 0.0f;
    burstSettings.preChickLevel = 0.0f;
    burstSettings.reverbMix = 0.0f;
    burstSettings.hissDrive = 0.0f;
    dampedComb.trigger(burstSettings, 1.0f);

    std::vector<float> burst(4096, 0.0f);
    std::ranges::for_each(std::views::iota(size_t{0}, burst.size()), [&](size_t idx) {
        burst[idx] = dampedComb.processSampleLayers(burstSettings).air.left;
    });
    const double earlyCentroid = spectralCentroid(burst, 48000.0, 64);
    const double lateCentroid = spectralCentroid(burst, 48000.0, 1800);
    CAPTURE(earlyCentroid, lateCentroid);
    CHECK(earlyCentroid > lateCentroid + 1200.0);
}

TEST_CASE("Jot FDN gated reverb uses S-curve smoothstep gating and click-free fractional modulation", "[core][synth][reverb]") {
    constexpr double sampleRate = 48000.0;
    GatedStereoReverb ungated;
    GatedStereoReverb gated;
    ungated.prepare(sampleRate);
    gated.prepare(sampleRate);

    [[maybe_unused]] const auto u0 = ungated.process(1.0f, -0.8f, 0.55f, 1800.0f, 0.35f, 0.0f, sampleRate);
    [[maybe_unused]] const auto g0 = gated.process(1.0f, -0.8f, 0.55f, 1800.0f, 0.35f, 40.0f, sampleRate);

    constexpr int gateStart = 1920; // 40 ms at 48 kHz
    constexpr int releaseLen = 720; // 15 ms at 48 kHz
    float earlyGain = -1.0f;
    float lateGain = -1.0f;

    std::ranges::for_each(std::views::iota(1, gateStart + releaseLen), [&](int step) {
        const auto ref = ungated.process(0.0f, 0.0f, 0.55f, 1800.0f, 0.35f, 0.0f, sampleRate);
        const auto cur = gated.process(0.0f, 0.0f, 0.55f, 1800.0f, 0.35f, 40.0f, sampleRate);
        const int over = step + 1 - gateStart;
        if (over >= 95 && over <= 125 && earlyGain < 0.0f && std::abs(ref.left) > 1.0e-4f)
            earlyGain = cur.left / ref.left;
        if (over >= 595 && over <= 625 && lateGain < 0.0f && std::abs(ref.left) > 1.0e-4f)
            lateGain = cur.left / ref.left;
    });

    CAPTURE(earlyGain, lateGain);
    REQUIRE(earlyGain > 0.0f);
    REQUIRE(lateGain >= 0.0f);
    CHECK(earlyGain > 0.90f);
    CHECK(lateGain < 0.10f);
}

TEST_CASE("Fractional MIDI key-tracking tunes metallic matrix, bronze modes, and cymbal shell modes", "[core][synth][keytrack]") {
    const auto centroidForNote = [](NoiseType type, float noiseLevel, float bodyLevel, float density,
                                    float keyTracking, int midiNote) {
        HatSynth synth;
        synth.prepare(48000.0);
        HatSynthSettings settings;
        settings.noiseType = type;
        settings.noiseLevel = noiseLevel;
        settings.bodyLevel = bodyLevel;
        settings.bodyTone = 0.35f;
        settings.bodyDecayMs = 250.0f;
        settings.hissDensity = density;
        settings.noiseHPF = 180.0f;
        settings.noiseLPF = 19500.0f;
        settings.chickLevel = 0.0f;
        settings.preChickLevel = 0.0f;
        settings.reverbMix = 0.0f;
        settings.hissDrive = 0.0f;
        settings.keyTracking = keyTracking;
        synth.triggerMidi(settings, 1.0f, midiNote);

        std::vector<float> audio(2048, 0.0f);
        std::ranges::for_each(std::views::iota(size_t{0}, audio.size()), [&](size_t idx) {
            audio[idx] = synth.processSampleLayers(settings).air.left;
        });
        return spectralCentroid(audio, 48000.0, 128);
    };

    const double metalLow = centroidForNote(NoiseType::AnalogMetal, 0.9f, 0.0f, 0.6f, 1.0f, 48);
    const double metalHalf = centroidForNote(NoiseType::AnalogMetal, 0.9f, 0.0f, 0.6f, 0.5f, 72);
    const double metalHigh = centroidForNote(NoiseType::AnalogMetal, 0.9f, 0.0f, 0.6f, 1.0f, 72);
    CAPTURE(metalLow, metalHalf, metalHigh);
    CHECK(metalHalf > metalLow + 200.0);
    CHECK(metalHigh > metalHalf + 200.0);

    const double bronzeLow = centroidForNote(NoiseType::BronzeCymbal, 0.9f, 0.0f, 0.90f, 1.0f, 48);
    const double bronzeHigh = centroidForNote(NoiseType::BronzeCymbal, 0.9f, 0.0f, 0.90f, 1.0f, 72);
    CAPTURE(bronzeLow, bronzeHigh);
    CHECK(bronzeHigh > bronzeLow + 800.0);

    const double bodyLow = centroidForNote(NoiseType::CrispWhite, 0.0f, 1.0f, 0.35f, 1.0f, 48);
    const double bodyHigh = centroidForNote(NoiseType::CrispWhite, 0.0f, 1.0f, 0.35f, 1.0f, 72);
    CAPTURE(bodyLow, bodyHigh);
    CHECK(bodyHigh > bodyLow + 400.0);
}

