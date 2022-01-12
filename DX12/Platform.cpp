#include"stdafx.h"

#include "Platform.h"

namespace platform{

    LRESULT CALLBACK msgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
        switch( msg ){
        case WM_DESTROY: // this message is read when the window is closed
            PostQuitMessage(0);
            break;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

	Window::Window(const int w, const int h) : width(w), height(h)
	{
        HINSTANCE hInstC = GetModuleHandle(0);
        WNDCLASSEX wc{0};
        
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.style = CS_OWNDC | CS_VREDRAW;
        wc.lpfnWndProc = msgProc;
        wc.hInstance = hInstC;
        wc.lpszClassName = "myClass";
        wc.hbrBackground = ((HBRUSH)COLOR_ACTIVEBORDER);
        wc.hCursor = 0;
        
        RegisterClassEx(&wc);
        
        DWORD window_style = WS_OVERLAPPEDWINDOW; //WS_VISIBLE shows window
        RECT wr ={0, 0, width, height};
        AdjustWindowRect(&wr, window_style, FALSE);
        
        hwnd = CreateWindowEx(NULL,
            "myClass",    // name of the window class
            "hi",   // title of the window
            window_style,   //window style
            CW_USEDEFAULT,    // x-position of the window
            CW_USEDEFAULT,    // y-position of the window
            wr.right-wr.left,    // width of the window
            wr.bottom-wr.top,    // height of the window
            NULL,    // we have no parent window, NULL
            NULL,    // we aren't using menus, NULL
            hInstC,    // application handle
            NULL);    // used with multiple windows, NULL
        
        //if( !hwnd ) printf("failed to create window!");


        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        inverse_count_freq = double(1)/freq.QuadPart;
        QueryPerformanceCounter(&start_count);
	}

    void Window::show() {
        ShowWindow(hwnd, SW_SHOW);
    }
    bool Window::poll(Window::PollEvent& w_event)
    {  
        w_event = platform::Window::PollEvent::POLL_EVENT_NONE;
        if( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) >0){
            // translate keystroke messages into the right format
            TranslateMessage(&msg);

            // send the message to the WindowProc function
            if( msg.message == WM_QUIT ) {   }

            switch( msg.message ) {
            case WM_QUIT:
                w_event = platform::Window::PollEvent::POLL_EVENT_QUIT;
                return false;
            case WM_SIZE:
                w_event = platform::Window::PollEvent::POLL_EVENT_RESIZED;
                break;
            }
            DispatchMessage(&msg);

            return true;
        }
        return false;
    }

    void Window::setTitle(const char* cstr){
        SetWindowTextA(hwnd, (LPCSTR)cstr);
    }
    double Window::getTime() {
        LARGE_INTEGER now_count;
        BOOL b = QueryPerformanceCounter(&now_count);
        return (now_count.QuadPart - start_count.QuadPart)*inverse_count_freq;
    }

    Input::Input(Window* w):window(w)
    {

    }
    void Input::getButton(int code,Button& butt){
        bool b = GetAsyncKeyState(code)>>16;
        butt.changed = (b!=butt.down);
        butt.down = b;
    }
}