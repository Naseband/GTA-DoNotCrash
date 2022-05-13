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
#include <CTheScripts.h>
#include <extensions/ScriptCommands.h>

#include "StructParser.h"

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

struct SConfig
{
    bool bLoaded = false;

    bool bActive = true;

    bool bActiveOnMission = false;
    bool bActiveOnSubmission = false;
    
    unsigned int iSwapDelay = 100;
    unsigned int iSwapBackDelay = 1500;

    bool bPedToPed = true;
    bool bPedToVehicle = true;
    bool bPedToObject = true;

    bool bVehicleToPed = true;
    bool bVehicleToVehicle = true;
    bool bVehicleToObject = true;

    bool bObjectToPed = true;
    bool bObjectToVehicle = true;
    bool bObjectToObject = true;
};

SConfig g_Config;

void LoadConfig()
{
    SConfig
        Base;

    StructParser<SConfig>
        *pStructParser = new StructParser<SConfig>(&Base);

#define VAR(x) &x, sizeof(x) // Only use it for non-arrays.

    // General

    pStructParser->Link(LinkType::BOOL, VAR(Base.bActive), 1, "General", "Active");

    pStructParser->Link(LinkType::BOOL, VAR(Base.bActiveOnMission), 1, "General", "ActiveOnMission");
    pStructParser->Link(LinkType::BOOL, VAR(Base.bActiveOnSubmission), 1, "General", "ActiveOnSubmission");

    pStructParser->Link(LinkType::UNSIGNED, VAR(Base.iSwapDelay), 1, "General", "SwapDelay");
    pStructParser->Link(LinkType::UNSIGNED, VAR(Base.iSwapBackDelay), 1, "General", "SwapBackDelay");

    // SwapTypes

    pStructParser->Link(LinkType::BOOL, VAR(Base.bPedToPed), 1, "SwapTypes", "PedToPed");
    pStructParser->Link(LinkType::BOOL, VAR(Base.bPedToVehicle), 1, "SwapTypes", "PedToVehicle");
    pStructParser->Link(LinkType::BOOL, VAR(Base.bPedToObject), 1, "SwapTypes", "PedToObject");

    pStructParser->Link(LinkType::BOOL, VAR(Base.bVehicleToPed), 1, "SwapTypes", "VehicleToPed");
    pStructParser->Link(LinkType::BOOL, VAR(Base.bVehicleToVehicle), 1, "SwapTypes", "VehicleToVehicle");
    pStructParser->Link(LinkType::BOOL, VAR(Base.bVehicleToObject), 1, "SwapTypes", "VehicleToObject");

    pStructParser->Link(LinkType::BOOL, VAR(Base.bObjectToPed), 1, "SwapTypes", "ObjectToPed");
    pStructParser->Link(LinkType::BOOL, VAR(Base.bObjectToVehicle), 1, "SwapTypes", "ObjectToVehicle");
    pStructParser->Link(LinkType::BOOL, VAR(Base.bObjectToObject), 1, "SwapTypes", "ObjectToObject");

#undef VAR

    if (pStructParser->ParseFile("scripts/DoNotCrash.ini", &g_Config) != -1 ||
        pStructParser->ParseFile("plugins/DoNotCrash.ini", &g_Config) != -1 ||
        pStructParser->ParseFile("DoNotCrash.ini", &g_Config) != -1
        )
    {
        g_Config.bLoaded = true;
    }

    delete pStructParser;
}

class DoNotCrash {
public:
    DoNotCrash()
    {
#if defined GTASA
        if (IsSAMP())
            return;
#endif

        // Add to scripts event

        Events::processScriptsEvent += []
        {
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

            static bool
                bInit = false;

            // Load config

            if (!bInit)
            {
                LoadConfig();
                bInit = true;
            }

            // Check if we even need to do anything

            if (!g_Config.bActive || 
                dwNow - dwLastSwap < g_Config.iSwapDelay ||
                !Command<Commands::IS_PLAYER_PLAYING>(0) ||
                !g_Config.bActiveOnMission && CTheScripts::IsPlayerOnAMission()
                )
                return;

#if defined GTASA

            if (!g_Config.bActiveOnSubmission && CTheScripts::bMiniGameInProgress)
                return;

#endif

            // Find Player and Vehicle

#if defined GTASA

            pPlayerPed = FindPlayerPed(0);
            pPlayerVehicle = FindPlayerVehicle(0, false);

#else

            pPlayerPed = FindPlayerPed();
            pPlayerVehicle = FindPlayerVehicle();

#endif

            if (!pPlayerPed || !pPlayerVehicle || pPlayerVehicle->m_pDriver != pPlayerPed)
                return;

            // Make all vehicles be fully processed. Modern hardware can handle it!

            for (int i = 0; i < CPools::ms_pVehiclePool->m_nSize; ++i)
            {
                if (CPools::ms_pVehiclePool->IsFreeSlotAtIndex(i))
                    continue;

                pTargetVehicle = CPools::ms_pVehiclePool->GetAt(i);

                if (!pTargetVehicle->m_pDriver)
                    continue;

                if ((pTargetVehicle->GetPosition() - pPlayerVehicle->GetPosition()).Magnitude() > 40.0f)
                    continue;

#if defined GTASA

                if (pTargetVehicle->m_nStatus == STATUS_SIMPLE)
                    pTargetVehicle->m_nStatus = STATUS_PHYSICS;

#else

                if (pTargetVehicle->m_nState == STATUS_SIMPLE)
                    pTargetVehicle->m_nState = STATUS_PHYSICS;

#endif
            }

            // Find last collided vehicle and do the thing
            
#if defined GTAVC

            if (pPlayerVehicle->m_pPhysColliding != nullptr && pPlayerVehicle->m_pPhysColliding->m_nType == eEntityType::ENTITY_TYPE_VEHICLE)
            {
                pTargetVehicle = static_cast<CVehicle*>(pPlayerVehicle->m_pPhysColliding);
                pTargetPed = pTargetVehicle->m_pDriver;
            

#else

            if (pPlayerVehicle->m_pDamageEntity != nullptr && pPlayerVehicle->m_pDamageEntity->m_nType == eEntityType::ENTITY_TYPE_VEHICLE)
            {
                pTargetVehicle = static_cast<CVehicle*>(pPlayerVehicle->m_pDamageEntity);
                pTargetPed = pTargetVehicle->m_pDriver;

#endif

                if (pLastVehicle == pTargetVehicle && dwNow - dwLastSwap < g_Config.iSwapBackDelay) // Dont immediately jump back to the previous car
                    return;

                pLastVehicle = pPlayerVehicle;
                dwLastSwap = dwNow;

                // Save velocity. When the vehicles are switched they usually lose all of it (requires status to be physics)

                vecPlayerVelocity = pPlayerVehicle->m_vecMoveSpeed;
                vecTargetVelocity = pTargetVehicle->m_vecMoveSpeed;

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
                pTargetVehicle->m_vecMoveSpeed = vecTargetVelocity;
            }
        }; // end processScriptsEvent
    }
} doNotCrash;
