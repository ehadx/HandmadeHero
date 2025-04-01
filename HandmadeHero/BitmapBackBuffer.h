#pragma once

#include "framework.h"

#define BYTES_PER_PIXEL 4;


typedef struct
{
    BITMAPINFO info;
    void* mem;
    LONG height;
    LONG width;
}
BitmapBackBuffer;

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
        buf->width,
        buf->height,
        buf->mem,
        &buf->info,
        DIB_RGB_COLORS,
        SRCCOPY
    );
}

static void
RenderWeirdGradient(const BitmapBackBuffer* buf, int xoffset, int yoffset)
{
    // how many bytes to move to get to the next row
	int pitch = buf->width * BYTES_PER_PIXEL; 

    // to move the pointer by 8 bit factor
    uint8_t* row = (uint8_t*)buf->mem; 

    for (int y = 0; y < buf->height; ++y)
    {
        uint32_t* pixel = (uint32_t*)row;
        for (int x = 0; x < buf->width; ++x)
        {
            /* ptr           +0 +1 +2 +3
             * lit-endian:   xx rr gg bb as it is in memory
             * win-bitmap:   bb gg rr xx as we load it
             */
            uint8_t blue = (x + xoffset);
            uint8_t green = (y + yoffset);

            *pixel++ = (green << 8) | blue;
			// *pixel = (green << 8) | blue; write to the pixel
			// pixel = pixel + 1; move to the next pixel
        }
        row += pitch;
    }
}

static void
ResizeDIBSection(BitmapBackBuffer* buf, LONG width, LONG height)
{
    if (buf->mem) VirtualFree(buf->mem, 0, MEM_RELEASE);

    buf->width = width;
    buf->height = height;

    buf->info.bmiHeader.biSize = sizeof(buf->info.bmiHeader);
    buf->info.bmiHeader.biWidth = buf->width;
    buf->info.bmiHeader.biHeight = buf->height;
    buf->info.bmiHeader.biPlanes = 1;
    buf->info.bmiHeader.biBitCount = 32; // DWORD aligne
    buf->info.bmiHeader.biCompression = BI_RGB;

    LONG bitmapMemorySize = buf->width * buf->height * BYTES_PER_PIXEL;
    buf->mem = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

