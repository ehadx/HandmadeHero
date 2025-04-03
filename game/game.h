#pragma once
#include <stdint.h>

typedef struct
{
    // BITMAPINFO info;
    void* mem;
    int height;
    int width;
    //int pitch;
}
PixelBackBuffer;

typedef struct
{
	int16_t* sampleOut;
	int sampleCount;
	int samplesPerSec;
    int toneHz;
}
GameSoundOutput;

void GameUpdateAndRender(PixelBackBuffer*, int, int, GameSoundOutput *);

