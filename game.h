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

struct game_button_state
{
	//One transition between up/down on a digital button
	int TransitionCount;
	bool32 EndedDown;
};
struct game_controller_input 
{
	bool32 IsAnalogue;

	//Left Stick
	real32 StartX;
	real32 StartY;

	real32 MinX;
	real32 MinY;

	real32 MaxX;
	real32 MaxY;

	real32 EndX;
	real32 EndY;

	union
	{
		game_button_state Buttons[6];
		struct
		{
			game_button_state A;
			game_button_state B;
			game_button_state X;
			game_button_state Y;
			game_button_state LeftShoulder;
			game_button_state RightShoulder;
		};
	};

};

struct game_input
{
	game_controller_input Controllers[4];
};

internal void 
GameUpdateAndRender(game_input *Input, game_offscreen_buffer *Buffer, 
					game_sound_output_buffer *SoundBuffer);