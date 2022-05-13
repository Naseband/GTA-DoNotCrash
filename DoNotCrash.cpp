// --------------------------------------------------------- 
/*

    DoNotCrash by Naseband

    Simple plugin for GTA 3, VC and SA that puts
    the player into any vehicle they collide with
    while driving.
    If the colliding vehicle has a driver, the driver
    of that vehicle will also swap to your old vehicle.

*/
// --------------------------------------------------------- 

#include "plugin.h"
#include <CEntity.h>
#include <CPlayerPed.h>
#include <CVehicle.h>
#include <CCamera.h>
#include <CPools.h>
#include <extensions/ScriptCommands.h>

#if defined GTAVC
#include <eEntityStatus.h>
#endif

#if defined GTASA
#include "SAMP.h"
#endif

#if defined GTAVC && !defined eEntityType
enum PLUGIN_API eEntityType // Missing in VC?
{
    ENTITY_TYPE_NOTHING = 0,
    ENTITY_TYPE_BUILDING = 1,
    ENTITY_TYPE_VEHICLE = 2,
    ENTITY_TYPE_PED = 3,
    ENTITY_TYPE_OBJECT = 4
};
#endif

using namespace plugin;

class DoNotCrash {
public:
    DoNotCrash()
    {
#if defined GTASA
        if (IsSAMP())
            return;
#endif

        Events::processScriptsEvent += []
        {
            if (!Command<Commands::IS_PLAYER_PLAYING>(0))
                return;

            CPlayerPed
                *pPlayerPed;

            CVehicle
                *pPlayerVehicle;

            CPed
                *pTargetPed;

            CVehicle
                *pTargetVehicle;

            CVector
                vecPlayerVelocity,
                vecTargetVelocity;

            static DWORD
                dwLastSwap = GetTickCount();

            DWORD
                dwNow = GetTickCount();

            static CVehicle
                *pLastVehicle = nullptr;

            int
                iPoolSize;

            if (dwNow - dwLastSwap < 100)
                return;

            // Find Player and Vehicle

#if !defined GTASA

            pPlayerPed = FindPlayerPed();

#else

            pPlayerPed = FindPlayerPed(0);

#endif

            if (!pPlayerPed)
                return;

#if !defined GTASA

            pPlayerVehicle = FindPlayerVehicle();

#else

            pPlayerVehicle = FindPlayerVehicle(0, false);

#endif

            if (!pPlayerVehicle || pPlayerVehicle->m_pDriver != pPlayerPed)
                return;

            // Make all vehicles be fully processed. Modern hardware can handle it!

            iPoolSize = CPools::ms_pVehiclePool->m_nSize;

            for (int i = 0; i < iPoolSize; ++i)
            {
                if (CPools::ms_pVehiclePool->IsFreeSlotAtIndex(i))
                    continue;

                pTargetVehicle = CPools::ms_pVehiclePool->GetAt(i);

                if (!pTargetVehicle->m_pDriver)
                    continue;

                if ((pTargetVehicle->GetPosition() - pPlayerVehicle->GetPosition()).Magnitude() > 40.0f)
                    continue;

#if !defined GTASA

                if (pTargetVehicle->m_nState == STATUS_SIMPLE)
                    pTargetVehicle->m_nState = STATUS_PHYSICS;

#endif
#if defined GTAVC

                if (pTargetVehicle->m_nState == STATUS_SIMPLE)
                    pTargetVehicle->m_nState = STATUS_PHYSICS;

#endif
#if defined GTASA

                if (pTargetVehicle->m_nStatus == STATUS_SIMPLE)
                    pTargetVehicle->m_nStatus = STATUS_PHYSICS;

#endif
            }

            // Find last collided vehicle and do the thing
            
#if defined GTA3

            if (pPlayerVehicle->m_pDamageEntity != nullptr && pPlayerVehicle->m_pDamageEntity->m_nType == eEntityType::ENTITY_TYPE_VEHICLE)
            {
                pTargetVehicle = (CVehicle*)pPlayerVehicle->m_pDamageEntity;
                pTargetPed = pTargetVehicle->m_pDriver;

#endif
#if defined GTAVC

            if (pPlayerVehicle->m_pPhysColliding != nullptr && pPlayerVehicle->m_pPhysColliding->m_nType == eEntityType::ENTITY_TYPE_VEHICLE)
            {
                pTargetVehicle = (CVehicle*)pPlayerVehicle->m_pPhysColliding;
                pTargetPed = pTargetVehicle->m_pDriver;

#endif
#if defined GTASA

            if (pPlayerVehicle->m_pDamageEntity != nullptr && pPlayerVehicle->m_pDamageEntity->m_nType == eEntityType::ENTITY_TYPE_VEHICLE)
            {
                pTargetVehicle = (CVehicle*)pPlayerVehicle->m_pDamageEntity;
                pTargetPed = pTargetVehicle->m_pDriver;

#endif
                if (pLastVehicle == pTargetVehicle && dwNow - dwLastSwap < 1500) // Dont immediately jump back to the previous car
                    return;

                pLastVehicle = pPlayerVehicle;
                dwLastSwap = dwNow;

                // Save velocity. When the vehicles are switched they usually lose all of it (requires status to be physics)

                vecPlayerVelocity = pPlayerVehicle->m_vecMoveSpeed;
                vecTargetVelocity = pPlayerVehicle->m_vecMoveSpeed;

                // Remove player from vehicle

                Command<Commands::WARP_CHAR_FROM_CAR_TO_COORD>(pPlayerPed, pPlayerPed->GetPosition().x, pPlayerPed->GetPosition().y, pPlayerPed->GetPosition().z + 15.0f);

                if (pTargetPed) // If the target vehicle has a driver, remove the ped from vehicle and put them in the player vehicle
                {
                    Command<Commands::WARP_CHAR_FROM_CAR_TO_COORD>(pTargetPed, pTargetPed->GetPosition().x, pTargetPed->GetPosition().y, pTargetPed->GetPosition().z + 15.0f);
                    Command<Commands::WARP_CHAR_INTO_CAR>(pTargetPed, pPlayerVehicle);
                }

                // Put player in target vehicle

                Command<Commands::WARP_CHAR_INTO_CAR>(pPlayerPed, pTargetVehicle);

                // Restore camera

                TheCamera.RestoreWithJumpCut();

                // Restore velocity

                pPlayerVehicle->m_vecMoveSpeed = vecPlayerVelocity;
                pPlayerVehicle->m_vecMoveSpeed = vecTargetVelocity;
            }
        }; // processScriptsEvent
    }
} doNotCrash;
