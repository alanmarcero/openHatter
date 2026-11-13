#include "Panels.h"
#include "BinaryData.h"
#include "Parameters.h"
#include "PluginProcessor.h"
#include "Theme.h"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace openhatter::ui {

namespace {

constexpr int kRefreshHz = 60;

} // namespace

// ============================================================================
// HatterStagePanel
// ============================================================================

HatterStagePanel::HatterStagePanel(OpenHatterProcessor& processor)
    : processor_(processor) {
    mascot_ = juce::ImageCache::getFromMemory(BinaryData::madhatter_png, BinaryData::madhatter_pngSize);

    auditionBtn_.setColour(juce::TextButton::buttonColourId, theme::kKnobBody);
    auditionBtn_.setColour(juce::TextButton::textColourOffId, theme::kAccentNeonGreen);
    auditionBtn_.onClick = [this]() {
        processor_.triggerAudition();
    };
    addAndMakeVisible(auditionBtn_);

    startTimerHz(kRefreshHz);
}

HatterStagePanel::~HatterStagePanel() {
    stopTimer();
}

void HatterStagePanel::timerCallback() {
    const uint32_t serial = processor_.displayTriggerCount.load(std::memory_order_relaxed);
    if (serial != seenTriggers_) {
        seenTriggers_ = serial;
        hatTriggerTimeMs_ = juce::Time::getMillisecondCounterHiRes();
        impactIntensity_ = std::clamp(processor_.displayVelocity.load(std::memory_order_relaxed), 0.35f, 1.0f);
    }
    repaint();
}

void HatterStagePanel::resized() {
    auto area = getLocalBounds().reduced(12, 10);
    auditionBtn_.setBounds(area.removeFromBottom(24).reduced(area.getWidth() / 4, 0));
}

void HatterStagePanel::paint(juce::Graphics& g) {
    drawPanel(g, getLocalBounds(), "Mad Hatter Off-Beat Stage", theme::kAccentAmber);

    auto stage = getLocalBounds().reduced(8, 28).withTrimmedBottom(28).toFloat();
    const juce::Graphics::ScopedSaveState saved(g);
    g.reduceClipRegion(stage.toNearestInt());

    // Deep stage background
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff0d1520), stage.getTopLeft(),
        juce::Colour(0xff06090e), stage.getBottomRight(), false));
    g.fillRoundedRectangle(stage, 8.0f);
    g.setColour(theme::kPanelEdge.withAlpha(0.7f));
    g.drawRoundedRectangle(stage.reduced(0.5f), 8.0f, 1.0f);

    const float peak = std::clamp(processor_.displayPeak.load(), 0.0f, 1.0f);
    const float beatPhase = std::clamp(processor_.displayBeatPhase.load(), 0.0f, 1.0f);
    // Emphasize the off-beat (beatPhase around 0.5) for EDM off-beat open hi-hat groove
    const float offBeatDist = std::abs(beatPhase - 0.5f) * 2.0f;
    const float pulse = std::max(peak, 0.35f * std::pow(1.0f - std::min(1.0f, offBeatDist), 2.5f));
    const bool locked = processor_.isHatLockedIn();

    const double hatAgeMs = juce::Time::getMillisecondCounterHiRes() - hatTriggerTimeMs_;
    constexpr double kAnimDurationMs = 520.0;
    const float hatPhase = (hatAgeMs < 0.0 || hatAgeMs > kAnimDurationMs)
        ? 1.0f
        : static_cast<float>(hatAgeMs / kAnimDurationMs);
    const bool isTriggerActive = (hatPhase < 1.0f);

    if (hatPhase < 0.16f) {
        const float flutter = std::sin(hatPhase * 48.0f * std::numbers::pi_v<float>);
        hatPose_ = -(1.0f - hatPhase / 0.16f) * 0.9f + flutter * 0.25f;
        headTilt_ = (hatPhase / 0.16f) * 0.02f;
        shockwave_ = 0.0f;
        impactFlash_ = 0.0f;
    } else if (hatPhase < 0.28f) {
        const float strike = (hatPhase - 0.16f) / 0.12f;
        hatPose_ = -std::sin(strike * std::numbers::pi_v<float>) * 1.0f;
        headTilt_ = 0.0f;
        shockwave_ = (1.0f - strike) * impactIntensity_;
        impactFlash_ = std::pow(1.0f - strike, 1.4f) * impactIntensity_;
    } else if (hatPhase < 1.0f) {
        const float recoil = (hatPhase - 0.28f) / 0.72f;
        const float decay = std::exp(-recoil * 3.8f);
        hatPose_ = std::sin(recoil * std::numbers::pi_v<float> * 3.0f) * decay * 0.85f;
        headTilt_ = std::sin(recoil * std::numbers::pi_v<float> * 2.0f) * decay * 0.035f;
        shockwave_ = std::max(0.0f, (1.0f - recoil * 2.2f) * impactIntensity_);
        impactFlash_ = std::max(0.0f, impactFlash_ * 0.92f);
    } else {
        hatPose_ = 0.0f;
        headTilt_ = 0.0f;
        shockwave_ = 0.0f;
        impactFlash_ = 0.0f;
        idleBob_ = std::sin((beatPhase - 0.25f) * 2.0f * std::numbers::pi_v<float>) * 2.0f;
    }

    const bool isHatOpen = (hatPhase < 0.75f && isTriggerActive);

    // Golden-cyan acoustic cymbal resonator behind the Mad Hatter
    const juce::Point<float> coneCentre{ stage.getCentreX(), stage.getCentreY() + 14.0f };
    const float baseRadius = std::min(stage.getWidth(), stage.getHeight()) * 0.39f;
    const float pumpExtra = (pulse * 12.0f) + (impactFlash_ * 12.0f);
    const float pumpRadius = baseRadius + pumpExtra;

    const juce::Rectangle<float> rimRect(
        coneCentre.x - baseRadius - 6.0f, coneCentre.y - baseRadius - 6.0f,
        (baseRadius + 6.0f) * 2.0f, (baseRadius + 6.0f) * 2.0f);

    const auto haloColor = isHatOpen ? theme::kAccentAmber : theme::kAccentCyan;
    drawGlow(g, rimRect.expanded(18.0f), haloColor.withAlpha(0.18f + pulse * 0.38f + impactFlash_ * 0.35f));
    g.setColour(juce::Colour(0xff18222d));
    g.fillEllipse(rimRect);

    for (int r = 1; r <= 3; ++r) {
        const float rippleR = baseRadius * (0.35f + 0.22f * static_cast<float>(r)) + pumpExtra * 0.35f;
        g.setColour(haloColor.withAlpha((0.30f - static_cast<float>(r) * 0.07f) * (0.35f + pulse + impactFlash_)));
        g.drawEllipse(coneCentre.x - rippleR, coneCentre.y - rippleR, rippleR * 2.0f, rippleR * 2.0f, 1.2f);
    }

    g.setColour(haloColor.withAlpha(0.40f + pulse * 0.45f + impactFlash_ * 0.4f));
    g.drawEllipse(rimRect.reduced(2.0f), 1.5f);

    const juce::Rectangle<float> coneRect(coneCentre.x - pumpRadius, coneCentre.y - pumpRadius, pumpRadius * 2.0f, pumpRadius * 2.0f);
    g.setGradientFill(juce::ColourGradient(
        juce::Colour(0xff090f15), coneCentre,
        juce::Colour(0xff2b3a4a), coneCentre.translated(pumpRadius, 0.0f), true));
    g.fillEllipse(coneRect);

    // Sweeping trance club laser spotlight beams
    for (int side = 0; side < 2; ++side) {
        const float srcX = side == 0 ? stage.getX() + 16.0f : stage.getRight() - 16.0f;
        const float floorY = stage.getBottom() - 4.0f;
        const auto beamCol = side == 0 ? theme::kAccentCyan : theme::kAccentAmber;

        const float sweepPhase = (side == 0 ? beatPhase : beatPhase + 0.5f);
        const float sweepOffset = std::sin(sweepPhase * 2.0f * std::numbers::pi_v<float>) * 24.0f;
        const float targetX = stage.getCentreX() + (isTriggerActive ? (side == 0 ? -12.0f : 12.0f) : sweepOffset);
        const float targetY = stage.getY() + 10.0f;

        juce::Path beam;
        beam.startNewSubPath(srcX - 8.0f, floorY);
        beam.lineTo(targetX - 25.0f, targetY);
        beam.lineTo(targetX + 25.0f, targetY);
        beam.lineTo(srcX + 8.0f, floorY);
        beam.closeSubPath();

        const float beamIntensity = 0.16f + pulse * 0.30f + (impactFlash_ * 0.45f);
        g.setGradientFill(juce::ColourGradient(
            beamCol.withAlpha(beamIntensity), srcX, floorY,
            beamCol.withAlpha(0.0f), targetX, targetY, false));
        g.fillPath(beam);
    }

    float shiftY = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (isTriggerActive) {
        if (hatPhase < 0.28f) {
            const float frac = (1.0f - hatPhase / 0.28f);
            shiftY = 5.0f * impactIntensity_ * frac;
            scaleX = 1.0f + 0.06f * impactIntensity_ * frac;
            scaleY = 1.0f - 0.05f * impactIntensity_ * frac;
        } else {
            shiftY = -hatPose_ * 8.0f * impactIntensity_;
            scaleX = 1.0f - hatPose_ * 0.04f * impactIntensity_;
            scaleY = 1.0f + hatPose_ * 0.04f * impactIntensity_;
        }
    } else {
        shiftY = idleBob_;
    }

    // Draw the 3D Mad Hatter mascot playing open hi-hat cymbals
    if (mascot_.isValid()) {
        const juce::Graphics::ScopedSaveState mascotSave(g);
        const float pivotX = stage.getCentreX();
        const float pivotY = stage.getCentreY() + 20.0f;

        g.addTransform(juce::AffineTransform::translation(-pivotX, -pivotY));
        g.addTransform(juce::AffineTransform::rotation(headTilt_));
        g.addTransform(juce::AffineTransform::scale(scaleX, scaleY));
        g.addTransform(juce::AffineTransform::translation(pivotX, pivotY + shiftY));

        g.setColour(juce::Colours::white);
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
        const auto mascotBounds = stage.withTrimmedTop(26.0f).reduced(6.0f, 4.0f).toNearestInt();
        g.drawImageWithin(mascot_, mascotBounds.getX(), mascotBounds.getY(), mascotBounds.getWidth(), mascotBounds.getHeight(),
                          juce::RectanglePlacement::centred);
    }

    // Expanding cymbal shimmer shockwave rings around the hi-hat cymbals
    if (shockwave_ > 0.01f) {
        const juce::Point<float> cymbalImpact{ stage.getCentreX() + 36.0f, stage.getCentreY() + 34.0f + shiftY };

        const float flareRadius = 22.0f + (1.0f - shockwave_) * 30.0f;
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xffffffff).withAlpha(shockwave_ * 0.85f), cymbalImpact,
            theme::kAccentAmber.withAlpha(0.0f), cymbalImpact.translated(flareRadius, 0.0f), true));
        g.fillEllipse(cymbalImpact.x - flareRadius, cymbalImpact.y - flareRadius * 0.45f, flareRadius * 2.0f, flareRadius * 0.9f);

        const std::array<juce::Colour, 3> ringCols = {
            theme::kAccentAmber,
            theme::kAccentCyan,
            theme::kAccentNeonGreen
        };
        for (size_t w = 0; w < ringCols.size(); ++w) {
            const float r = 16.0f + static_cast<float>(w) * 16.0f + (1.0f - shockwave_) * 54.0f;
            const float ringAlpha = shockwave_ * std::max(0.0f, 0.72f - static_cast<float>(w) * 0.18f);
            g.setColour(ringCols[w].withAlpha(ringAlpha));
            g.drawEllipse(cymbalImpact.x - r, cymbalImpact.y - r * 0.4f, r * 2.0f, r * 0.8f, 2.0f);
        }
    }

    // Top stage banner: "HAT ON. OFF-BEAT."
    const auto bannerRect = stage.removeFromTop(24.0f);
    g.setColour(juce::Colour(0xcc060a0f));
    g.fillRoundedRectangle(bannerRect.reduced(14.0f, 2.0f), 4.0f);

    const auto sloganCol = isHatOpen ? theme::kAccentNeonGreen : theme::kAccentAmber;
    g.setColour(sloganCol);
    g.setFont(juce::FontOptions(12.5f, juce::Font::bold));
    g.drawText("HAT ON. OFF-BEAT.", bannerRect, juce::Justification::centred, true);

    // Neon State Indicator LED
    const auto ledRect = juce::Rectangle<float>(stage.getX() + 14.0f, stage.getY() + 4.0f, 72.0f, 18.0f);
    if (isHatOpen) {
        drawGlow(g, ledRect.expanded(6.0f), theme::kAccentNeonGreen.withAlpha(0.35f + impactFlash_ * 0.4f));
    }
    g.setColour(isHatOpen ? theme::kAccentNeonGreen.withAlpha(0.28f + impactFlash_ * 0.25f) : theme::kKnobBody);
    g.fillRoundedRectangle(ledRect, 3.0f);
    g.setColour(isHatOpen ? theme::kAccentNeonGreen : theme::kPanelEdge);
    g.drawRoundedRectangle(ledRect, 3.0f, 1.0f);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(isHatOpen ? theme::kAccentNeonGreen : theme::kTextDim);
    g.drawText(isHatOpen ? "HAT OPEN" : "OFF-BEAT", ledRect, juce::Justification::centred, true);

    // Tuned note & frequency badge on top-right of stage
    const int noteIdx = static_cast<int>(*processor_.parameters.getRawParameterValue(params::kToneNote));
    const int octIdx = static_cast<int>(*processor_.parameters.getRawParameterValue(params::kToneOctave));
    const float fineCt = *processor_.parameters.getRawParameterValue(params::kToneFine);
    const float tunedHz = core::noteToHz(noteIdx, octIdx, fineCt);
    const auto noteStr = juce::String(core::kNoteNames[static_cast<size_t>(std::clamp(noteIdx, 0, 11))].data());

    const auto keyRect = juce::Rectangle<float>(stage.getRight() - 118.0f, stage.getY() + 4.0f, 104.0f, 18.0f);
    g.setColour(theme::kKnobBody.withAlpha(0.9f));
    g.fillRoundedRectangle(keyRect, 3.0f);
    g.setColour(locked ? theme::kAccentAmber : theme::kAccentCyan.withAlpha(0.7f));
    g.drawRoundedRectangle(keyRect, 3.0f, 1.0f);
    g.setColour(locked ? theme::kAccentAmber : theme::kAccentCyan);
    g.drawText(noteStr + juce::String(octIdx) + " (" + juce::String(tunedHz, 0) + " Hz)",
               keyRect, juce::Justification::centred, true);
}

// ============================================================================
// HatSynthPanel
// ============================================================================

HatSynthPanel::HatSynthPanel(OpenHatterProcessor& processor)
    : preBursts_(processor.parameters, openhatter::params::kPreBursts, "Bounces", theme::kAccentCyan),
      preSpread_(processor.parameters, openhatter::params::kPreSpread, "Spread", theme::kAccentCyan),
      preDecay_(processor.parameters, openhatter::params::kPreDecay, "Bounce Decay", theme::kAccentCyan),
      preJitter_(processor.parameters, openhatter::params::kPreJitter, "Humanize", theme::kAccentCyan),
      flamSpread_(processor.parameters, openhatter::params::kFlamSpread, "Bounce Pan", theme::kAccentCyan),
      preLevel_(processor.parameters, openhatter::params::kPreLevel, "Bounce Level", theme::kAccentCyan),
      toneMode_(processor.parameters, openhatter::params::kToneMode, "Cymbal Mode", theme::kAccentAmber),
      toneNote_(processor.parameters, openhatter::params::kToneNote, "Root Note", theme::kAccentAmber),
      toneOctave_(processor.parameters, openhatter::params::kToneOctave, "Octave", theme::kAccentAmber),
      toneFine_(processor.parameters, openhatter::params::kToneFine, "Fine Tune", theme::kAccentAmber),
      lockPitchBtn_(processor.parameters, openhatter::params::kLockPitch, "LOCK PITCH", theme::kAccentAmber),
      toneFreq_(processor.parameters, openhatter::params::kToneFreq, "Bell Freq", theme::kAccentAmber),
      toneQ_(processor.parameters, openhatter::params::kToneQ, "Bell Q", theme::kAccentAmber),
      toneDecay_(processor.parameters, openhatter::params::kToneDecay, "Metal Decay", theme::kAccentAmber),
      tonePitchEnv_(processor.parameters, openhatter::params::kTonePitchEnv, "Pitch Chick", theme::kAccentAmber),
      tonePitchDecay_(processor.parameters, openhatter::params::kTonePitchDecay, "Pitch Decay", theme::kAccentAmber),
      toneLevel_(processor.parameters, openhatter::params::kToneLevel, "Metal Level", theme::kAccentAmber),
      ampAttack_(processor.parameters, openhatter::params::kAmpAttack, "Attack", theme::kAccentAmber),
      ampDecay_(processor.parameters, openhatter::params::kAmpDecay, "Decay", theme::kAccentAmber),
      ampSustain_(processor.parameters, openhatter::params::kAmpSustain, "Sustain", theme::kAccentAmber),
      ampRelease_(processor.parameters, openhatter::params::kAmpRelease, "Release", theme::kAccentAmber),
      noiseType_(processor.parameters, openhatter::params::kNoiseType, "Air Texture", theme::kAccentViolet),
      noiseAttack_(processor.parameters, openhatter::params::kNoiseAttack, "Air Attack", theme::kAccentViolet),
      noiseHold_(processor.parameters, openhatter::params::kNoiseHold, "Air Hold", theme::kAccentViolet),
      noiseDecay_(processor.parameters, openhatter::params::kNoiseDecay, "Air Decay", theme::kAccentViolet),
      noiseCurve_(processor.parameters, openhatter::params::kNoiseCurve, "Tail Curve", theme::kAccentViolet),
      noiseHPF_(processor.parameters, openhatter::params::kNoiseHPF, "Air HPF", theme::kAccentViolet),
      noiseLPF_(processor.parameters, openhatter::params::kNoiseLPF, "Air LPF", theme::kAccentViolet),
      noiseWidth_(processor.parameters, openhatter::params::kNoiseStereoWidth, "Air Width", theme::kAccentViolet),
      noiseLevel_(processor.parameters, openhatter::params::kNoiseLevel, "Air Level", theme::kAccentViolet),
      clickType_(processor.parameters, openhatter::params::kClickType, "Chick Type", theme::kAccentNeonGreen),
      clickLevel_(processor.parameters, openhatter::params::kClickLevel, "Chick Level", theme::kAccentNeonGreen),
      clickFreq_(processor.parameters, openhatter::params::kClickFreq, "Chick Freq", theme::kAccentNeonGreen),
      articulationLock_("Articulation", theme::kAccentCyan,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::articulation); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::articulation, on); }),
      cymbalLock_("Cymbal & Tuning", theme::kAccentAmber,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::cymbalAndTuning); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::cymbalAndTuning, on); }),
      airLock_("Air", theme::kAccentViolet,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::air); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::air, on); }) {
    for (juce::Component* component : std::array<juce::Component*, 40>{
             &preBursts_, &preSpread_, &preDecay_, &preJitter_, &flamSpread_, &preLevel_,
             &toneMode_, &toneNote_, &toneOctave_, &toneFine_, &lockPitchBtn_, &toneFreq_, &toneQ_,
             &toneDecay_, &tonePitchEnv_, &tonePitchDecay_, &toneLevel_, &ampAttack_, &ampDecay_,
             &ampSustain_, &ampRelease_, &noiseType_, &noiseAttack_, &noiseHold_, &noiseDecay_,
             &noiseCurve_, &noiseHPF_, &noiseLPF_, &noiseWidth_, &noiseLevel_, &clickType_,
             &clickLevel_, &clickFreq_, &articulationLock_, &cymbalLock_, &airLock_, nullptr, nullptr, nullptr, nullptr})
        if (component != nullptr) addAndMakeVisible(component);
    setAdvancedMode(false);
}

void HatSynthPanel::setPage(int page) {
    page_ = std::clamp(page, 0, 1);
    setAdvancedMode(advancedMode_);
}

void HatSynthPanel::setAdvancedMode(bool advanced) {
    advancedMode_ = advanced;
    const bool hat = page_ == 0;
    const auto show = [hat](juce::Component& component) {
        component.setVisible(hat);
    };
    show(toneMode_); show(toneNote_); show(toneOctave_); show(lockPitchBtn_); show(toneDecay_);
    show(toneLevel_); show(ampAttack_); show(ampDecay_); show(ampSustain_); show(ampRelease_);
    show(clickType_); show(clickLevel_);
    for (juce::Component* component : std::array<juce::Component*, 6>{
             &toneFine_, &toneFreq_, &toneQ_, &tonePitchEnv_, &tonePitchDecay_, &clickFreq_})
        component->setVisible(hat && advancedMode_);
    const bool motion = !hat;
    for (juce::Component* component : std::array<juce::Component*, 6>{
             &preBursts_, &preSpread_, &preDecay_, &preJitter_, &flamSpread_, &preLevel_})
        component->setVisible(motion && (advancedMode_ || component == &preBursts_ || component == &preSpread_ || component == &preLevel_));
    for (juce::Component* component : std::array<juce::Component*, 9>{
             &noiseType_, &noiseAttack_, &noiseHold_, &noiseDecay_, &noiseCurve_, &noiseHPF_, &noiseLPF_, &noiseWidth_, &noiseLevel_})
        component->setVisible(motion && (advancedMode_ || component == &noiseType_ || component == &noiseDecay_
                                         || component == &noiseWidth_ || component == &noiseLevel_));
    articulationLock_.setVisible(true);
    cymbalLock_.setVisible(hat);
    airLock_.setVisible(motion);
    resized();
    repaint();
}

void HatSynthPanel::paint(juce::Graphics& g) {
    drawPanel(g, getLocalBounds(), page_ == 0 ? (advancedMode_ ? "HAT · CYMBAL BODY & ADSR" : "HAT · PRIMARY SOUND")
                                                : (advancedMode_ ? "MOTION · ARTICULATION & AIR" : "MOTION · OFF-BEAT SHAPE"),
              page_ == 0 ? theme::kAccentCyan : theme::kAccentViolet);
    for (size_t i = 0; i < sectionBounds_.size(); ++i) {
        const auto bounds = sectionBounds_[i].toFloat().reduced(1.0f);
        if (bounds.isEmpty()) continue;
        const auto accent = i == 0 ? theme::kAccentAmber : (i == 1 ? theme::kAccentViolet : theme::kAccentNeonGreen);
        g.setColour(theme::kPanel.brighter(0.035f));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(accent.withAlpha(0.72f));
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
        juce::String title;
        if (page_ == 0) title = i == 0 ? "TUNED METAL & KEY" : (i == 1 ? "ONE-SHOT ADSR" : "CHICK TRANSIENT");
        else title = i == 0 ? "STICK BOUNCES" : "AIR TAIL & WIDTH";
        g.setColour(accent);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(title, sectionBounds_[i].reduced(12, 0).withHeight(20).translated(0, 5), juce::Justification::centredLeft);
    }
}

void HatSynthPanel::resized() {
    auto area = getLocalBounds().reduced(12, 30);
    sectionBounds_.fill({});
    if (page_ == 0) {
        sectionBounds_[0] = area.removeFromTop(310).reduced(0, 3);
        area.removeFromTop(8);
        sectionBounds_[1] = area.removeFromTop(205).reduced(0, 3);
        area.removeFromTop(8);
        sectionBounds_[2] = area.reduced(0, 3);
        cymbalLock_.setBounds(sectionBounds_[0].removeFromRight(88).withY(sectionBounds_[0].getY() + 5).withHeight(22));
        articulationLock_.setBounds(sectionBounds_[2].removeFromRight(88).withY(sectionBounds_[2].getY() + 5).withHeight(22));
        auto metal = sectionBounds_[0].reduced(8, 28);
        auto pitchRow = metal.removeFromTop(108);
        if (advancedMode_)
            layoutRow(pitchRow, { &toneMode_, &toneNote_, &toneOctave_, &toneFine_, &lockPitchBtn_, &toneFreq_ });
        else
            layoutRow(pitchRow, { &toneMode_, &toneNote_, &toneOctave_, &lockPitchBtn_ });
        if (advancedMode_)
            layoutRow(metal.reduced(0, 2), { &toneQ_, &toneDecay_, &tonePitchEnv_, &tonePitchDecay_, &toneLevel_ });
        else
            layoutRow(metal.reduced(0, 2), { &toneDecay_, &toneLevel_ });
        layoutRow(sectionBounds_[1].reduced(8, 34), { &ampAttack_, &ampDecay_, &ampSustain_, &ampRelease_ });
        if (advancedMode_)
            layoutRow(sectionBounds_[2].reduced(8, 34), { &clickType_, &clickLevel_, &clickFreq_ });
        else
            layoutRow(sectionBounds_[2].reduced(8, 34), { &clickType_, &clickLevel_ });
        return;
    }

    sectionBounds_[0] = area.removeFromTop(270).reduced(0, 3);
    area.removeFromTop(8);
    sectionBounds_[1] = area.reduced(0, 3);
    articulationLock_.setBounds(sectionBounds_[0].removeFromRight(88).withY(sectionBounds_[0].getY() + 5).withHeight(22));
    airLock_.setBounds(sectionBounds_[1].removeFromRight(88).withY(sectionBounds_[1].getY() + 5).withHeight(22));
    if (advancedMode_)
        layoutRow(sectionBounds_[0].reduced(8, 42), { &preBursts_, &preSpread_, &preDecay_, &preJitter_, &flamSpread_, &preLevel_ });
    else
        layoutRow(sectionBounds_[0].reduced(8, 42), { &preBursts_, &preSpread_, &preLevel_ });
    auto airArea = sectionBounds_[1].reduced(8, 32);
    if (advancedMode_) {
        layoutRow(airArea.removeFromTop(126), { &noiseType_, &noiseAttack_, &noiseHold_, &noiseDecay_, &noiseCurve_ });
        layoutRow(airArea, { &noiseHPF_, &noiseLPF_, &noiseWidth_, &noiseLevel_ });
    } else {
        layoutRow(airArea.removeFromTop(126), { &noiseType_, &noiseDecay_ });
        layoutRow(airArea, { &noiseWidth_, &noiseLevel_ });
    }
}

// ============================================================================
// EffectsPanel
// ============================================================================

EffectsPanel::EffectsPanel(OpenHatterProcessor& processor)
    : distType_(processor.parameters, openhatter::params::kDistType, "Exciter Mode", theme::kAccentFlame),
      distDrive_(processor.parameters, openhatter::params::kDistDrive, "Drive", theme::kAccentFlame),
      distTone_(processor.parameters, openhatter::params::kDistTone, "Sheen Tilt", theme::kAccentFlame),
      distMix_(processor.parameters, openhatter::params::kDistMix, "Exciter Mix", theme::kAccentFlame),
      compThresh_(processor.parameters, openhatter::params::kCompThresh, "Threshold", theme::kAccentAmber),
      compRatio_(processor.parameters, openhatter::params::kCompRatio, "Ratio", theme::kAccentAmber),
      compAttack_(processor.parameters, openhatter::params::kCompAttack, "Comp Attack", theme::kAccentAmber),
      compRelease_(processor.parameters, openhatter::params::kCompRelease, "Comp Release", theme::kAccentAmber),
      compMakeup_(processor.parameters, openhatter::params::kCompMakeup, "Makeup", theme::kAccentAmber),
      compMix_(processor.parameters, openhatter::params::kCompMix, "Comp Mix", theme::kAccentAmber),
      limiterCeiling_(processor.parameters, openhatter::params::kLimiterCeiling, "Ceiling", theme::kAccentNeonGreen),
      limiterRelease_(processor.parameters, openhatter::params::kLimiterRelease, "Limit Release", theme::kAccentNeonGreen),
      limiterEnabled_(processor.parameters, openhatter::params::kLimiterEnabled, "LIMITER", theme::kAccentNeonGreen),
      reverbSize_(processor.parameters, openhatter::params::kReverbSize, "Room Size", theme::kAccentViolet),
      reverbDecay_(processor.parameters, openhatter::params::kReverbDecay, "Rev Decay", theme::kAccentViolet),
      reverbDamp_(processor.parameters, openhatter::params::kReverbDamp, "Damping", theme::kAccentViolet),
      reverbGate_(processor.parameters, openhatter::params::kReverbGate, "Choke Gate", theme::kAccentViolet),
      reverbMix_(processor.parameters, openhatter::params::kReverbMix, "Rev Mix", theme::kAccentViolet),
      reverseMode_(processor.parameters, openhatter::params::kReverseMode, "Rev Sync", theme::kAccentCyan),
      reverseMix_(processor.parameters, openhatter::params::kReverseMix, "Rev Swell", theme::kAccentCyan),
      midiMode_(processor.parameters, openhatter::params::kMidiMode, "MIDI Mode", theme::kTextDim),
      lockInBtn_(processor.parameters, openhatter::params::kLockIn, "LOCK WAV", theme::kAccentAmber),
      masterWidth_(processor.parameters, openhatter::params::kStereoWidth, "Master Width", theme::kAccentCyan),
      outputGain_(processor.parameters, openhatter::params::kOutputGain, "Output", theme::kAccentAmber),
      exciterLock_("Exciter", theme::kAccentFlame,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::exciter); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::exciter, on); }),
      dynamicsLock_("Dynamics", theme::kAccentAmber,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::dynamics); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::dynamics, on); }),
      spaceLock_("Space & Output", theme::kAccentCyan,
          [&processor] { return processor.isSectionLocked(OpenHatterProcessor::PresetSection::spaceAndOutput); },
          [&processor](bool on) { processor.setSectionLocked(OpenHatterProcessor::PresetSection::spaceAndOutput, on); }) {
    for (juce::Component* component : std::array<juce::Component*, 26>{
             &distType_, &distDrive_, &distTone_, &distMix_, &compThresh_, &compRatio_, &compAttack_, &compRelease_,
             &compMakeup_, &compMix_, &limiterCeiling_, &limiterRelease_, &limiterEnabled_, &reverbSize_, &reverbDecay_,
             &reverbDamp_, &reverbGate_, &reverbMix_, &reverseMode_, &reverseMix_, &midiMode_, &lockInBtn_,
             &masterWidth_, &outputGain_, &exciterLock_, &dynamicsLock_}) addAndMakeVisible(component);
    addAndMakeVisible(spaceLock_);
    setAdvancedMode(false);
}

void EffectsPanel::setAdvancedMode(bool advanced) {
    advancedMode_ = advanced;
    distTone_.setVisible(advanced);
    compThresh_.setVisible(advanced);
    compRatio_.setVisible(advanced);
    compAttack_.setVisible(advanced);
    compRelease_.setVisible(advanced);
    compMakeup_.setVisible(advanced);
    limiterRelease_.setVisible(advanced);
    reverbDecay_.setVisible(advanced);
    reverbDamp_.setVisible(advanced);
    reverseMode_.setVisible(advanced);
    reverseMix_.setVisible(advanced);
    midiMode_.setVisible(advanced);
    resized();
    repaint();
}

void EffectsPanel::paint(juce::Graphics& g) {
    drawPanel(g, getLocalBounds(), advancedMode_ ? "FX · EXCITER, DYNAMICS & SPACE · FULL CONTROLS"
                                                   : "FX · MASTERING & MOTION",
              theme::kAccentFlame);
    for (size_t i = 0; i < sectionBounds_.size(); ++i) {
        const auto bounds = sectionBounds_[i].toFloat().reduced(1.0f);
        if (bounds.isEmpty()) continue;
        const auto accent = i == 0 ? theme::kAccentFlame : (i == 1 ? theme::kAccentAmber : theme::kAccentCyan);
        g.setColour(theme::kPanel.brighter(0.035f));
        g.fillRoundedRectangle(bounds, 8.0f);
        g.setColour(accent.withAlpha(0.72f));
        g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
        const auto title = i == 0 ? "SHIMMER EXCITER" : (i == 1 ? "COMPRESSOR & LIMITER" : "SPACE, REVERSE & OUTPUT");
        g.setColour(accent);
        g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
        g.drawText(title, sectionBounds_[i].reduced(12, 0).withHeight(20).translated(0, 5), juce::Justification::centredLeft);
    }
}

void EffectsPanel::resized() {
    auto area = getLocalBounds().reduced(14, 32);
    const int firstHeight = std::max(170, area.getHeight() / 4);
    const int middleHeight = std::max(250, area.getHeight() / 3);
    sectionBounds_[0] = area.removeFromTop(firstHeight).reduced(0, 3);
    area.removeFromTop(8);
    sectionBounds_[1] = area.removeFromTop(middleHeight).reduced(0, 3);
    area.removeFromTop(8);
    sectionBounds_[2] = area.reduced(0, 3);
    exciterLock_.setBounds(sectionBounds_[0].removeFromRight(88).withY(sectionBounds_[0].getY() + 5).withHeight(22));
    dynamicsLock_.setBounds(sectionBounds_[1].removeFromRight(88).withY(sectionBounds_[1].getY() + 5).withHeight(22));
    spaceLock_.setBounds(sectionBounds_[2].removeFromRight(88).withY(sectionBounds_[2].getY() + 5).withHeight(22));
    if (advancedMode_)
        layoutRow(sectionBounds_[0].reduced(8, 34), { &distType_, &distDrive_, &distTone_, &distMix_ });
    else
        layoutRow(sectionBounds_[0].reduced(8, 34), { &distType_, &distDrive_, &distMix_ });
    auto dynamics = sectionBounds_[1].reduced(8, 32);
    if (advancedMode_) {
        layoutRow(dynamics.removeFromTop(116), { &compThresh_, &compRatio_, &compAttack_, &compRelease_, &compMakeup_, &compMix_ });
        layoutRow(dynamics, { &limiterCeiling_, &limiterRelease_, &limiterEnabled_ });
    } else {
        layoutRow(dynamics.removeFromTop(116), { &compMix_, &limiterCeiling_ });
        layoutRow(dynamics, { &limiterEnabled_ });
    }
    auto space = sectionBounds_[2].reduced(8, 30);
    if (advancedMode_) {
        layoutRow(space.removeFromTop(116), { &reverbSize_, &reverbDecay_, &reverbDamp_, &reverbGate_, &reverbMix_, &reverseMode_, &reverseMix_ });
        layoutRow(space, { &midiMode_, &lockInBtn_, &masterWidth_, &outputGain_ });
    } else {
        layoutRow(space.removeFromTop(116), { &reverbSize_, &reverbGate_, &reverbMix_ });
        layoutRow(space, { &lockInBtn_, &masterWidth_, &outputGain_ });
    }
}

} // namespace openhatter::ui
