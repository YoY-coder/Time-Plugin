#pragma once

// =============================================================================
//  LiveClock — le cœur du chronométrage de la session en cours.
//
//  Cette classe ne dépend pas de JUCE : elle reçoit l'heure (horloge monotone,
//  en secondes) de l'extérieur, ce qui la rend précise (pas de comptage de
//  "ticks") et facile à tester.
//
//  Règles :
//   - Activité (clic / touche avec le DAW au premier plan) : tout le temps écoulé
//     depuis la dernière activité ("en attente") est validé, à condition que
//     l'écart soit <= 5 minutes.
//   - Lecture du transport : le temps compte toujours, sans limite.
//   - Plus de 5 minutes sans activité (hors lecture) : le temps en attente est
//     supprimé et le chrono s'arrête jusqu'à la prochaine activité.
//   - Pause forcée : rien ne compte.
// =============================================================================

struct PlaybackInfo
{
    bool   playingNow       = false; // le transport joue (et processBlock appelé il y a < 1 s)
    double playStart        = -1.0;  // instant (monotone) où la lecture en cours a commencé
    double lastPlayingBlock = -1.0;  // instant (monotone) du dernier bloc audio joué
};

class LiveClock
{
public:
    static constexpr double inactivityTimeout = 300.0; // 5 minutes
    static constexpr double activeDisplayTime = 10.0;  // "Actif" pendant 10 s après une action

    enum class State { Active, Playing, Waiting, Stopped, Paused };

    explicit LiveClock (double now = 0.0) { reset (now); }

    /** Remet la session à zéro (aucune pause, rien en attente). */
    void reset (double now);

    /** À appeler régulièrement (~ toutes les 250 ms). */
    void tick (double now, bool userActivity, const PlaybackInfo& playback);

    void setPaused (bool shouldPause, double now);
    bool isPaused() const noexcept               { return paused; }

    /** Temps validé de la session en cours (secondes). */
    double getCommittedSeconds() const noexcept  { return committed; }

    /** Temps en attente de validation (secondes), 0 si aucun. */
    double getPendingSeconds (double now) const noexcept;

    /** Temps restant avant que le temps en attente soit retiré. */
    double getSecondsBeforeDiscard (double now) const noexcept;

    State getState (double now) const noexcept;

private:
    void commitUpTo (double t);

    double committed    = 0.0;
    double pendingFrom  = 0.0;   // début du temps non encore validé (= dernière activité)
    double lastTick     = 0.0;
    double sessionBegin = 0.0;
    bool   idle         = true;  // true = rien en attente, chrono arrêté
    bool   paused       = false;
    bool   playingNow   = false;
};
