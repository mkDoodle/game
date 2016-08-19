internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer)
{
	local_persist real32 tSine;
	int ToneVolume = 3000;
	int ToneHz = 261;
	//Having this as an integer will cause inaccuracies but fuck it this is test code
	int WavePeriodSamples = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 *SampleOut = SoundBuffer->Samples;

	for (uint32 SampleIndex = 0;
		SampleIndex < SoundBuffer->SampleCount;
		SampleIndex++)
	{
		real32 SineValue = sinf(tSine);

		int16 SampleValue = (int16)(SineValue * ToneVolume);

		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		//Writing in 1 Sample each pass, so I've used 1/Wave Period Samples
		// this gives me the amount of the way through the wave that I went through with the sample
		//this value is between 0 and 1
		// I then multiply by 2pi as this gives me the radian representation of that movement as sinf take radians
		tSine += 2.0f * PI32 * (1.0f / WavePeriodSamples);
	}
}

internal void
RenderAnimatedGradient(game_offscreen_buffer *Buffer, int RedOffset, int GreenOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;

	for (int Y = 0; Y < Buffer->Height; Y++)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; X++)
		{
			uint8 Green = X + RedOffset;
			uint8 Red = Y + GreenOffset;
			 
			//Layout in memory is 0x xx RR GG BB
			*Pixel++ = (Red << 16) | (Green << 8);
		}

		Row += Buffer->Pitch;
	}
}

internal void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int RedOffset, int GreenOffset)
{
	RenderAnimatedGradient(Buffer, RedOffset, GreenOffset);
}