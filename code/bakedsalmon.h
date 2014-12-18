#if !defined(BAKEDSALMON_H)

/**
	Services that the platform layer provides to game
 **/

/**
	Services that the game provides to platform layer
 **/

// Parameters: timing, controller input, bitmap buffer, sound buffer
struct game_offscreen_buffer
{
	// Pixels are always 32-bits long. In memory: BB GG RR xx
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer);

#define BAKEDSALMON_H
#endif