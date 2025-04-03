#define BYTES_PER_PIXEL 4

#include "game.h"
#include <stdint.h>

#define _USE_MATH_DEFINES
#include <math.h>

static void
OutputSound(GameSoundOutput *soundOutput)
{
    static float tSine;
    int16_t toneVolume = 3000;
    int16_t* sampleOut = soundOutput->sampleOut;
    int wavePeriod = soundOutput->samplesPerSec / soundOutput->toneHz;

	for (int i = 0; i < soundOutput->sampleCount; i++)
	{																			
		float sineValue = sinf(tSine); 
        int16_t sampleValue = (int16_t)(sineValue * toneVolume);
		*sampleOut++ = sampleValue;
		*sampleOut++ = sampleValue;
        tSine += 2. * M_PI * 1. / (float)wavePeriod;
	}																			
}

static void
RenderWeirdGradient(const PixelBackBuffer* buf, int xoffset, int yoffset)
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

void 
GameUpdateAndRender(PixelBackBuffer *pixelBuf, int blueOffset, int greenOffset, GameSoundOutput *soundBuf)
{
    OutputSound(soundBuf);
	RenderWeirdGradient(pixelBuf, blueOffset, greenOffset);
}


