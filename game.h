//Services that the platform layer provides to the game

//Services that the game provides to the platform layer
struct game_offscreen_buffer
{
	void* Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer
{
	int16 *Samples;
	int SamplesPerSecond;
	int SampleCount;
};
internal void 
GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer, int RedOffset, int GreenOffset);