#pragma once

namespace CoreEngine
{
    class SingleAppInstance
    {
    public:
        explicit SingleAppInstance(const wchar_t* name) noexcept;
        ~SingleAppInstance() noexcept;
        [[nodiscard]] bool IsFirstInstance() const noexcept;

        SingleAppInstance(const SingleAppInstance&) = delete;
        SingleAppInstance& operator=(const SingleAppInstance&) = delete;

    private:
        void* m_mutex{};
    };
}