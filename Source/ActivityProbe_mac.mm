// Détection d'activité — macOS
#import <AppKit/AppKit.h>
#import <CoreGraphics/CoreGraphics.h>
#include <unistd.h>
#include <cmath>

#include "ActivityProbe.h"

namespace
{
    // Secondes écoulées depuis le dernier clic souris ou la dernière touche.
    // CGEventSourceSecondsSinceLastEventType ne nécessite AUCUNE autorisation
    // d'accessibilité.
    double secondsSinceLastClickOrKey()
    {
        const CGEventType types[] = { kCGEventLeftMouseDown,
                                      kCGEventRightMouseDown,
                                      kCGEventOtherMouseDown,
                                      kCGEventKeyDown,
                                      kCGEventFlagsChanged };  // touches de modification (cmd, alt…)
        double best = 1.0e9;

        for (auto type : types)
        {
            const double s = CGEventSourceSecondsSinceLastEventType (kCGEventSourceStateCombinedSessionState, type);

            if (std::isfinite (s) && s >= 0.0 && s < best)
                best = s;
        }

        return best;
    }

    double monotonicNow()
    {
        return [[NSProcessInfo processInfo] systemUptime];
    }
}

struct ActivityProbe::Impl
{
    pid_t ownPid    = getpid();
    pid_t parentPid = getppid();
    double lastInputTime = monotonicNow() - secondsSinceLastClickOrKey(); // instant du dernier input connu

    bool hostIsForeground() const
    {
        @autoreleasepool
        {
            NSRunningApplication* app = [[NSWorkspace sharedWorkspace] frontmostApplication];

            if (app == nil)
                return false;

            const pid_t pid = [app processIdentifier];
            return pid == ownPid || (parentPid > 1 && pid == parentPid);
        }
    }
};

ActivityProbe::ActivityProbe() : impl (std::make_unique<Impl>()) {}
ActivityProbe::~ActivityProbe() = default;

bool ActivityProbe::isHostForeground() const   { return impl->hostIsForeground(); }

bool ActivityProbe::poll()
{
    const double inputTime = monotonicNow() - secondsSinceLastClickOrKey();

    // Marge de 50 ms pour ignorer les petites imprécisions de mesure.
    const bool hasNew = inputTime > impl->lastInputTime + 0.05;

    if (hasNew)
        impl->lastInputTime = inputTime;

    return hasNew && impl->hostIsForeground();
}
