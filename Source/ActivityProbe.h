#pragma once

#include <memory>

// =============================================================================
//  ActivityProbe — détecte si l'utilisateur a cliqué / tapé une touche depuis
//  la dernière sonde, ET si le DAW est l'application au premier plan.
//
//  Le plugin tourne dans le processus du DAW. Certains DAW (ex. Bitwig) font
//  tourner les plugins dans un processus séparé : le processus PARENT est
//  donc lui aussi considéré comme "le DAW".
//
//  Une implémentation par système :
//   - ActivityProbe_win.cpp   (Windows)
//   - ActivityProbe_mac.mm    (macOS)
//   - ActivityProbe_other.cpp (autres : aucune détection)
// =============================================================================
class ActivityProbe
{
public:
    ActivityProbe();
    ~ActivityProbe();

    /** Renvoie true si le DAW est au premier plan ET qu'un nouvel input
        (clic ou touche) a eu lieu depuis l'appel précédent. */
    bool poll();

    /** true si le DAW (ou son processus parent) est l'application active. */
    bool isHostForeground() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;

    ActivityProbe (const ActivityProbe&) = delete;
    ActivityProbe& operator= (const ActivityProbe&) = delete;
};
