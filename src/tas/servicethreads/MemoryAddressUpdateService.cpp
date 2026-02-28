#include "tas/servicethreads/MemoryAddressUpdateService.h"

#include "tas/memory/MemoryAddressFinder.h"
#include "tas/globalstate/MemoryAddressState.h"
#include "tas/memory/MemoryRW.h"

#include "core/utility/Assert.h"

#include <thread>
#include <atomic>

namespace AsphaltTas::MemoryAddressUpdateService
{
namespace 
{
    std::atomic<bool> g_thread_is_running = false;
}
    void LaunchThread() noexcept
    {
        if (GetThreadIsRunning()) return;
        
        g_thread_is_running.store(true);
        std::thread([]()
        {
            ENGINE_INFO_LOG("Memory Address Update Thread launched.");
            while (GetThreadIsRunning())
            {
                //////////////////////////////////////////////
                // Racer State
                //////////////////////////////////////////////
                try 
                {
                    RacerStateAddresses::ManuallySetAddresses(MemoryAddressFinder::FindRacerStateBaseAddress(CoreEngine::Units::MilliSecond(100)));
                } 
                catch (...) 
                { 
                    RacerStateAddresses::ManuallySetAddresses(INVALID_ADDRESS); 
                }
                if (!GetThreadIsRunning()) break;
                //////////////////////////////////////////////
                // Camera State
                //////////////////////////////////////////////
                try 
                { 
                    CameraStateAddresses::ManuallySetAddresses(MemoryAddressFinder::FindCameraStateAddresses(CoreEngine::Units::MilliSecond(100)));
                } 
                catch (...) 
                { 
                    CameraStateAddresses::ManuallySetAddresses(INVALID_ADDRESS); 
                }
                if (!GetThreadIsRunning()) break;
                //////////////////////////////////////////////
                // Lap Time
                //////////////////////////////////////////////
                try 
                { 
                    RaceProgressStateAddresses::SetLapTimeAddress(MemoryAddressFinder::FindLapTimeAddress(CoreEngine::Units::MilliSecond(100)));
                } 
                catch (...) 
                { 
                    try 
                    {
                        CoreEngine::Units::MicroSecond time = MemoryRW::ReadCurrentRaceTime();
                        if (CoreEngine::Units::Convert<CoreEngine::Units::Second>(time) > CoreEngine::Units::Second(2000))
                            RaceProgressStateAddresses::SetLapTimeAddress(INVALID_ADDRESS); 
                    } 
                    catch (...)
                    {
                        RaceProgressStateAddresses::SetLapTimeAddress(INVALID_ADDRESS); 
                    }
                }
                if (!GetThreadIsRunning()) break;
                //////////////////////////////////////////////
                // Race Progress %
                //////////////////////////////////////////////
                try 
                { 
                    RaceProgressStateAddresses::SetRaceProgressAddress(MemoryAddressFinder::FindRaceProgressAddress(CoreEngine::Units::MilliSecond(100)));
                } 
                catch (const std::exception& e) 
                { 
                    try 
                    {
                        const float progress = MemoryRW::ReadCurrentRaceProgress();
                        if (progress < 0.0f || progress > 100.1f)
                            RaceProgressStateAddresses::SetRaceProgressAddress(INVALID_ADDRESS); 
                    } catch (...)
                    {
                        RaceProgressStateAddresses::SetRaceProgressAddress(INVALID_ADDRESS); 
                    }
                } 
                if (!GetThreadIsRunning()) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            ENGINE_INFO_LOG("Memory Address Update Thread stopped.");
        }).detach();
    }

    void StopThread() noexcept
    {
        g_thread_is_running.store(false, std::memory_order::release);
    }

    bool GetThreadIsRunning() noexcept
    {
        return g_thread_is_running.load(std::memory_order::relaxed);
    }
}