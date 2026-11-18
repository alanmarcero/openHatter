#include "HatEngine.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace openhatter::core {

namespace {

[[nodiscard]] inline float dbToLin(float db) noexcept {
    return std::pow(10.0f, db / 20.0f);
}

void writeUint16LE(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xff));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
}

void writeUint32LE(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>(v & 0xff));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xff));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xff));
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xff));
}

} // namespace

// ============================================================================
// HatWavCache
// ============================================================================

void HatWavCache::clear() noexcept {
    isLocked_ = false;
    left_.clear();
    right_.clear();
}

void HatWavCache::capture(const std::vector<float>& left, const std::vector<float>& right, double sampleRate) {
    left_ = left;
    right_ = right;
    sampleRate_ = sampleRate;
    isLocked_ = !left_.empty();
}

std::vector<uint8_t> HatWavCache::toWavBytes() const {
    if (!isLocked_ || left_.empty()) {
        return {};
    }

    const uint32_t numFrames = static_cast<uint32_t>(left_.size());
    const uint16_t numChannels = 2;
    const uint32_t sRate = static_cast<uint32_t>(sampleRate_);
    const uint16_t bitsPerSample = 32;
    const uint32_t dataBytes = numFrames * numChannels * sizeof(float);
    const uint32_t chunkSize = 36 + dataBytes;

    std::vector<uint8_t> wav;
    wav.reserve(44 + dataBytes);

    wav.insert(wav.end(), { 'R', 'I', 'F', 'F' });
    writeUint32LE(wav, chunkSize);
    wav.insert(wav.end(), { 'W', 'A', 'V', 'E' });

    wav.insert(wav.end(), { 'f', 'm', 't', ' ' });
    writeUint32LE(wav, 16);
    writeUint16LE(wav, 3);
    writeUint16LE(wav, numChannels);
    writeUint32LE(wav, sRate);
    writeUint32LE(wav, sRate * numChannels * sizeof(float));
    writeUint16LE(wav, numChannels * sizeof(float));
    writeUint16LE(wav, bitsPerSample);

    wav.insert(wav.end(), { 'd', 'a', 't', 'a' });
    writeUint32LE(wav, dataBytes);

    for (size_t i = 0; i < left_.size(); ++i) {
        float sampleL = left_[i];
        float sampleR = (i < right_.size()) ? right_[i] : sampleL;

        uint32_t rawL, rawR;
        std::memcpy(&rawL, &sampleL, sizeof(float));
        std::memcpy(&rawR, &sampleR, sizeof(float));
        writeUint32LE(wav, rawL);
        writeUint32LE(wav, rawR);
    }

    return wav;
}

bool HatWavCache::fromWavBytes(const std::vector<uint8_t>& wavBytes) {
    if (wavBytes.size() < 44) {
        return false;
    }

    if (std::memcmp(wavBytes.data(), "RIFF", 4) != 0 || std::memcmp(wavBytes.data() + 8, "WAVE", 4) != 0) {
        return false;
    }

    size_t offset = 12;
    uint16_t numChannels = 0;
    uint32_t sRate = 48000;
    uint16_t formatTag = 0;
    size_t dataOffset = 0;
    size_t dataSize = 0;

    while (offset + 8 <= wavBytes.size()) {
        char chunkId[5] = { 0 };
        std::memcpy(chunkId, wavBytes.data() + offset, 4);
        uint32_t chunkSize = 0;
        std::memcpy(&chunkSize, wavBytes.data() + offset + 4, 4);
        offset += 8;

        if (std::memcmp(chunkId, "fmt ", 4) == 0 && chunkSize >= 16) {
            std::memcpy(&formatTag, wavBytes.data() + offset, 2);
            std::memcpy(&numChannels, wavBytes.data() + offset + 2, 2);
            std::memcpy(&sRate, wavBytes.data() + offset + 4, 4);
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            dataOffset = offset;
            dataSize = chunkSize;
            break;
        }

        offset += chunkSize;
    }

    if (dataOffset == 0 || numChannels == 0 || (formatTag != 3 && formatTag != 1)) {
        return false;
    }

    sampleRate_ = sRate;
    const size_t bytesPerFrame = numChannels * (formatTag == 3 ? sizeof(float) : 2);
    const size_t numFrames = dataSize / bytesPerFrame;

    left_.resize(numFrames);
    right_.resize(numFrames);

    if (formatTag == 3) {
        const auto* floatPtr = reinterpret_cast<const float*>(wavBytes.data() + dataOffset);
        for (size_t i = 0; i < numFrames; ++i) {
            left_[i] = floatPtr[i * numChannels];
            right_[i] = (numChannels > 1) ? floatPtr[i * numChannels + 1] : left_[i];
        }
    } else {
        const auto* intPtr = reinterpret_cast<const int16_t*>(wavBytes.data() + dataOffset);
        for (size_t i = 0; i < numFrames; ++i) {
            left_[i] = static_cast<float>(intPtr[i * numChannels]) / 32768.0f;
            right_[i] = (numChannels > 1) ? static_cast<float>(intPtr[i * numChannels + 1]) / 32768.0f : left_[i];
        }
    }

    isLocked_ = true;
    return true;
}

// ============================================================================
// HatEngine
// ============================================================================

void HatEngine::prepare(double sampleRate) {
    sampleRate_ = sampleRate;
    synth_.prepare(sampleRate);
    distL_.reset();
    distR_.reset();
    compressor_.prepare(sampleRate);
    limiter_.prepare(sampleRate);
    reversePreviewSynth_.prepare(sampleRate);

    const int maxReverseSamples = static_cast<int>(sampleRate * 2.0);
    reversePreviewL_.assign(maxReverseSamples, 0.0f);
    reversePreviewR_.assign(maxReverseSamples, 0.0f);
    const size_t maxEchoSamples = std::max<size_t>(16, static_cast<size_t>(sampleRate * 2.0));
    echoBufferL_.assign(maxEchoSamples, 0.0f);
    echoBufferR_.assign(maxEchoSamples, 0.0f);
    reset();
}

void HatEngine::reset() noexcept {
    synth_.reset();
    distL_.reset();
    distR_.reset();
    compressor_.reset();
    limiter_.reset();
    std::fill(echoBufferL_.begin(), echoBufferL_.end(), 0.0f);
    std::fill(echoBufferR_.begin(), echoBufferR_.end(), 0.0f);
    echoWriteIndex_ = 0;
    reversePreviewSynth_.reset();
    reversePreviewSamples_ = 0;
    reverseSourceSamplesAvailable_ = 0;
    reversePlayHead_ = -1;
    reverseDuration_ = 0;
    reverseMix_ = 0.5f;
    isReversing_ = false;
    reverseCallbackWorkUnits_ = 0;
    reversePreviewRenderCount_ = 0;
    rollActive_ = false;
    rollFullAdsrActive_ = false;
    rollStep_ = 0;
    nextRollPpq_ = 0.0;
    rollTransportWasPlaying_ = false;
    currentPeak_ = 0.0f;
    currentBeatPhase_ = 0.0f;
    currentHatProgress_ = 1.0f;
    ccVolume_ = 1.0f;
    ccBrightness_ = 1.0f;
}

void HatEngine::lockCurrentHat(const EngineSettings& settings) {
    std::vector<float> dryL, dryR;
    synth_.renderBlock(dryL, dryR, settings.synth);

    std::vector<float> renderedL(dryL.size());
    std::vector<float> renderedR(dryR.size());

    DistortionShaper dL, dR;
    StereoCompressor comp;
    BrickwallLimiter lim;
    comp.prepare(sampleRate_);
    lim.prepare(sampleRate_);

    for (size_t i = 0; i < dryL.size(); ++i) {
        float sL = dryL[i];
        float sR = dryR[i];

        if (settings.effects.distMix > 0.001f) {
            const float wetDistL = dL.processSample(sL, settings.effects.distType, settings.effects.distDrive, settings.effects.distTone, sampleRate_);
            const float wetDistR = dR.processSample(sR, settings.effects.distType, settings.effects.distDrive, settings.effects.distTone, sampleRate_);
            sL = sL * (1.0f - settings.effects.distMix) + wetDistL * settings.effects.distMix;
            sR = sR * (1.0f - settings.effects.distMix) + wetDistR * settings.effects.distMix;
        }

        if (settings.effects.stereoWidth != 1.0f) {
            const float mid = (sL + sR) * 0.5f;
            const float side = (sL - sR) * 0.5f * settings.effects.stereoWidth;
            sL = mid + side;
            sR = mid - side;
        }

        comp.process(sL, sR, settings.effects.compressor);
        lim.process(sL, sR, settings.effects.limiter);

        renderedL[i] = sL;
        renderedR[i] = sR;
    }

    wavCache_.capture(renderedL, renderedR, sampleRate_);
}

void HatEngine::unlockHat() noexcept {
    wavCache_.clear();
}

bool HatEngine::restoreLockedWav(const std::vector<uint8_t>& wavBytes) {
    return wavCache_.fromWavBytes(wavBytes);
}

void HatEngine::triggerHat(
    const EngineSettings& settings,
    double samplePpq,
    double bpm,
    double beatsPerBar,
    float velocity,
    int midiNoteNumber,
    int midiChannel,
    bool reverseCue,
    bool cacheIncomingHit) noexcept {
    triggerSerial_++;
    currentHatProgress_ = 0.0f;

    const bool engageReverse = shouldTriggerReverse(settings.effects.reverseMode, reverseCue, samplePpq, beatsPerBar);
    const bool cacheReversePreview = settings.effects.reverseMode != ReverseSyncMode::Off
        && cacheIncomingHit && (engageReverse || settings.rollEnabled);
    if (cacheReversePreview) {
        reversePreviewSynth_.copyRandomStateFrom(synth_);
        reversePreviewSamples_ = reversePreviewSynth_.renderMidiBlock(
            std::span<float>(reversePreviewL_), std::span<float>(reversePreviewR_),
            settings.synth, velocity, midiNoteNumber);
        ++reversePreviewRenderCount_;
        reverseCallbackWorkUnits_ += static_cast<uint64_t>(reversePreviewSamples_);
    }
    if (engageReverse && settings.effects.reverseMode != ReverseSyncMode::Off) {
        const int syncSamples = computeSyncedReverseSamples(
            settings.effects.reverseMode, samplePpq, bpm, beatsPerBar, sampleRate_);
        const int maxReverseSamples = static_cast<int>(reversePreviewL_.size());
        reverseDuration_ = std::min(syncSamples, maxReverseSamples);
        reverseSourceSamplesAvailable_ = std::min(reversePreviewSamples_, reverseDuration_);
        reverseMix_ = settings.effects.reverseMix;
        isReversing_ = reverseDuration_ > 0 && reverseSourceSamplesAvailable_ > 0;
        reversePlayHead_ = isReversing_ ? 0 : -1;
        if (isReversing_)
            ++reverseCallbackWorkUnits_;
    }

    synth_.triggerMidi(settings.synth, velocity, midiNoteNumber, midiChannel);
}

void HatEngine::process(
    const EngineSettings& settings,
    const Transport& transport,
    std::span<const MidiTrigger> midiTriggers,
    float* const* channels,
    int numChannels,
    int numSamples,
    std::span<const MidiCcEvent> controlChanges,
    const LayerOutputBuffers* layerOutputs) noexcept {
    if (settings.bypass || numChannels < 2 || channels == nullptr || numSamples <= 0) {
        return;
    }

    double ppq = transport.ppqPosition.value_or(fallbackPpq_);
    const double ppqPerSample = (transport.bpm / 60.0) / sampleRate_;
    const double beatsPerBar = transport.beatsPerBar > 0.0 ? transport.beatsPerBar : 4.0;
    constexpr std::array<double, 3> rollStepLengths { 0.5, 0.25, 0.125 };
    const int divisionIndex = std::clamp(settings.rollDivision, 0, 2);
    const double rollStepPpq = rollStepLengths[static_cast<size_t>(divisionIndex)];
    const double swing = std::clamp(static_cast<double>(settings.rollSwing), 0.0, 0.49);
    if (!settings.rollEnabled)
        rollActive_ = false;
    if (rollActive_ && transport.isPlaying && !rollTransportWasPlaying_) {
        const double nextGrid = (std::floor(ppq / rollStepPpq) + 1.0) * rollStepPpq;
        nextRollPpq_ = nextGrid + (rollStep_ % 2 == 1 ? rollStepPpq * swing : 0.0);
    }
    if (transport.ppqPosition && std::abs(*transport.ppqPosition - fallbackPpq_) > 0.5 && rollActive_) {
        const double nextGrid = (std::floor(ppq / rollStepPpq) + 1.0) * rollStepPpq;
        nextRollPpq_ = nextGrid + (rollStep_ % 2 == 1 ? rollStepPpq * swing : 0.0);
    }
    rollTransportWasPlaying_ = transport.isPlaying;

    int triggerIdx = 0;
    int ccIdx = 0;
    float blockPeak = 0.0f;
    const float outputGain = dbToLin(settings.effects.outputGainDb);

    for (int sample = 0; sample < numSamples; ++sample) {
        while (ccIdx < static_cast<int>(controlChanges.size()) && controlChanges[ccIdx].sampleOffset <= sample) {
            const auto& cc = controlChanges[ccIdx++];
            if (cc.controller == settings.midiChokeCc && cc.value >= 64) {
                synth_.choke();
                rollActive_ = false;
                reversePlayHead_ = -1;
                isReversing_ = false;
            }
            if (cc.controller == 7) {
                ccVolume_ = static_cast<float>(cc.value) / 127.0f;
            } else if (cc.controller == 74) {
                ccBrightness_ = static_cast<float>(cc.value) / 127.0f;
            } else if (cc.controller == 120 || cc.controller == 123) {
                synth_.reset();
                rollActive_ = false;
                reversePlayHead_ = -1;
                isReversing_ = false;
            } else if (cc.controller == 121) {
                ccVolume_ = 1.0f;
                ccBrightness_ = 1.0f;
            }
        }

        while (triggerIdx < static_cast<int>(midiTriggers.size()) && midiTriggers[triggerIdx].sampleOffset <= sample) {
            const auto& trig = midiTriggers[triggerIdx++];
            if (trig.choke) {
                synth_.choke();
                rollActive_ = false;
                reversePlayHead_ = -1;
                isReversing_ = false;
                continue;
            }
            if (trig.noteOff) {
                synth_.noteOff(trig.noteNumber, trig.midiChannel, settings.synth);
                if (rollFullAdsrActive_ && trig.noteNumber == rollNoteNumber_
                    && trig.midiChannel == rollMidiChannel_)
                    rollActive_ = false;
                continue;
            }
            auto triggerSettings = settings;
            if (trig.audition) {
                triggerSettings.synth.fullAdsrEnabled = false;
                triggerSettings.rollEnabled = false;
            }
            triggerHat(triggerSettings, ppq, transport.bpm, beatsPerBar,
                       trig.velocity, trig.noteNumber, trig.midiChannel, trig.audition, true);
            if (!trig.audition && settings.rollEnabled) {
                rollActive_ = true;
                rollNoteNumber_ = trig.noteNumber;
                rollMidiChannel_ = trig.midiChannel;
                rollFullAdsrActive_ = settings.synth.fullAdsrEnabled;
                rollVelocity_ = trig.velocity;
                rollStep_ = 1;
                nextRollPpq_ = (std::floor(ppq / rollStepPpq) + 1.0) * rollStepPpq;
                if (rollStep_ % 2 == 1)
                    nextRollPpq_ += rollStepPpq * swing;
            }
        }

        if (rollActive_ && settings.rollEnabled && transport.isPlaying) {
            int catchupSteps = 0;
            while (ppq + 1.0e-10 >= nextRollPpq_ && catchupSteps++ < 8) {
                const float accent = rollStep_ % 4 == 3
                    ? 1.0f + std::clamp(settings.rollAccent, 0.0f, 1.0f)
                    : 1.0f;
                triggerHat(settings, ppq, transport.bpm, beatsPerBar,
                           std::clamp(rollVelocity_ * accent, 0.0f, 1.0f),
                           rollNoteNumber_, rollMidiChannel_, false, false);
                rollFullAdsrActive_ = settings.synth.fullAdsrEnabled;
                rollStep_ = (rollStep_ + 1) % 4;
                const double interval = rollStepPpq * (rollStep_ % 2 == 1 ? 1.0 + swing : 1.0 - swing);
                nextRollPpq_ += interval;
            }
        }

        const float masterGain = outputGain * ccVolume_;

        float sL = 0.0f;
        float sR = 0.0f;
        if (synth_.isPlaying()) {
            auto activeSettings = settings.synth;
            if (std::abs(ccBrightness_ - 1.0f) > 0.001f) {
                activeSettings.noiseLPF = std::clamp(settings.synth.noiseLPF * ccBrightness_, 800.0f, 20000.0f);
            }
            const auto layers = synth_.processSampleLayers(activeSettings);
            sL = layers.output.left;
            sR = layers.output.right;
            currentHatProgress_ = synth_.getEnvelopeProgress();

            if (layerOutputs != nullptr) {
                if (layerOutputs->metalLeft != nullptr) layerOutputs->metalLeft[sample] = layers.metal.left * masterGain;
                if (layerOutputs->metalRight != nullptr) layerOutputs->metalRight[sample] = layers.metal.right * masterGain;
                if (layerOutputs->airLeft != nullptr) layerOutputs->airLeft[sample] = layers.air.left * masterGain;
                if (layerOutputs->airRight != nullptr) layerOutputs->airRight[sample] = layers.air.right * masterGain;
                if (layerOutputs->chickLeft != nullptr) layerOutputs->chickLeft[sample] = layers.chick.left * masterGain;
                if (layerOutputs->chickRight != nullptr) layerOutputs->chickRight[sample] = layers.chick.right * masterGain;
                if (layerOutputs->reverbLeft != nullptr) layerOutputs->reverbLeft[sample] = layers.reverb.left * masterGain;
                if (layerOutputs->reverbRight != nullptr) layerOutputs->reverbRight[sample] = layers.reverb.right * masterGain;
            }
        } else {
            currentHatProgress_ = 1.0f;
        }

        if (layerOutputs != nullptr && !synth_.isPlaying()) {
            if (layerOutputs->metalLeft != nullptr) layerOutputs->metalLeft[sample] = 0.0f;
            if (layerOutputs->metalRight != nullptr) layerOutputs->metalRight[sample] = 0.0f;
            if (layerOutputs->airLeft != nullptr) layerOutputs->airLeft[sample] = 0.0f;
            if (layerOutputs->airRight != nullptr) layerOutputs->airRight[sample] = 0.0f;
            if (layerOutputs->chickLeft != nullptr) layerOutputs->chickLeft[sample] = 0.0f;
            if (layerOutputs->chickRight != nullptr) layerOutputs->chickRight[sample] = 0.0f;
            if (layerOutputs->reverbLeft != nullptr) layerOutputs->reverbLeft[sample] = 0.0f;
            if (layerOutputs->reverbRight != nullptr) layerOutputs->reverbRight[sample] = 0.0f;
        }

        if (reversePlayHead_ >= 0 && reversePlayHead_ < reverseDuration_) {
            if (reversePlayHead_ < reverseSourceSamplesAvailable_) {
                const int sourceIndex = reverseSourceSamplesAvailable_ - 1 - reversePlayHead_;
                const float ramp = static_cast<float>(reversePlayHead_) / static_cast<float>(reverseDuration_);
                sL += reversePreviewL_[sourceIndex] * ramp * reverseMix_;
                sR += reversePreviewR_[sourceIndex] * ramp * reverseMix_;
            }
            ++reverseCallbackWorkUnits_;
            reversePlayHead_++;
        } else {
            reversePlayHead_ = -1;
            isReversing_ = false;
        }

        if (settings.effects.distMix > 0.001f) {
            const float wetDistL = distL_.processSample(sL, settings.effects.distType, settings.effects.distDrive, settings.effects.distTone, sampleRate_);
            const float wetDistR = distR_.processSample(sR, settings.effects.distType, settings.effects.distDrive, settings.effects.distTone, sampleRate_);
            sL = sL * (1.0f - settings.effects.distMix) + wetDistL * settings.effects.distMix;
            sR = sR * (1.0f - settings.effects.distMix) + wetDistR * settings.effects.distMix;
        }

        if (!echoBufferL_.empty()) {
            const size_t bufSize = echoBufferL_.size();
            const double safeBpm = std::clamp(std::isfinite(transport.bpm) && transport.bpm > 0.0 ? transport.bpm : 138.0, 40.0, 260.0);
            const size_t delaySamples = std::clamp<size_t>(static_cast<size_t>(0.375 * 60.0 / safeBpm * sampleRate_), 1, bufSize - 1);
            const size_t readIdx = (echoWriteIndex_ + bufSize - delaySamples) % bufSize;
            const float delayedL = echoBufferL_[readIdx];
            const float delayedR = echoBufferR_[readIdx];
            const float echoMix = std::clamp(settings.effects.echoMix, 0.0f, 1.0f);
            echoBufferL_[echoWriteIndex_] = std::tanh(sL + delayedR * 0.35f);
            echoBufferR_[echoWriteIndex_] = std::tanh(sR + delayedL * 0.35f);
            echoWriteIndex_ = (echoWriteIndex_ + 1) % bufSize;
            if (echoMix > 0.001f) {
                sL += delayedL * echoMix * 0.65f;
                sR += delayedR * echoMix * 0.65f;
            }
        }

        if (settings.effects.stereoWidth != 1.0f) {
            const float mid = (sL + sR) * 0.5f;
            const float side = (sL - sR) * 0.5f * settings.effects.stereoWidth;
            sL = mid + side;
            sR = mid - side;
        }

        compressor_.process(sL, sR, settings.effects.compressor);
        limiter_.process(sL, sR, settings.effects.limiter);

        sL *= masterGain;
        sR *= masterGain;

        channels[0][sample] = sL;
        channels[1][sample] = sR;

        const float framePeak = std::max(std::abs(sL), std::abs(sR));
        blockPeak = std::max(blockPeak, framePeak);

        ppq += ppqPerSample;
    }

    fallbackPpq_ = ppq;
    currentPeak_ = blockPeak;
    currentBeatPhase_ = transport.isPlaying ? static_cast<float>(ppq - std::floor(ppq)) : 0.0f;
}

} // namespace openhatter::core
