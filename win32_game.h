struct win32_window_dimension
{
	int Width;
	int Height;
};

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct win32_sound_output
{
	int SamplesPerSecond;
	uint32 RunningSampleIndex;
	int BytesPerSample;
	int LatencySampleCount;
	int SecondaryBufferSize;
};