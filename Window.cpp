#include "window.h"


#define WINDOW_GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define WINDOW_GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (msg) {
    case WM_CREATE:
    {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        window = (Window*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_INPUT:
	{
		UINT dwSize = sizeof(RAWINPUT);
		static BYTE lpb[sizeof(RAWINPUT)];

		GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

		RAWINPUT* raw = (RAWINPUT*)lpb;

		if (raw->header.dwType == RIM_TYPEMOUSE)
		{
			window->updateMouse(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
		}
        break;
	}

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        if (window) {
            window->keys[wParam] = true;
        }
        break;
    case WM_KEYUP:
        if (window) {
            window->keys[wParam] = false;
        }
        break;
    case WM_LBUTTONDOWN:
    {
        if (window) {
           // window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
            window->mouseButtons[0] = true;
        }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (window) {
			//window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
            window->mouseButtons[0] = false;
        }
        break;
    }
  //  case WM_MOUSEMOVE:
  //  {
		////if (window)window->updateMouse(WINDOW_GET_X_LPARAM(lParam), WINDOW_GET_Y_LPARAM(lParam));
  //      break;
  //  }

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
	return 0;
}


void Window::updateMouse(int x, int y)
{
	//float dx = static_cast<float>( x - mousex ) * sensitivity;
	//float dy = static_cast<float>( y - mousey ) * sensitivity;
 //   player->updateCamera(dx, dy);

 //   mousex = x;
 //   mousey = y;

    float dx = static_cast<float>(x) * sensitivity;
    float dy = static_cast<float>(y) * sensitivity;
    player->updateCamera(dx, dy);
}

void Window::create(int window_width, int window_height)
{
    WNDCLASSEX wc = {0};
    hinstance = GetModuleHandle(NULL);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinstance;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = L"WindowClass";
    wc.cbSize = sizeof(WNDCLASSEX);
    RegisterClassEx(&wc);

    width = window_width;
    height = window_height;
  
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, L"WindowClass", L"Game", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
        0, 0, width, height, NULL, NULL, hinstance, this);

	//register raw input
    RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x02;
	rid.dwFlags = RIDEV_INPUTSINK;
	rid.hwndTarget = hwnd;
    RegisterRawInputDevices(&rid, 1, sizeof(rid));
}

bool Window::processMessages()
{
    MSG msg;
    ZeroMemory(&msg, sizeof(MSG));
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
		if (msg.message == WM_QUIT)
			return true;
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	return false;
}
