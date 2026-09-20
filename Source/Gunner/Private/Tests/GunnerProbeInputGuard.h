#pragma once
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

/** Keep hardware input out of opt-in synthetic probes; controller injection still runs normally. */
struct FGunnerProbeInputGuard
{
    void Begin(UWorld* World)
    {
        if (bActive || !World || !World->GetGameViewport()) return;
        Viewport = World->GetGameViewport();
        bWasIgnoring = Viewport->IgnoreInput();
        Viewport->SetIgnoreInput(true);
        bActive = true;
    }

    void Restore()
    {
        if (bActive && Viewport.IsValid()) Viewport->SetIgnoreInput(bWasIgnoring);
        bActive = false;
        Viewport.Reset();
    }

private:
    TWeakObjectPtr<UGameViewportClient> Viewport;
    bool bActive = false;
    bool bWasIgnoring = false;
};
