#include "AsphaltDLL.h"

#include "AsphaltDLLUtility.h"
#include "DetourFunctions.h"

#include "MinHook.h"

#include <Xinput.h>
#include <chrono>
#include <atomic>
#include <thread>

namespace
{
    HANDLE g_thread_handle      = nullptr;
    HMODULE g_hmodule           = nullptr;
    std::atomic<bool> g_running = false;
}

extern "C" __declspec(dllexport) void RequestShutdown() noexcept
{
    AsphaltDLL::Shutdown();
}

namespace AsphaltDLL 
{
    DWORD WINAPI Loop(LPVOID) noexcept
    {
        Utility::InitConsole();

        if (MH_Initialize() == MH_OK)
        {
            DLL_INFO_LOG("Successfully initialized MinHook.");
            SetupHooks();
        }
        else
        {
            DLL_ERROR_PRINT("Failed to initialize MinHook.");
        }

        while (g_running.load(std::memory_order::acquire))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(8));
            
            {
                LOCK_CURRENT_STATE_MUTEX();
                DetourFunctions::StateManager::OnHandleGeneralInBuffer();
            }
        }

        RemoveHooks();

        MH_Uninitialize();
        Communication::SharedMemory::ShutdownSharedMemory();
        Utility::ShutdownConsole();

        FreeLibraryAndExitThread(g_hmodule, 0);
    }

    void SetupHooks() noexcept
    {
        DetourFunctions::CameraUpdate::PatchEnableGameFovWriteInstruction();

        DetourFunctions::FloatXorObfuscationSetter::SetupHook();
        DetourFunctions::FloatXorObfuscationSetter::EnableHook();

        DetourFunctions::FloatXorObfuscationGetter::SetupHook();
        DetourFunctions::FloatXorObfuscationGetter::EnableHook();

        DetourFunctions::OnNewFrameWithPhysics::SetupHook();
        DetourFunctions::OnNewFrameWithPhysics::EnableHook();

        DetourFunctions::NewBulletPhysicsTick::SetupHook();
        DetourFunctions::NewBulletPhysicsTick::EnableHook();

        DetourFunctions::BrakeValue::SetupHook();
        DetourFunctions::BrakeValue::EnableHook();

        DetourFunctions::SteeringValue::SetupHook();
        DetourFunctions::SteeringValue::EnableHook();

        DetourFunctions::AcceleratorValue::SetupHook();
        DetourFunctions::AcceleratorValue::EnableHook();

        DetourFunctions::EnableNitro::SetupHook();
        DetourFunctions::EnableNitro::EnableHook();

        DetourFunctions::DecreaseNitroBarFunc::SetupHook();
        DetourFunctions::DecreaseNitroBarFunc::EnableHook();

        DetourFunctions::IncreaseNitroBarFunc::SetupHook();
        DetourFunctions::IncreaseNitroBarFunc::EnableHook();

        DetourFunctions::RacerTransformUpdate::SetupHook();
        DetourFunctions::RacerTransformUpdate::EnableHook();

        DetourFunctions::GetLocalRacerStruct::SetupHook();
        DetourFunctions::GetLocalRacerStruct::EnableHook();

        DetourFunctions::CameraUpdate::SetupHook();
        DetourFunctions::CameraUpdate::EnableHook();

        DetourFunctions::BarrelRollStabilization::SetupHook();
        DetourFunctions::BarrelRollStabilization::EnableHook();

        DetourFunctions::BarrelYawStabilization::SetupHook();
        DetourFunctions::BarrelYawStabilization::EnableHook();

        DetourFunctions::OnWreck::SetupHook();
        DetourFunctions::OnWreck::EnableHook();

        DetourFunctions::OnRespawnButtonPressed::SetupHook();
        DetourFunctions::OnRespawnButtonPressed::EnableHook();

        DetourFunctions::GetPhysicsInterval::SetupHook();
        DetourFunctions::GetPhysicsInterval::EnableHook();

        DetourFunctions::OnBeginRaceFunction::SetupHook();
        DetourFunctions::OnBeginRaceFunction::EnableHook();

        DetourFunctions::OnClickPlayFunction::SetupHook();
        DetourFunctions::OnClickPlayFunction::EnableHook();

        DetourFunctions::OnEndRaceFunction::SetupHook();
        DetourFunctions::OnEndRaceFunction::EnableHook();

        DetourFunctions::OnUpdateRaceProgress::SetupHook();
        DetourFunctions::OnUpdateRaceProgress::EnableHook();

        DetourFunctions::OnUpdateCheckpoint::SetupHook();
        DetourFunctions::OnUpdateCheckpoint::EnableHook();

        DetourFunctions::RenderGUIToggle::SetupHook();
        DetourFunctions::RenderGUIToggle::EnableHook();

        DetourFunctions::XInput_GetState::SetupHook();
        DetourFunctions::XInput_GetState::EnableHook();

        DetourFunctions::MainLoopNewFrameDispatcher::SetupHook();
        DetourFunctions::MainLoopNewFrameDispatcher::EnableHook();

        DetourFunctions::AnimationProgressFunction::SetupHook();
        DetourFunctions::AnimationProgressFunction::EnableHook();

        DetourFunctions::UcrtBaseRand::SetupHook();
        DetourFunctions::UcrtBaseRand::EnableHook();

        DetourFunctions::BVHBroadphaseTraversal::SetupHook();
        DetourFunctions::BVHBroadphaseTraversal::EnableHook();

        /////////////////////////////////////////////////////////////////
        // Experimental
        /////////////////////////////////////////////////////////////////
        //DetourFunctions::Experimental::JtlAbsolutePath::SetupHook();
        //DetourFunctions::Experimental::JtlAbsolutePath::EnableHook();

        //DetourFunctions::Experimental::FunctionLookup::SetupHook();
        //DetourFunctions::Experimental::FunctionLookup::EnableHook();

        //DetourFunctions::Experimental::OnRaycastVehicleUpdate::SetupHook();
        //DetourFunctions::Experimental::OnRaycastVehicleUpdate::EnableHook();

        //DetourFunctions::Experimental::PhysicsWorldRaycast::SetupHook();
        //DetourFunctions::Experimental::PhysicsWorldRaycast::EnableHook();
    }

    void RemoveHooks() noexcept
    {
        DetourFunctions::XInput_GetState::DisableHook();
        DetourFunctions::OnNewFrameWithPhysics::DisableHook();
        DetourFunctions::NewBulletPhysicsTick::DisableHook();
        DetourFunctions::BrakeValue::DisableHook();
        DetourFunctions::SteeringValue::DisableHook();
        DetourFunctions::AcceleratorValue::DisableHook();
        DetourFunctions::EnableNitro::DisableHook();
        DetourFunctions::DecreaseNitroBarFunc::DisableHook();
        DetourFunctions::IncreaseNitroBarFunc::DisableHook();
        DetourFunctions::RacerTransformUpdate::DisableHook();
        DetourFunctions::CameraUpdate::DisableHook();
        DetourFunctions::BarrelRollStabilization::DisableHook();
        DetourFunctions::BarrelYawStabilization::DisableHook();
        DetourFunctions::OnWreck::DisableHook();
        DetourFunctions::OnRespawnButtonPressed::DisableHook();
        DetourFunctions::GetPhysicsInterval::DisableHook();
        DetourFunctions::OnBeginRaceFunction::DisableHook();
        DetourFunctions::OnClickPlayFunction::DisableHook();
        DetourFunctions::OnEndRaceFunction::DisableHook();
        DetourFunctions::FloatXorObfuscationGetter::DisableHook();
        DetourFunctions::FloatXorObfuscationSetter::DisableHook();
        DetourFunctions::MainLoopNewFrameDispatcher::DisableHook();
        DetourFunctions::AnimationProgressFunction::DisableHook();
        DetourFunctions::UcrtBaseRand::DisableHook();
        DetourFunctions::BVHBroadphaseTraversal::DisableHook();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        DetourFunctions::XInput_GetState::RemoveHook();
        DetourFunctions::OnNewFrameWithPhysics::RemoveHook();
        DetourFunctions::NewBulletPhysicsTick::RemoveHook();
        DetourFunctions::BrakeValue::RemoveHook();
        DetourFunctions::SteeringValue::RemoveHook();
        DetourFunctions::AcceleratorValue::RemoveHook();
        DetourFunctions::EnableNitro::RemoveHook();
        DetourFunctions::DecreaseNitroBarFunc::RemoveHook();
        DetourFunctions::IncreaseNitroBarFunc::RemoveHook();
        DetourFunctions::RacerTransformUpdate::RemoveHook();
        DetourFunctions::CameraUpdate::RemoveHook();
        DetourFunctions::BarrelRollStabilization::RemoveHook();
        DetourFunctions::BarrelYawStabilization::RemoveHook();
        DetourFunctions::OnWreck::RemoveHook();
        DetourFunctions::OnRespawnButtonPressed::RemoveHook();
        DetourFunctions::GetPhysicsInterval::RemoveHook();
        DetourFunctions::OnBeginRaceFunction::RemoveHook();
        DetourFunctions::OnClickPlayFunction::RemoveHook();
        DetourFunctions::OnEndRaceFunction::RemoveHook();
        DetourFunctions::FloatXorObfuscationGetter::RemoveHook();
        DetourFunctions::FloatXorObfuscationSetter::RemoveHook();
        DetourFunctions::MainLoopNewFrameDispatcher::RemoveHook();
        DetourFunctions::AnimationProgressFunction::RemoveHook();
        DetourFunctions::UcrtBaseRand::RemoveHook();
        DetourFunctions::BVHBroadphaseTraversal::RemoveHook();
    }

    void Initialize(HMODULE hmodule) noexcept
    {
        g_hmodule = hmodule;
        g_running.store(true, std::memory_order::release);
        g_thread_handle = CreateThread(nullptr, 0, &AsphaltDLL::Loop, nullptr, 0, nullptr);
    }

    void Shutdown() noexcept
    {
        DLL_INFO_LOG("Shutdown initiated...");
        g_running.store(false, std::memory_order::release);
    }
}