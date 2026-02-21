#include "tas/servicethreads/MemoryAddressUpdateService.h"

#include "tas/memory/MemoryAddressFinder.h"
#include "tas/globalstate/MemoryAddressState.h"

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
            while (GetThreadIsRunning())
            {
                try 
                {
                    RacerStateAddresses::ManuallySetAddresses(MemoryAddressFinder::FindRacerStateBaseAddress());
                } 
                catch (...) 
                { 
                    RacerStateAddresses::ManuallySetAddresses(0);
                }
                try 
                {
                    CameraStateAddresses::ManuallySetAddresses(MemoryAddressFinder::FindCameraStateAddresses());
                } 
                catch (...)
                {
                    CameraStateAddresses::ManuallySetAddresses(0);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
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