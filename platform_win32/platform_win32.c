#include "framework.h"
#include "../game/game.h"

#define _USE_MATH_DEFINES
#include <math.h>

#define X_INPUT_GET_STATE(name) \
	DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(XInputGetState_);

#define X_INPUT_SET_STATE(name) \
	DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(XInputSetState_);

static X_INPUT_GET_STATE(XInputGetStateStub)
{ 
    UNREFERENCED_PARAMETER(pState);
    UNREFERENCED_PARAMETER(dwUserIndex);
    return ERROR_DEVICE_NOT_CONNECTED;
}

static X_INPUT_SET_STATE(XInputSetStateStub)
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

typedef struct
{
	BITMAPINFO info;
	PixelBackBuffer pixelsBuf;
}
BitmapBackBuffer;

static bool TheIsGameRunning;
static BitmapBackBuffer ThePixelBuffer;
static LPDIRECTSOUNDBUFFER TheSoundBuffer;

static void
DisplayBufferInWindow(const BitmapBackBuffer* buf, LONG windowWidth, LONG windowHeight, HDC dc)
{
    StretchDIBits(
        dc, 
        0,
        0,
        windowWidth,
        windowHeight,
        0,
        0,
        buf->pixelsBuf.width,
        buf->pixelsBuf.height,
        buf->pixelsBuf.mem,
        &buf->info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

#define BYTES_PER_PIXEL 4

static void
ResizeDIBSection(BitmapBackBuffer* buf, LONG width, LONG height)
{
    if (buf->pixelsBuf.mem) VirtualFree(buf->pixelsBuf.mem, 0, MEM_RELEASE);

    buf->pixelsBuf.width = width;
    buf->pixelsBuf.height = height;

    buf->info.bmiHeader.biSize = sizeof(buf->info.bmiHeader);
    buf->info.bmiHeader.biWidth = buf->pixelsBuf.width;
    buf->info.bmiHeader.biHeight = buf->pixelsBuf.height;
    buf->info.bmiHeader.biPlanes = 1;
    buf->info.bmiHeader.biBitCount = 32; // DWORD aligne
    buf->info.bmiHeader.biCompression = BI_RGB;

    LONG bitmapMemorySize = buf->pixelsBuf.width * buf->pixelsBuf.height * BYTES_PER_PIXEL;
    buf->pixelsBuf.mem = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}


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
XInputLoop(int *xoffset, int *yoffset, GameSoundOutput *soundOutput)
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

			soundOutput->toneHz = 512 + (int)(256. * lStickY / 30000.0);

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
	unsigned int sampleIndex;
	int toneVolume;
	int toneHz;
	int wavePeriod;
	int bytesPerSample;
	int soundBufferSize;
	int latencySampleCount;
}
SoundOutput;

static void
FillSoundBuffer(SoundOutput* soundOutput, GameSoundOutput* gameSoundOutput,  DWORD byteToLock, DWORD bytesToWrite)
{
	void *region1, *region2;
	DWORD region1Size, region2Size;
	if (SUCCEEDED(IDirectSoundBuffer_Lock(
		TheSoundBuffer, byteToLock, bytesToWrite, &region1, &region1Size, &region2, &region2Size, 0)))
	{
#define DS_BUF_REGION_WRITE_LOOP(region, regionSize)								\
		{																			\
			DWORD regionSampleCount = regionSize / soundOutput->bytesPerSample;		\
            int16_t* destSample = (int16_t*)region;									\
			int16_t* sourceSample = (int16_t*)gameSoundOutput->sampleOut;			\
			for (DWORD i = 0; i < regionSampleCount; i++)							\
			{																		\
				*destSample++ = *sourceSample++;									\
				*destSample++ = *sourceSample++;									\
				soundOutput->sampleIndex++;											\
			}																		\
		}

		DS_BUF_REGION_WRITE_LOOP(region1, region1Size);
		DS_BUF_REGION_WRITE_LOOP(region2, region2Size);

		IDirectSoundBuffer_Unlock(TheSoundBuffer, region1, region1Size, region2, region2Size);
	}
}

static void
ClearSoundBuffer(int32_t soundBufferSize)
{
	VOID* region1, * region2;
	DWORD region1Size, region2Size;
	if (!SUCCEEDED(IDirectSoundBuffer_Lock(
		TheSoundBuffer, 0, soundBufferSize, &region1, &region1Size, &region2, &region2Size, 0
	)))
	{
		OutputDebugStringA("Cannot lock sound buffer\n");
		return;
	}
	int8_t* sampleOut = (int8_t*)region1;
	for (DWORD sampleIdx = 0; sampleIdx < region1Size; sampleIdx++)
		*sampleOut++ = 0;

	sampleOut = (int8_t*)region2;
	for (DWORD sampleIdx = 0; sampleIdx < region2Size; sampleIdx++)
		*sampleOut++ = 0;

	IDirectSoundBuffer_Unlock(TheSoundBuffer, region1, region1Size, region2, region2Size);
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

	LARGE_INTEGER perfCountFrequencyResult;
	QueryPerformanceFrequency(&perfCountFrequencyResult);
	int64_t perfCountFrequency = perfCountFrequencyResult.QuadPart;

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
	
	SoundOutput soundOutput = {
		.samplesPerSec = 48000,
		.sampleIndex = 0,
		.toneVolume = 3000,
		.toneHz = 256,
		.wavePeriod = 48000 / 256,
		.bytesPerSample = sizeof(int16_t) * 2,
		.soundBufferSize = 48000 * sizeof(int16_t) * 2,
		.latencySampleCount = 48000 % 15,
	};
	int16_t* samples = VirtualAlloc(0, soundOutput.soundBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	InitDirectSound(hWnd, soundOutput.samplesPerSec, soundOutput.soundBufferSize);
	ClearSoundBuffer(soundOutput.soundBufferSize);
	IDirectSoundBuffer_Play(TheSoundBuffer, 0, 0, DSBPLAY_LOOPING);

    ResizeDIBSection(&ThePixelBuffer, 1200, 720);
    LoadXInput();

    int xoffset = 0, yoffset = 0;
    HDC drawCtx = GetDC(hWnd);
    TheIsGameRunning = true;

	LARGE_INTEGER lastCounter;
	QueryPerformanceCounter(&lastCounter);
	LONGLONG lastCycleCount = __rdtsc();
    while (TheIsGameRunning)
    {
        MSG message;
        while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
        {
            if (message.message == WM_QUIT) TheIsGameRunning = false;

            TranslateMessage(&message);
            DispatchMessage(&message);
        }

		XInputLoop(&xoffset, &yoffset, &TheSoundBuffer);

		DWORD playCursor, writeCursor, byteToLock, targetCursor, bytesToWrite;
		bool soundIsValid = false;
		if (SUCCEEDED(IDirectSoundBuffer_GetCurrentPosition(TheSoundBuffer, &playCursor, &writeCursor)))
		{
			byteToLock = (soundOutput.sampleIndex * soundOutput.bytesPerSample) % soundOutput.soundBufferSize;
			targetCursor = (playCursor + soundOutput.latencySampleCount * soundOutput.bytesPerSample) % soundOutput.soundBufferSize;

			if (byteToLock > targetCursor)
			{
				bytesToWrite = soundOutput.soundBufferSize - byteToLock;
				bytesToWrite += targetCursor;
			}
			else bytesToWrite = targetCursor - byteToLock;

			soundIsValid = true;
		}

		GameSoundOutput gameSoundOutput = { 0 };
		gameSoundOutput.sampleOut = samples;
		gameSoundOutput.samplesPerSec = soundOutput.samplesPerSec;
		gameSoundOutput.sampleCount = bytesToWrite / soundOutput.bytesPerSample;
		gameSoundOutput.toneHz = soundOutput.toneHz;

		GameUpdateAndRender(&ThePixelBuffer.pixelsBuf, xoffset, yoffset, &gameSoundOutput);

		if (soundIsValid)
			FillSoundBuffer(&soundOutput, &gameSoundOutput, byteToLock, bytesToWrite);
		
		LONGLONG endCycleCount = __rdtsc();
		LARGE_INTEGER endCounter;
		QueryPerformanceCounter(&endCounter);
		LONGLONG cyclesElapsed = endCycleCount - lastCycleCount;
		LONGLONG counterElapsed = endCounter.QuadPart - lastCounter.QuadPart;
		LONGLONG millisecPerFrame = 1000 * counterElapsed / perfCountFrequency;
		LONGLONG fps = perfCountFrequency / counterElapsed;
		LONGLONG megaCyclesPerFrame = cyclesElapsed / (1000ll * 1000);

		char buffer[256];
		wsprintfA(buffer, "%I64dms/f, %I64dfps, %I64dmc/f\n",
			millisecPerFrame, fps, megaCyclesPerFrame);
		OutputDebugStringA(buffer);

		// QueryPerformaceCounter does not always give us a consistent time because process
		// can be interrupted by the OS, so we are always setting the lastCounter to the
		// endCounter, so we can get a consistent time.
		lastCounter = endCounter;
		lastCycleCount = endCycleCount;

		WindowDimension dimension = GetWindowDimension(hWnd);
        DisplayBufferInWindow(&ThePixelBuffer, dimension.width, dimension.height, drawCtx);
        xoffset += 1;
    }

    return TRUE;
}
