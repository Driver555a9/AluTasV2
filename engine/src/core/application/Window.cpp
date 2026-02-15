#include "core/application/Window.h"

#include "core/utility/Assert.h"

//std
#include <iostream>

#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3native.h>
    #include <dwmapi.h>
#else 
    #include
    #define GLFW_INCLUDE_NONE
    #include "GLFW/glfw.h"
#endif


namespace CoreEngine
{
    Window::Window(WindowCreationConfig config) noexcept
    : m_window_ptr(glfwCreateWindow(config.m_size.first, config.m_size.second, config.m_title.c_str(), nullptr, nullptr)), m_is_clickthrough_window(config.m_is_transparent_clickthrough)
    {
        ENGINE_ASSERT (m_window_ptr && "Failed to create window");

    #ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(m_window_ptr);
        
        if (config.m_is_windowed_fullscren)
        {
            LONG style = GetWindowLong(hwnd, GWL_STYLE);
            style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
            SetWindowLong(hwnd, GWL_STYLE, style);

            GLFWmonitor* monitor    = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            SetWindowPos(hwnd, HWND_TOP, 0, 0, mode->width, mode->height, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }

        if (config.m_is_transparent_clickthrough) 
        {
            LONG ex = GetWindowLong(hwnd, GWL_EXSTYLE);
            ex |= WS_EX_LAYERED;
            SetWindowLong(hwnd, GWL_EXSTYLE, ex);
            
            SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
            glfwSetWindowAttrib(m_window_ptr, GLFW_FLOATING, true);
        }
    #else
        m_is_clickthrough_window = false;
    #endif
    }

    Window::~Window() noexcept
    {
        if (m_window_ptr) 
        {
            glfwDestroyWindow(m_window_ptr);
            m_window_ptr = nullptr;
        }
    }

    ///////////////////////////////
    // Move
    ///////////////////////////////
    Window::Window(Window&& other) noexcept : m_window_ptr(other.m_window_ptr), m_is_clickthrough_window(other.m_is_clickthrough_window)
    {
        other.m_window_ptr = nullptr;
    }

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this != &other)
        {
            if (m_window_ptr)
            {
                glfwDestroyWindow(m_window_ptr);
            }

            m_window_ptr = other.m_window_ptr;
            m_is_clickthrough_window = other.m_is_clickthrough_window;

            other.m_window_ptr = nullptr;
        }
        return *this;
    }

    ///////////////////////////////
    // Methods
    ///////////////////////////////
    GLFWwindow* Window::GetRawPtr() const noexcept
    {
        return m_window_ptr;
    }

#ifdef _WIN32
    void Window::SetClickthrough(bool enable) noexcept 
    {
        HWND hwnd = glfwGetWin32Window(GetRawPtr());
        LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
        
        if (enable) 
        {
            style |= WS_EX_TRANSPARENT;
        } 
        else 
        {
            style &= ~WS_EX_TRANSPARENT;
        }
        
        SetWindowLong(hwnd, GWL_EXSTYLE, style);
    }
#endif

    bool Window::GetIsClickthrough() const noexcept
    {
        return m_is_clickthrough_window;
    }
}