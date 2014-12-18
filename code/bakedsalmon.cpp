#include "bakedsalmon.h"

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
	int XThresh = 1;
	int XCur = 1;
	uint8 *Row = (uint8*)Buffer->Memory;
	win32_pixel_color PixelColor;

	for(int Y = 0; Y < Buffer->Height; ++Y)
	{
		uint32 *Pixel = (uint32*)Row;
		for(int X = 0; X < Buffer->Width; ++X)
		{
			//uint8 Blue = (uint8)(X+XOffset);
			//uint8 Green = (uint8)(X+YOffset);
			/*uint8 Blue = (uint8)0;
			uint8 Green = (uint8)0;
			uint8 Red = (uint8)0;*/
			/*
				Memory:    BB GG RR xx   (xx = pad)
				Register:  xx RR GG BB

				Pixel (32-bit)
			*/

			if(XCur < XThresh)
			{
				PixelColor.Blue = (uint8)0;
				PixelColor.Green = (uint8)0;
				PixelColor.Red = (uint8)0;
				*Pixel++ = ((PixelColor.Red << 16) | (PixelColor.Green << 8) | PixelColor.Blue);
			}
			else
			{
				PixelColor.Blue = (uint8)(X+XOffset);
				PixelColor.Green = (uint8)(Y+YOffset);
				*Pixel++ = ((PixelColor.Green << 8) | PixelColor.Blue);
			}

				/**Pixel++ = ((Green << 8) | Blue);
				XCur = XCur / 2;

				*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
				XCur = 32;*/
				XCur++;
		}
		XThresh = XThresh == 256 ? 1 : XThresh*2;
		XCur = XCur > XThresh ? 1 : XCur;
		Row += Buffer->Pitch;
	}
}

internal void
GameUpdateAndRendor(game_offscreen_buffer, int BlueOffset, int GreenOffset)
{
	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);
}