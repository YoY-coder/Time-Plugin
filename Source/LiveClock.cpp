#include "LiveClock.h"
#include <algorithm>

void LiveClock::reset (double now)
{
    committed    = 0.0;
    pendingFrom  = now;
    lastTick     = now;
    sessionBegin = now;
    idle         = true;
    paused       = false;
    playingNow   = false;
}

void LiveClock::commitUpTo (double t)
{
    if (t > pendingFrom)
    {
        committed  += t - pendingFrom;
        pendingFrom = t;
    }
}

void LiveClock::tick (double now, bool userActivity, const PlaybackInfo& playback)
{
    if (now < lastTick)  // ne devrait jamais arriver avec une horloge monotone
        now = lastTick;

    playingNow = playback.playingNow;

    if (paused)
    {
        lastTick = now;
        return;
    }

    // --- 1. Lecture : le temps de lecture compte toujours -------------------
    const bool playedSinceLastTick = playback.playingNow
                                  || (playback.lastPlayingBlock >= 0.0 && playback.lastPlayingBlock > lastTick);

    if (playedSinceLastTick)
    {
        const double playEnd = playback.playingNow ? now
                                                   : std::min (now, playback.lastPlayingBlock);

        double start = playback.playStart;

        // Si la lecture démarre moins de 5 min après la dernière activité,
        // le temps en attente avant la lecture est validé lui aussi.
        if (! idle && playback.playStart - pendingFrom <= inactivityTimeout)
            start = pendingFrom;

        start = std::max (start, sessionBegin);

        if (playEnd > start)
        {
            pendingFrom = start;
            commitUpTo (playEnd);
            idle = false;
        }
    }

    // --- 2. Activité utilisateur ---------------------------------------------
    if (userActivity)
    {
        if (! idle && now - pendingFrom <= inactivityTimeout)
            commitUpTo (now);           // l'attente est validée
        else
            pendingFrom = now;          // reprise après inactivité : on repart d'ici

        idle = false;
    }

    // --- 3. Inactivité de plus de 5 minutes : le temps en attente est retiré --
    if (! idle && ! playback.playingNow && now - pendingFrom > inactivityTimeout)
    {
        idle        = true;
        pendingFrom = now;
    }

    lastTick = now;
}

void LiveClock::setPaused (bool shouldPause, double now)
{
    if (shouldPause == paused)
        return;

    if (shouldPause)
    {
        // Appuyer sur Pause est une action : le temps en attente est validé.
        if (! idle && now - pendingFrom <= inactivityTimeout)
            commitUpTo (now);

        paused = true;
        idle   = true;
    }
    else
    {
        // Reprendre est une action : le chrono repart d'ici.
        paused      = false;
        idle        = false;
        pendingFrom = now;
    }

    lastTick = now;
}

double LiveClock::getPendingSeconds (double now) const noexcept
{
    if (paused || idle)
        return 0.0;

    const double p = now - pendingFrom;
    return (p > 0.0 && p <= inactivityTimeout) ? p : 0.0;
}

double LiveClock::getSecondsBeforeDiscard (double now) const noexcept
{
    if (paused || idle)
        return 0.0;

    return std::max (0.0, inactivityTimeout - (now - pendingFrom));
}

LiveClock::State LiveClock::getState (double now) const noexcept
{
    if (paused)                                   return State::Paused;
    if (playingNow)                               return State::Playing;
    if (idle || now - pendingFrom > inactivityTimeout) return State::Stopped;
    if (now - pendingFrom < activeDisplayTime)    return State::Active;
    return State::Waiting;
}
