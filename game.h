/*
	Note:

	GAME_INTERNAL :
	0 - Build for public release
	1 - Build for developer only

	GAME_SLOW :
	0 - No slow code allowed!
	1 - Slow code welcome						
*/

#if GAME_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif




#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) ((Kilobytes(Value)) * 1024LL)
#define Gigabytes(Value) ((Megabytes(Value)) * 1024LL)
#define Terabytes(Value) ((Gigabytes(Value)) * 1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32
SafeTruncateUInt64(uint64 Value)
{
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return Result;
}

//Services that the platform layer provides to the game
#if GAME_INTERNAL
/*	IMPORTANT

	These are NOT for doing anything in the shipping game! They are blocking and do not protect against lost data
*/
struct debug_read_file_result
{
	uint32 ContentsSize;
	void *Contents;
};

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
internal void DEBUGPlatformFreeFileMemory(void *Memory);
internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory);
#endif

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


struct game_memory
{
	bool32 IsInitialised;

	uint64 PermanentStorageSize;
	void *PermanentStorage; //REQUIRED to be cleared to 0

	uint64 TransientStorageSize;
	void *TransientStorage;
};

internal void 
GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, 
					game_sound_output_buffer *SoundBuffer);




struct game_state
{
	int RedOffset;
	int GreenOffset;
	int ToneHz;
};