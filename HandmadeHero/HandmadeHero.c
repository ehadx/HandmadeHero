#include "framework.h"
#include "BitmapBackBuffer.h"

#define RectW(r) r.right - r.left
#define RectH(r) r.bottom - r.top
#define PRectW(r) r->right - r->left
#define PRectH(r) r->bottom - r->top

static bool Running;
static BitmapBackBuffer buf;

typedef struct
{
	int width;
	int height;
} WindowDimension;

static WindowDimension
GetWindowDimension(HWND hWnd)
{
    WindowDimension result = {0};
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	result.width = RectW(clientRect);
	result.height = RectH(clientRect);
	return result;
}

static void
WindowSizeHandler(HWND hWnd) 
{
	WindowDimension dimension = GetWindowDimension(hWnd);
    ResizeDIBSection(&buf, dimension.width, dimension.height);
    OutputDebugStringW(L"WM_SIZE\n");
}

static void
WindowPaintHandler(HWND hWnd)
{
	WindowDimension dimension = GetWindowDimension(hWnd);
    PAINTSTRUCT paint;
    HDC deviceContext = BeginPaint(hWnd, &paint);
    RECT pRect = paint.rcPaint;
    DisplayBufferInWindow(
        &buf,
        dimension.width,
		dimension.height,
        deviceContext, 
        pRect.left, 
        pRect.top, 
        RectW(pRect), 
        RectH(pRect)
    );
    EndPaint(hWnd, &paint);
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
		.style = CS_HREDRAW | CS_VREDRAW,
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

    while (Running)
    {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT) Running = false;

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

        RenderWeirdGradient(&buf, xoffset, 0);

		WindowDimension dimension = GetWindowDimension(hWnd);
        HDC deviceContext = GetDC(hWnd);
        DisplayBufferInWindow(
            &buf, dimension.width, dimension.height,
            deviceContext, 0, 0, dimension.width, dimension.height
        );
        ReleaseDC(hWnd, deviceContext);

        ++xoffset;
    }

    return TRUE;
}
