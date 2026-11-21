#pragma once

#include "HatEngine.h"
#include <juce_audio_processors/juce_audio_processors.h>

namespace openhatter::params {

// Stick Bounce / Pre-Chick Articulation
inline constexpr const char* kPreBursts = "pre_bursts";
inline constexpr const char* kPreSpread = "pre_spread";
inline constexpr const char* kPreDecay = "pre_decay";
inline constexpr const char* kPreJitter = "pre_jitter";
inline constexpr const char* kFlamSpread = "flam_spread";
inline constexpr const char* kPreLevel = "pre_level";

// Decay morph, echo and amplitude envelope
inline constexpr const char* kDecayScale = "decay_scale";
inline constexpr const char* kEchoMix = "echo_mix";
inline constexpr const char* kAmpEnabled = "amp_enabled";
inline constexpr const char* kAmpAttack = "amp_attack";
inline constexpr const char* kAmpDecay = "amp_decay";
inline constexpr const char* kAmpSustain = "amp_sustain";
inline constexpr const char* kAmpRelease = "amp_release";

// Air Sizzle & Shimmer Hiss
inline constexpr const char* kNoiseType = "noise_type";
inline constexpr const char* kNoiseAttack = "noise_attack";
inline constexpr const char* kNoiseHold = "noise_hold";
inline constexpr const char* kNoiseDecay = "noise_decay";
inline constexpr const char* kNoiseCurve = "noise_curve";
inline constexpr const char* kNoiseHPF = "noise_hpf";
inline constexpr const char* kNoiseLPF = "noise_lpf";
inline constexpr const char* kNoiseStereoWidth = "noise_stereo_width";
inline constexpr const char* kNoiseLevel = "noise_level";
inline constexpr const char* kVelocityAir = "velocity_air";
inline constexpr const char* kHissColor = "hiss_color";
inline constexpr const char* kHissDensity = "hiss_density";
inline constexpr const char* kHissDrive = "hiss_drive";

// Spun Bronze Cymbal Body
inline constexpr const char* kBodyLevel = "body_level";
inline constexpr const char* kBodyTone = "body_tone";
inline constexpr const char* kBodyDecay = "body_decay";

// Stick Chick / Transient
inline constexpr const char* kClickType = "click_type";
inline constexpr const char* kClickLevel = "click_level";
inline constexpr const char* kClickFreq = "click_freq";
inline constexpr const char* kVelocityChick = "velocity_chick";
inline constexpr const char* kKeyTracking = "key_tracking";

// Shimmer Saturation & Exciter
inline constexpr const char* kDistType = "dist_type";
inline constexpr const char* kDistDrive = "dist_drive";
inline constexpr const char* kDistTone = "dist_tone";
inline constexpr const char* kDistMix = "dist_mix";

// Dynamics: Compressor
inline constexpr const char* kCompThresh = "comp_thresh";
inline constexpr const char* kCompRatio = "comp_ratio";
inline constexpr const char* kCompAttack = "comp_attack";
inline constexpr const char* kCompRelease = "comp_release";
inline constexpr const char* kCompMakeup = "comp_makeup";
inline constexpr const char* kCompMix = "comp_mix";

// Dynamics: Limiter
inline constexpr const char* kLimiterCeiling = "limiter_ceiling";
inline constexpr const char* kLimiterRelease = "limiter_release";
inline constexpr const char* kLimiterEnabled = "limiter_enabled";

// Trance Shimmer Reverb & Choke Gate
inline constexpr const char* kReverbSize = "reverb_size";
inline constexpr const char* kReverbDecay = "reverb_decay";
inline constexpr const char* kReverbDamp = "reverb_damp";
inline constexpr const char* kReverbGate = "reverb_gate";
inline constexpr const char* kReverbMix = "reverb_mix";
inline constexpr const char* kVelocityReverb = "velocity_reverb";

// Reverse Swell & Sync
inline constexpr const char* kReverseMode = "reverse_mode";
inline constexpr const char* kReverseMix = "reverse_mix";

// Global / Master & Output EQ
inline constexpr const char* kLockIn = "lock_in";
inline constexpr const char* kStereoWidth = "stereo_width";
inline constexpr const char* kOutputGain = "output_gain";
inline constexpr const char* kEqLowFreq = "eq_low_freq";
inline constexpr const char* kEqLowDb = "eq_low_db";
inline constexpr const char* kEqMidFreq = "eq_mid_freq";
inline constexpr const char* kEqMidDb = "eq_mid_db";
inline constexpr const char* kEqHighFreq = "eq_high_freq";
inline constexpr const char* kEqHighDb = "eq_high_db";
inline constexpr const char* kPreset = "preset";
inline constexpr const char* kMidiChokeNote = "midi_choke_note";
inline constexpr const char* kMidiChokeCc = "midi_choke_cc";
inline constexpr const char* kRollEnabled = "roll_enabled";
inline constexpr const char* kRollDivision = "roll_division";
inline constexpr const char* kRollSwing = "roll_swing";
inline constexpr const char* kRollAccent = "roll_accent";

// The editor readouts use the same text as the parameters.
juce::String formatMs(float ms);
juce::String formatDb(float db);
juce::String formatHz(float hz);

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
openhatter::core::EngineSettings readSettings(const juce::AudioProcessorValueTreeState& apvts);

} // namespace openhatter::params
