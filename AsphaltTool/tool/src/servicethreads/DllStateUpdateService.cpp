#include "servicethreads/DllStateUpdateService.h"
#include "DllStateUpdateService.h"
#include "globalstate/AsphaltDllManager.h"

#include <atomic>
#include <thread>

namespace AsphaltTas
{
    namespace DllStateUpdateService
    {
    namespace
    {
        std::atomic<bool> g_thread_is_running;
    }

        void LaunchThread() noexcept
        {
            std::thread([]()
            {
                g_thread_is_running.store(true, std::memory_order::release);
                while (GetThreadIsRunning())
                {
                    AsphaltDllManager::UpdateCurrentCommunicationState();
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }).detach();
        }

        void StopThread() noexcept
        {
            g_thread_is_running.store(false, std::memory_order::release);
        }

        bool GetThreadIsRunning() noexcept
        {
            return g_thread_is_running.load(std::memory_order::acquire);
        }
    }
}