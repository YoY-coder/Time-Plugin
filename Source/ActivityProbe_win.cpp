// Détection d'activité — Windows
#ifndef NOMINMAX
 #define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
 #define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tlhelp32.h>

#include "ActivityProbe.h"

#pragma comment (lib, "user32.lib")

namespace
{
    DWORD findParentProcessId (DWORD pid)
    {
        DWORD parent = 0;
        HANDLE snapshot = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

        if (snapshot == INVALID_HANDLE_VALUE)
            return 0;

        PROCESSENTRY32 entry {};
        entry.dwSize = sizeof (entry);

        if (Process32First (snapshot, &entry))
        {
            do
            {
                if (entry.th32ProcessID == pid)
                {
                    parent = entry.th32ParentProcessID;
                    break;
                }
            } while (Process32Next (snapshot, &entry));
        }

        CloseHandle (snapshot);
        return parent;
    }

    DWORD readLastInputTick()
    {
        LASTINPUTINFO info {};
        info.cbSize = sizeof (info);
        return GetLastInputInfo (&info) ? info.dwTime : 0;
    }
}

struct ActivityProbe::Impl
{
    DWORD ownPid     = GetCurrentProcessId();
    DWORD parentPid  = findParentProcessId (GetCurrentProcessId());
    DWORD lastInput  = readLastInputTick();

    bool hostIsForeground() const
    {
        HWND window = GetForegroundWindow();

        if (window == nullptr)
            return false;

        DWORD pid = 0;
        GetWindowThreadProcessId (window, &pid);

        return pid != 0 && (pid == ownPid || (parentPid != 0 && pid == parentPid));
    }
};

ActivityProbe::ActivityProbe() : impl (std::make_unique<Impl>()) {}
ActivityProbe::~ActivityProbe() = default;

bool ActivityProbe::isHostForeground() const   { return impl->hostIsForeground(); }

bool ActivityProbe::poll()
{
    // GetLastInputInfo donne l'instant (en "ticks" système) du dernier input
    // clavier/souris de la session. S'il a changé, il y a eu un nouvel input.
    const DWORD input  = readLastInputTick();
    const bool  hasNew = (input != impl->lastInput);
    impl->lastInput = input;

    return hasNew && impl->hostIsForeground();
}
