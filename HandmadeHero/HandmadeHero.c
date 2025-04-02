#include "framework.h"
#include "BitmapBackBuffer.h"

#define _USE_MATH_DEFINES
#include <math.h>

#define X_INPUT_GET_STATE(name) \
	DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(XInputGetState_);

#define X_INPUT_SET_STATE(name) \
	DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
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

static XInputSetState_* XInputSetState__;
static XInputGetState_* XInputGetState__;

#define DIRECT_SOUND_CREATE(name) \
	HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(DirectSoundCreate_);

static bool TheIsGameRunning;
static BitmapBackBuffer ThePixelBuffer;
static LPDIRECTSOUNDBUFFER TheSoundBuffer;

static void 
LoadXInput(void)
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

static void
InitDirectSound(HWND hWnd, int32_t samplesPerSec, int32_t bufferSize)
{
	HMODULE directSoundLibrary = LoadLibraryA("dsound.dll");
    if (!directSoundLibrary)
	{
		OutputDebugStringA("Could not load dsound.dll\n");
		return;
	}

	DirectSoundCreate_* directSoundCreate = (DirectSoundCreate_*)
		GetProcAddress(directSoundLibrary, "DirectSoundCreate");

	LPDIRECTSOUND directSound = NULL;
	if (!directSoundCreate || !SUCCEEDED(directSoundCreate(0, &directSound, 0)))
	{
        OutputDebugStringA("Cannot initialize direct sound\n");
		return;
	}

	if (!SUCCEEDED(IDirectSound_SetCooperativeLevel(directSound, hWnd, DSSCL_PRIORITY)))
	{
        OutputDebugStringA("Cannot set cooperative level\n");
		return;
	}
	
	DSBUFFERDESC priBufferDesc = {
        .dwSize = sizeof(priBufferDesc),
        .dwFlags = DSBCAPS_PRIMARYBUFFER,
    };

    // all this is doing is getting a handle to the primary sound card so we can set
    // its format which puts the sound card into a mode that allows it to play in the
	// format we want
	LPDIRECTSOUNDBUFFER primaryBuffer = NULL;
	if (!SUCCEEDED(IDirectSound_CreateSoundBuffer(directSound, &priBufferDesc, &primaryBuffer, 0)))
	{
        OutputDebugStringA("Cannot create primary buffer\n");
		return;
	}

    WAVEFORMATEX waveFormat = { 0 };
	waveFormat.cbSize = 0; 
	waveFormat.wFormatTag = WAVE_FORMAT_PCM;  
	waveFormat.nChannels = 2;  
	waveFormat.nSamplesPerSec = samplesPerSec;  
	waveFormat.wBitsPerSample = 16;  
	waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;  
	waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    
    if (!SUCCEEDED(IDirectSoundBuffer_SetFormat(primaryBuffer, &waveFormat)))
    {
        OutputDebugStringA("Cannot set primary buffer format\n");
        return;
    }

	DSBUFFERDESC secBufferDesc = {
        .dwSize = sizeof(secBufferDesc),
        .dwBufferBytes = bufferSize,
		.lpwfxFormat = &waveFormat,
    };

	// the buffer we actually write to, we are not allowed in these days to write to the
	// primary buffer, which will write directly to the sound card, this is why we need
    // a secondary buffer.
	if (!SUCCEEDED(IDirectSound_CreateSoundBuffer(directSound, &secBufferDesc, &TheSoundBuffer, 0)))
	{
        OutputDebugStringA("Cannot create secondary buffer\n");
		return;
	}

    OutputDebugStringA("DIRECT SOUND INITIALIZED\n");
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
    DisplayBufferInWindow(&ThePixelBuffer, dimension.width, dimension.height, deviceContext);
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
		TheIsGameRunning = false;
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
        TheIsGameRunning = false;
        break;

    case WM_CLOSE:
        // handle this with a message to the user
        TheIsGameRunning = false;
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
        result = DefWindowProcA(hWnd, message, wParam, lParam);
        break;
    }

    return result;
}

static void
XInputLoop(int *xoffset, int *yoffset)
{
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

			*xoffset += lStickX / 4096;
			*yoffset += lStickY / 4096;

			DBG_UNREFERENCED_LOCAL_VARIABLE(up);
			DBG_UNREFERENCED_LOCAL_VARIABLE(down);
			DBG_UNREFERENCED_LOCAL_VARIABLE(left);
			DBG_UNREFERENCED_LOCAL_VARIABLE(right);
			DBG_UNREFERENCED_LOCAL_VARIABLE(start);
			DBG_UNREFERENCED_LOCAL_VARIABLE(back);
			DBG_UNREFERENCED_LOCAL_VARIABLE(leftShoulder);
			DBG_UNREFERENCED_LOCAL_VARIABLE(rightShoulder);
			DBG_UNREFERENCED_LOCAL_VARIABLE(bButton);
			DBG_UNREFERENCED_LOCAL_VARIABLE(aButton);
			DBG_UNREFERENCED_LOCAL_VARIABLE(xButton);
			DBG_UNREFERENCED_LOCAL_VARIABLE(yButton);
		}
		else
		{
			// This controller is not available
		}
	}
}

typedef struct
{
	int samplesPerSec;
	int toneHz;
	int16_t toneVolume;
	uint32_t wavePeriod;
	uint32_t bytesPerSample;
	uint32_t soundBufferSize;
	uint32_t sampleIndex;
}
SoundOutput;

static void
FillSoundBuffer(SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite)
{
	void *region1, *region2;
	DWORD region1Size, region2Size;
	if (SUCCEEDED(IDirectSoundBuffer_Lock(
		TheSoundBuffer, byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0)))
	{
#define DS_BUF_REGION_WRITE_LOOP(region, regionSize)									\
		{																				\
			DWORD sampleCount = regionSize / soundOutput->bytesPerSample;				\
			int16_t* sampleOut = (int16_t*)region;										\
			for (DWORD sampleIdx = 0; sampleIdx < sampleCount; sampleIdx++)				\
			{																			\
				float t = 2.0f * M_PI *													\
					(float)soundOutput->sampleIndex / (float)soundOutput->wavePeriod;	\
				float sineValue = sinf(t); 												\
				int16_t sampleValue = (int16_t)(sineValue * soundOutput->toneVolume);	\
				*sampleOut++ = sampleValue;												\
				*sampleOut++ = sampleValue;												\
				soundOutput->sampleIndex++;												\
			}																			\
		}

		DS_BUF_REGION_WRITE_LOOP(region1, region1Size);
		DS_BUF_REGION_WRITE_LOOP(region2, region2Size);

		IDirectSoundBuffer_Unlock(TheSoundBuffer, region1, region1Size, region2, region2Size);
	}
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
	
	bool soundIsPlaying = false;

	SoundOutput soundOutput = {
		.samplesPerSec = 48000,
		.toneHz = 256,
		.toneVolume = 1000,
		.wavePeriod = soundOutput.samplesPerSec / soundOutput.toneHz,
		.bytesPerSample = sizeof(int16_t) * 2,
		.soundBufferSize = soundOutput.samplesPerSec * soundOutput.bytesPerSample,
		.sampleIndex = 0
	};

	InitDirectSound(hWnd, soundOutput.samplesPerSec, soundOutput.soundBufferSize);
	FillSoundBuffer(&soundOutput, 0, soundOutput.soundBufferSize);
	IDirectSoundBuffer_Play(TheSoundBuffer, 0, 0, DSBPLAY_LOOPING);

    ResizeDIBSection(&ThePixelBuffer, 1200, 720);
    LoadXInput();

    int xoffset = 0, yoffset = 0;
    HDC drawCtx = GetDC(hWnd);
    TheIsGameRunning = true;

    while (TheIsGameRunning)
    {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT) TheIsGameRunning = false;

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

		XInputLoop(&xoffset, &yoffset);
        RenderWeirdGradient(&ThePixelBuffer, xoffset, yoffset);

		DWORD playCursor, writeCursor;
		if (!soundIsPlaying && 
			SUCCEEDED(IDirectSoundBuffer_GetCurrentPosition(TheSoundBuffer, &playCursor, &writeCursor)))
		{
			DWORD bytesToWrite, 
				byteToLock = soundOutput.sampleIndex * soundOutput.bytesPerSample % soundOutput.soundBufferSize;

			if (byteToLock == playCursor)
				bytesToWrite = 0;
			else if (byteToLock > playCursor)
			{
				bytesToWrite = soundOutput.soundBufferSize - byteToLock;
				bytesToWrite += playCursor;
			}
			else bytesToWrite = playCursor - byteToLock;

			FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
		}
		
		WindowDimension dimension = GetWindowDimension(hWnd);
        DisplayBufferInWindow(&ThePixelBuffer, dimension.width, dimension.height, drawCtx);
        xoffset += 1;
    }

    return TRUE;
}
