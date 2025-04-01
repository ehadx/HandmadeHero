#include "framework.h"
#include "BitmapBackBuffer.h"

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(XInputGetState_);

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(XInputSetState_);

X_INPUT_GET_STATE(XInputGetStateStub)
{ 
    UNREFERENCED_PARAMETER(pState);
    UNREFERENCED_PARAMETER(dwUserIndex);
    return ERROR_DEVICE_NOT_CONNECTED;
}

X_INPUT_SET_STATE(XInputSetStateStub)
{ 
    UNREFERENCED_PARAMETER(pVibration);
    UNREFERENCED_PARAMETER(dwUserIndex);
    return ERROR_DEVICE_NOT_CONNECTED;
}

static bool Running;
static BitmapBackBuffer buf;
static XInputSetState_* XInputSetState__;
static XInputGetState_* XInputGetState__;

static void LoadXInput(void)
{
	HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!xInputLibrary) xInputLibrary = LoadLibraryA("xinput9_1_0.dll");

	if (xInputLibrary)
	{
		XInputGetState_* xInputGetState = (XInputGetState_*)GetProcAddress(xInputLibrary, "XInputGetState");
		if (!xInputGetState) xInputGetState = XInputGetStateStub;
		XInputSetState_* xInputSetState = (XInputSetState_*)GetProcAddress(xInputLibrary, "XInputSetState");
		if (!xInputSetState) xInputSetState = XInputSetStateStub;
		XInputGetState__ = xInputGetState;
		XInputSetState__ = xInputSetState;
	}
}

typedef struct
{
	int width;
	int height;
}
WindowDimension;

static WindowDimension
GetWindowDimension(HWND hWnd)
{
    WindowDimension result = {0};
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	result.width = clientRect.right - clientRect.left;
	result.height = clientRect.bottom - clientRect.top;
	return result;
}

static void
WindowSizeHandler(HWND hWnd) 
{
    OutputDebugStringW(L"WM_SIZE\n");
}

static void
WindowPaintHandler(HWND hWnd)
{
	WindowDimension dimension = GetWindowDimension(hWnd);
    PAINTSTRUCT paint;
    HDC deviceContext = BeginPaint(hWnd, &paint);
    RECT pRect = paint.rcPaint;
    DisplayBufferInWindow(&buf, dimension.width, dimension.height, deviceContext);
    EndPaint(hWnd, &paint);
}

static void
WindowKeyboardHandler(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	uint32_t code = wParam;
	bool wasDown = (lParam & (1ll << 30)) != 0;
	bool isDown = (lParam & (1ll << 31)) == 0;

	if (isDown && wasDown) return;

    if (code == 'W')
    {
    }
    else if (code == 'A')
    {
    }
    else if (code == 'D')
    {
    }
    else if (code == 'S')
    {
    }
    else if (code == 'Q')
    {
    }
    else if (code == 'E')
    {
    }
    else if (code == VK_UP)
    {
    }
    else if (code == VK_LEFT)
    {
    }
    else if (code == VK_DOWN)
    {
    }
    else if (code == VK_RIGHT)
    {
    }
    else if (code == VK_ESCAPE)
    {
    }
    else if (code == VK_SPACE)
    {
    }

	bool altKeyIsDown = (lParam & (1ll << 29)) != 0;
    if (code == VK_F4 && altKeyIsDown)
    {
		Running = false;
    }
}

// Processes messages for the main window.
static LRESULT CALLBACK 
WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
    case WM_SIZE:
        WindowSizeHandler(hWnd);
        break;

    case WM_DESTROY:
        // handle this with an error
        Running = false;
        break;

    case WM_CLOSE:
        // handle this with a message to the user
        Running = false;
        break;

    case WM_ACTIVATEAPP:
        OutputDebugStringW(L"WM_ACTIVATEAPP\n");
        break;

    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP:
    case WM_KEYDOWN:
		WindowKeyboardHandler(hWnd, wParam, lParam);
        break;

    case WM_PAINT:
        WindowPaintHandler(hWnd);
        break;

    default:
        result = DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }

    return result;
}

int APIENTRY
wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	WNDCLASSEXW windowClass = {
		.cbSize = sizeof(WNDCLASSEX),
		.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		.lpfnWndProc = WindowProcedure,
		.hInstance = hInstance,
		.lpszClassName = L"HAND_MADE_HERO",
	};

	RegisterClassExW(&windowClass);

	HWND hWnd;
	if (!(hWnd = CreateWindowExW(
		0,
		windowClass.lpszClassName,
		L"Handmade Hero",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	))) return FALSE;

    Running = true;
    int xoffset = 0, yoffset = 0;
    HDC drawCtx = GetDC(hWnd);

    ResizeDIBSection(&buf, 1200, 720);
    LoadXInput();
    while (Running)
    {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT) Running = false;

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        // Should we poll this more frequently?
        for (DWORD ctrlIdx = 0; ctrlIdx  < XUSER_MAX_COUNT; ctrlIdx++)
        {
			XINPUT_STATE controllerState;
			if (XInputGetStateStub(ctrlIdx, &controllerState) == ERROR_SUCCESS)
			{
				// This controller is plugged in
				// See if ControllerState dwPacketNumber increments too rapidly
				XINPUT_GAMEPAD* pad = &controllerState.Gamepad;
                bool up = pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                bool down = pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                bool left = pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                bool right = pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                bool start = pad->wButtons & XINPUT_GAMEPAD_START;
                bool back = pad->wButtons & XINPUT_GAMEPAD_BACK;
                bool leftShoulder = pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                bool rightShoulder = pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                bool aButton = pad->wButtons & XINPUT_GAMEPAD_A;
                bool bButton = pad->wButtons & XINPUT_GAMEPAD_B;
                bool xButton = pad->wButtons & XINPUT_GAMEPAD_X;
                bool yButton = pad->wButtons & XINPUT_GAMEPAD_Y;

                int16_t lStickX = pad->sThumbLX;
                int16_t lStickY = pad->sThumbLY;

                xoffset += lStickX >> 12;
				yoffset += lStickY >> 12;

				UNREFERENCED_PARAMETER(up);
				UNREFERENCED_PARAMETER(down);
				UNREFERENCED_PARAMETER(left);
				UNREFERENCED_PARAMETER(right);
				UNREFERENCED_PARAMETER(start);
				UNREFERENCED_PARAMETER(back);
				UNREFERENCED_PARAMETER(leftShoulder);
				UNREFERENCED_PARAMETER(rightShoulder);
				UNREFERENCED_PARAMETER(bButton);
				UNREFERENCED_PARAMETER(aButton);
				UNREFERENCED_PARAMETER(xButton);
				UNREFERENCED_PARAMETER(yButton);
			}
			else
			{
				// This controller is not available
			}
        }
        RenderWeirdGradient(&buf, xoffset, yoffset);
		WindowDimension dimension = GetWindowDimension(hWnd);
        DisplayBufferInWindow(&buf, dimension.width, dimension.height, drawCtx);
        xoffset += 1;
    }

    return TRUE;
}
