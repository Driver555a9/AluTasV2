#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

//std
#include <string>

namespace CoreEngine
{
    class Window
    {
    public:
        struct WindowCreationConfig 
        {
            std::string m_title;
            std::pair<int, int> m_size {500, 500};
            bool m_is_windowed_fullscren = false;
            bool m_is_transparent_clickthrough = false;
        };

        ~Window() noexcept;
        explicit Window(WindowCreationConfig config) noexcept;
        explicit Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

    ///////////////////////////////
    // Methods
    ///////////////////////////////
        [[nodiscard]] GLFWwindow* GetRawPtr() const noexcept;

    #ifdef _WIN32
        void SetClickthrough(bool enable) noexcept;
    #endif
        [[nodiscard]] bool GetIsClickthrough() const noexcept;

    ///////////////////////////////
    // Copying forbidden
    ///////////////////////////////
        Window(const Window&)            = delete;
        Window& operator=(const Window&) = delete;

    private:
        GLFWwindow* m_window_ptr = nullptr;
        bool m_is_clickthrough_window = false;
    };
}