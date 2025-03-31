#pragma once

#include "framework.h"

#define BYTES_PER_PIXEL 4;

typedef struct
{
    BITMAPINFO info;
    LONG height;
    LONG width;
    void* mem;
}
BitmapBackBuffer;

static void
DisplayBufferInWindow(
    const BitmapBackBuffer* buf, LONG windowWidth, LONG windowHeight, HDC dc,
    LONG x, LONG y, LONG w, LONG h)
{
    StretchDIBits(
        dc,
        // x, y, w, h, x, y, w, h, 
        0, 0, buf->width, buf->height, 0, 0,
        windowWidth, windowHeight,
        buf->mem, &buf->info, DIB_RGB_COLORS, SRCCOPY
    );
}

static void
RenderWeirdGradient(const BitmapBackBuffer* buf, int xoffset, int yoffset)
{
    int pitch = buf->width * BYTES_PER_PIXEL;
    uint8_t* row = (uint8_t*)buf->mem; // to move the pointer by 8 bytes factor
    for (int y = 0; y < buf->height; ++y)
    {
        uint32_t* pixel = (uint32_t*)row;
        for (int x = 0; x < buf->width; ++x)
        {
            /* ptr           +0 +1 +2 +3
             * pixel in mem: 00 00 00 00
             * lit-endian:   xx bb gg rr
             * win-bitmap:   bb gg rr xx
             */
            uint8_t blue = (x + xoffset);
            uint8_t green = (y + yoffset);

            *pixel++ = (green << 8) | blue;
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
    buf->mem = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

