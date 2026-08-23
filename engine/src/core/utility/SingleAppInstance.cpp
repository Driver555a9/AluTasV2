#include "core/utility/SingleAppInstance.h"

#ifdef _WIN32 
    #include <windows.h>
#endif

namespace CoreEngine 
{
    SingleAppInstance::SingleAppInstance(const wchar_t* name) noexcept
    {
    #ifdef _WIN32
        m_mutex = CreateMutexW(nullptr, TRUE, name);

        if (m_mutex && GetLastError() == ERROR_ALREADY_EXISTS)
        {
            CloseHandle(m_mutex);
            m_mutex = nullptr;
        }
    #endif
    }

    SingleAppInstance::~SingleAppInstance() noexcept
    {
    #ifdef _WIN32
        if (m_mutex)
        {
            CloseHandle(m_mutex);
        }
    #endif
    }

    [[nodiscard]] bool SingleAppInstance::IsFirstInstance() const noexcept
    {
    #ifdef _WIN32
        return m_mutex != nullptr;
    #else 
        return true;
    #endif
    }
}