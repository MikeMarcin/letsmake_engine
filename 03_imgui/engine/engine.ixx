module;

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <imgui_impl_win32.h>

#define MY_PRINTF(...) { char cad[512]; sprintf(cad, __VA_ARGS__); OutputDebugString(cad); }

export module LetsMakeEngine;

#if __INTELLISENSE__
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#else
#pragma warning(suppress:5050)
import std.core;
#endif

export namespace LetsMakeEngine {
    class Window;

    using tstring = std::basic_string<TCHAR>;
    using OnResizeHandler = std::function<bool(Window&, std::uint32_t, std::uint32_t)>;

    /// \brief Window Mode
    enum class WindowMode {
        windowed,
        borderless_window,
        fullscreen,
    };

    /// \brief Configuration for making a Window.
    struct WindowConfig {
        tstring             title;
        std::uint32_t       width = 1;
        std::uint32_t       height = 1;
        WindowMode          mode = WindowMode::windowed;
        bool                pauseAppWhenInactive = true;
        OnResizeHandler     onResize = nullptr;
    };

    std::unique_ptr<Window> MakeWindow(WindowConfig aConfig);
    bool PumpEvents();
}

namespace LetsMakeEngine {
    class Window {
    public:
        ~Window();

        HWND NativeHandle() const { return _hwnd; }

    private:
        friend std::unique_ptr<Window> MakeWindow(WindowConfig);
        friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
        LRESULT ProcessMessage(UINT aMsg, WPARAM aWparam, LPARAM aLparam);

        WindowConfig _config;
        HWND _hwnd = nullptr;
        WINDOWPLACEMENT _restorePlacement{ sizeof(WINDOWPLACEMENT) };
        bool _isFullscreen = false;
        bool _isMinimized = false;
        bool _isSuspended = false;
        bool _inSizemove = false;

        static constexpr LONG MinWidth = 320;
        static constexpr LONG MinHeight = 240;
    };
}

module :private;

// Forward declare message handler from imgui_impl_win32.cpp 
extern "C++" IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace LetsMakeEngine {
    Window::~Window() {
        if (_hwnd != nullptr) {
            DestroyWindow(_hwnd);
        }
    }

    LRESULT CALLBACK WndProc(HWND aHwnd, UINT aMsg, WPARAM aWparam, LPARAM aLparam) {
        if (ImGui_ImplWin32_WndProcHandler(aHwnd, aMsg, aWparam, aLparam)) {
            return true;
        }

        Window* self = nullptr;

        // see https://devblogs.microsoft.com/oldnewthing/20191014-00/?p=102992
        if ( aMsg != WM_NCCREATE ) {
            // GetWindowLongPtr returns 0 if GWLP_USERDATA has not been set yet.
            self = reinterpret_cast<Window*>(GetWindowLongPtr(aHwnd, GWLP_USERDATA));
        } else {
            auto createStruct = reinterpret_cast<LPCREATESTRUCT>(aLparam);
            self = reinterpret_cast<Window*>(createStruct->lpCreateParams);
            self->_hwnd = aHwnd; // initialize the window handle
            SetWindowLongPtr(aHwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        }

        if (self != nullptr) {
            return self->ProcessMessage(aMsg, aWparam, aLparam);
        }
        return DefWindowProc(aHwnd, aMsg, aWparam, aLparam);
    }
    
    std::pair<DWORD, DWORD> GetWindowStyle(WindowMode mode) {
        DWORD style;
        DWORD exStyle;
        switch (mode) {
        case WindowMode::windowed:
            style = WS_OVERLAPPEDWINDOW;
            exStyle = WS_EX_OVERLAPPEDWINDOW;
            break;
        case WindowMode::borderless_window:
            style = WS_POPUP;
            exStyle = 0;
            break;
        case WindowMode::fullscreen:
            style = WS_POPUP;
            exStyle = WS_EX_TOPMOST;
            break;
        }
        return std::make_pair(style, exStyle);
    }

    LRESULT Window::ProcessMessage(UINT aMsg, WPARAM aWparam, LPARAM aLparam) {
        switch (aMsg) {
        case WM_ACTIVATEAPP:
            if (_config.pauseAppWhenInactive) {
                const bool isBecomingActive = (aWparam == TRUE);
                const auto priorityClass = isBecomingActive ? NORMAL_PRIORITY_CLASS : IDLE_PRIORITY_CLASS;
                SetPriorityClass(GetCurrentProcess(), priorityClass);
            }
            break;
        case WM_SYSKEYDOWN:
            if (aWparam == VK_RETURN && (aLparam & 0x60000000) == 0x20000000) {
                // Implements the classic ALT+ENTER fullscreen toggle
                if (_isFullscreen) {
                    auto [style, exStyle] = GetWindowStyle(WindowMode::windowed);
                    SetWindowLongPtr(_hwnd, GWL_STYLE, style);
                    SetWindowLongPtr(_hwnd, GWL_EXSTYLE, exStyle);
                    SetWindowPlacement(_hwnd, &_restorePlacement);
                    SetWindowPos(_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                } else {
                    GetWindowPlacement(_hwnd, &_restorePlacement);
                    auto [style, exStyle] = GetWindowStyle(WindowMode::fullscreen);
                    SetWindowLongPtr(_hwnd, GWL_STYLE, style);
                    SetWindowLongPtr(_hwnd, GWL_EXSTYLE, exStyle);
                    SetWindowPos(_hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
                    ShowWindow(_hwnd, SW_SHOWMAXIMIZED);
                }
                _isFullscreen = !_isFullscreen;
            }
            break;
        case WM_CREATE:
            GetWindowPlacement(_hwnd, &_restorePlacement);
            break;
        case WM_MENUCHAR:
            // Ignore so we don't produce an error beep.
            return MAKELRESULT(0, MNC_CLOSE);
        case WM_PAINT: {
            PAINTSTRUCT ps;
            std::ignore = BeginPaint(_hwnd, &ps);
            EndPaint(_hwnd, &ps);
            break;
        }
        case WM_SIZE:
            if (!_inSizemove && aWparam != SIZE_MINIMIZED && _config.onResize) {
                if (_config.onResize(*this, static_cast<std::uint32_t>(LOWORD(aLparam)), static_cast<std::uint32_t>(HIWORD(aLparam)))) {
                    return 0;
                }
            }
            break;
        case WM_ENTERSIZEMOVE:
            _inSizemove = true;
            break;
        case WM_EXITSIZEMOVE:
            _inSizemove = false;
            if (_config.onResize) {
                RECT cr;
                GetClientRect(_hwnd, &cr);
                if (_config.onResize(*this, static_cast<std::uint32_t>(cr.right - cr.left), static_cast<std::uint32_t>(cr.bottom - cr.top))) {
                    return 0;
                }
            }
            break;
        case WM_GETMINMAXINFO: {
            auto info = reinterpret_cast<MINMAXINFO*>(aLparam);
            info->ptMinTrackSize.x = MinWidth;
            info->ptMinTrackSize.y = MinHeight;
            break;
        }
        case WM_SIZING: {
            // maintain aspec ratio when resizing
            auto& rect = *reinterpret_cast<RECT*>(aLparam);
            const float aspectRatio = float(_config.width) / _config.height;
            const DWORD style = GetWindowLong(_hwnd, GWL_STYLE);
            const DWORD exStyle = GetWindowLong(_hwnd, GWL_EXSTYLE);

            RECT emptyRect{};
            AdjustWindowRectEx(&emptyRect, style, false, exStyle);
            rect.left -= emptyRect.left;
            rect.right -= emptyRect.right;
            rect.top -= emptyRect.top;
            rect.bottom -= emptyRect.bottom;

            auto newWidth = std::max(rect.right - rect.left, MinWidth);
            auto newHeight = std::max(rect.bottom - rect.top, MinHeight);

            switch (aWparam) {
            case WMSZ_LEFT:
                newHeight = static_cast<LONG>(newWidth / aspectRatio);
                rect.left = rect.right - newWidth;
                rect.bottom = rect.top + newHeight;
                break;
            case WMSZ_RIGHT:
                newHeight = static_cast<LONG>(newWidth / aspectRatio);
                rect.right = rect.left + newWidth;
                rect.bottom = rect.top + newHeight;
                break;
            case WMSZ_TOP:
            case WMSZ_TOPRIGHT:
                newWidth = static_cast<LONG>(newHeight * aspectRatio);
                rect.right = rect.left + newWidth;
                rect.top = rect.bottom - newHeight;
                break;
            case WMSZ_BOTTOM:
            case WMSZ_BOTTOMRIGHT:
                newWidth = static_cast<LONG>(newHeight * aspectRatio);
                rect.right = rect.left + newWidth;
                rect.bottom = rect.top + newHeight;
                break;
            case WMSZ_TOPLEFT:
                newWidth = static_cast<LONG>(newHeight * aspectRatio);
                rect.left = rect.right - newWidth;
                rect.top = rect.bottom - newHeight;
                break;
            case WMSZ_BOTTOMLEFT:
                newWidth = static_cast<LONG>(newHeight * aspectRatio);
                rect.left = rect.right - newWidth;
                rect.bottom = rect.top + newHeight;
                break;            
            }
            AdjustWindowRectEx(&rect, style, false, exStyle);
            return TRUE;
        }
        case WM_NCDESTROY:
            // see https://devblogs.microsoft.com/oldnewthing/20050726-00/?p=34803
            PostQuitMessage(0);
            ImGui_ImplWin32_Shutdown();
            _hwnd = nullptr;
            SetWindowLongPtr(_hwnd, GWLP_USERDATA, LONG_PTR(0));
            break;
        }

        return DefWindowProc(_hwnd, aMsg, aWparam, aLparam);
    }

    std::unique_ptr<Window> MakeWindow(WindowConfig aConfig) {
        // Create the window
        auto window = std::make_unique<Window>();
        window->_config = std::move(aConfig);

        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = &WndProc;
        wcex.hInstance = GetModuleHandle(nullptr);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszClassName = "LetsMakeEngineWindow";
        wcex.hIconSm = LoadIcon(0, IDI_APPLICATION);
        auto wndclassAtom = RegisterClassEx(&wcex);
        if (!wndclassAtom) {
            throw std::system_error{ std::error_code(static_cast<int>(GetLastError()), std::system_category()) };
        }

        // Setup window style
        RECT windowRect{ 0, 0, static_cast<LONG>(aConfig.width), static_cast<LONG>(aConfig.height) };
        auto [style, exStyle] = GetWindowStyle(aConfig.mode);
        AdjustWindowRectEx( &windowRect, style, FALSE, exStyle );
                
        auto hwnd = CreateWindowEx(
            exStyle,
            MAKEINTATOM(wndclassAtom),
            window->_config.title.c_str(),
            style,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            windowRect.right - windowRect.left, // width
            windowRect.bottom - windowRect.top, // height
            HWND_DESKTOP, // parent
            0, // menu
            0, // application instance
            window.get() // lpCreateParams
        );

        if( !hwnd ) {
            std::error_code ec( static_cast<int>(GetLastError()), std::system_category() );
            UnregisterClass(MAKEINTATOM(wndclassAtom), GetModuleHandle(nullptr));
            throw std::system_error{ ec };
        }

        if (aConfig.mode == WindowMode::fullscreen) {
            ShowWindow(hwnd, SW_SHOWMAXIMIZED);
            window->_isFullscreen = true;
        } else {
            ShowWindow(hwnd, SW_SHOW);
        }

        ImGui_ImplWin32_Init(hwnd);
        return window;
    }

    bool PumpEvents() {
        MSG msg{};
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                return false;
            }
        }
        ImGui_ImplWin32_NewFrame();
        return true;
    }
}
