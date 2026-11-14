// Tests de la logique du chrono (sans DAW). Compilation :
//   cmake -B build-tests -DTIME_BUILD_TESTS=ON && cmake --build build-tests --target TimeTests
#include "LiveClock.h"
#include <cmath>
#include <cstdio>

static int failures = 0;

static void check (bool ok, const char* what)
{
    std::printf ("%s  %s\n", ok ? "[OK]  " : "[ECHEC]", what);
    if (! ok) ++failures;
}

static bool near (double a, double b, double tol = 0.3) { return std::abs (a - b) <= tol; }

// Simule le passage du temps par pas de 250 ms
struct Sim
{
    LiveClock clock { 0.0 };
    double t = 0.0;
    PlaybackInfo pb;

    void run (double seconds, bool activityEachTick = false, bool playing = false)
    {
        const double end = t + seconds;
        while (t + 1e-9 < end)
        {
            t += 0.25;
            if (playing)
            {
                if (! pb.playingNow) pb.playStart = t - 0.25;
                pb.playingNow = true;
                pb.lastPlayingBlock = t;
            }
            else
            {
                pb.playingNow = false;
            }
            clock.tick (t, activityEachTick, pb);
        }
    }
    void click() { clock.tick (t, true, pb); }
    double total() const { return clock.getCommittedSeconds() + clock.getPendingSeconds (t); }
};

int main()
{
    {
        Sim s; s.run (60);
        check (near (s.total(), 0), "Rien ne compte sans activité au chargement");
    }
    {
        Sim s; s.click(); s.run (60, true);
        check (near (s.clock.getCommittedSeconds(), 60), "Activité continue : 60 s comptées");
    }
    {
        Sim s; s.click(); s.run (120); 
        check (near (s.clock.getPendingSeconds (s.t), 120), "2 min sans action : 2 min en attente");
        check (s.clock.getState (s.t) == LiveClock::State::Waiting, "Etat 'en attente'");
        s.click();
        check (near (s.clock.getCommittedSeconds(), 120), "Action avant 5 min : l'attente est validée");
    }
    {
        Sim s; s.click(); s.run (30, true); s.run (301);
        check (near (s.total(), 30), "Plus de 5 min sans action : les 5 min sont retirées");
        check (s.clock.getState (s.t) == LiveClock::State::Stopped, "Etat arrêté");
        s.run (100); s.click(); s.run (10, true);
        check (near (s.total(), 40), "Reprise : seul le temps après la reprise compte");
    }
    {
        Sim s; s.run (20, false, true); s.run (2000, false, true);
        check (near (s.clock.getCommittedSeconds(), 2020), "Lecture : compte toujours, sans limite");
        s.run (100);
        check (near (s.total(), 2120), "Après lecture : temps en attente");
        s.run (250);
        check (near (s.total(), 2020), "Après lecture + 5 min : retour au temps de lecture");
    }
    {
        Sim s; s.click(); s.run (60, true);
        s.clock.setPaused (true, s.t);
        s.run (100, true); s.run (100, false, true);
        check (near (s.total(), 60), "Pause : ni l'activité ni la lecture ne comptent");
        s.clock.setPaused (false, s.t);
        s.run (10, true);
        check (near (s.total(), 70), "Fin de pause : le chrono repart");
    }
    {
        Sim s; s.click(); s.run (60);   // 60 s en attente
        s.run (30, false, true);        // lecture démarrée < 5 min après la dernière action
        check (near (s.clock.getCommittedSeconds(), 90), "Lecture : valide aussi l'attente précédente");
    }
    {
        // Export offline : le timer de l'interface est bloqué pendant 40 s
        Sim s; s.click();
        s.pb.playingNow = true; s.pb.playStart = 0.1; s.pb.lastPlayingBlock = 40.0;
        s.t = 40.2; s.pb.playingNow = false; s.clock.tick (s.t, false, s.pb);
        check (near (s.clock.getCommittedSeconds(), 40.0), "Export / tick retardé : temps réel écoulé compté");
    }

    std::printf ("\n%s (%d échec(s))\n", failures == 0 ? "TOUS LES TESTS PASSENT" : "DES TESTS ECHOUENT", failures);
    return failures == 0 ? 0 : 1;
}
