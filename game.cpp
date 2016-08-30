internal void
DrawRentangle(game_offscreen_buffer *Buffer, 
			  int MinX, int MinY, int MaxX, int MaxY,
			  int R, int G, int B)
{
	
	
	uint8 *Row = (uint8 *)Buffer->Memory + (MinX * Buffer->BytesPerPixel)  + (MinY * Buffer->Pitch);

	//TODO: test if this will go up en entirity of the square of if condition should be <=
	for (int Y = MinY; 
		 Y < MaxY;
		 Y++)
	{
		uint32 *Pixel = (uint32 *)Row;
		
		for (int X = MinX;
			 X < MaxX;
			 X++)
		{	 
			//Layout in memory is 0x xx RR GG BB
			*Pixel++ = (R << 16) | (G << 8)| (B);
		}

		Row += Buffer->Pitch;
	}
}


//TODO Fix issue where sound suddenly outputs at a higher frequency
internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	local_persist real32 tSine = 0;
	int ToneVolume = 3000;
	//Having this as an integer will cause inaccuracies but fuck it this is test code
	int WavePeriodSamples = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 *SampleOut = SoundBuffer->Samples;

	for (int SampleIndex = 0;
		SampleIndex < SoundBuffer->SampleCount;
		++SampleIndex)
	{
		real32 SineValue = sinf(tSine);

		int16 SampleValue = (int16)(SineValue * ToneVolume);

		*SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;

		//Writing in 1 Sample each pass, so I've used 1/Wave Period Samples
		// this gives me the amount of the way through the wave that I went through with the sample
		//this value is between 0 and 1
		// I then multiply by 2pi as this gives me the radian representation of that movement as sinf take radians
		tSine += 2.0f * PI32 * 1.0f / (real32)WavePeriodSamples;
	}
}


/*internal void
RenderAnimatedGradient(game_offscreen_buffer *Buffer, int RedOffset, int GreenOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;

	for (int Y = 0; Y < Buffer->Height; Y++)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->Width; X++)
		{
			uint8 Green = (uint8)(X + RedOffset);
			uint8 Red = (uint8)(Y + GreenOffset);
			 
			//Layout in memory is 0x xx RR GG BB
			*Pixel++ = (Red << 16) | (Green << 8);
		}

		Row += Buffer->Pitch;
	}
}*/

internal void
GameUpdateAndRender(game_memory *Memory, game_input *Input, game_offscreen_buffer *Buffer, 
					game_sound_output_buffer *SoundBuffer)
{
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

	game_state *GameState = (game_state *)Memory->PermanentStorage;
	if(!Memory->IsInitialised)
	{
		char *Filename = __FILE__;

		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
		if(File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}

		GameState->ToneHz = 256;

		//this might be more appropriate to do in the platform layer
		Memory->IsInitialised = true;
	}

	//some player variables
	local_persist int MinX = 10;
	local_persist int MinY = 10;
	local_persist int MaxX = 110;
	local_persist int MaxY = 110;

	//temp hardcoded latch between keyboarda and controller
	Input->IsController = false;

	if(Input->IsController)
	{
		game_controller_input *Input0 = &Input->Controllers[0];

		if(Input0->IsAnalogue)
		{
			//Use analogue movement tuning
			GameState->RedOffset +=  (int)(4.0f * (Input0->EndX));
			GameState->ToneHz = 256 + (int)(128.0f * (Input0->EndY));  
		}
		else
		{
			//use digital movement tuning
		}

		if(Input0->A.EndedDown)
		{
			GameState->GreenOffset += 1;
		}
	}
	else
	{
		game_keyboard_input *Keyboard = &Input->Keyboard;

		if(Keyboard->Up.EndedDown)
		{
			MinY -= 10;
			MaxY -= 10;
		}
		else if(Keyboard->Down.EndedDown)
		{
			MinY += 10;
			MaxY += 10;
		}
		else if(Keyboard->Left.EndedDown)
		{
			MinX -= 10;
			MaxX -= 10;
		}
		else if(Keyboard->Right.EndedDown)
		{
			MinX += 10;
			MaxX += 10;
		}


	}
	

	//RenderAnimatedGradient(Buffer, GameState->RedOffset, GameState->GreenOffset);
	
	
	DrawRentangle(Buffer, 0, 0, Buffer->Width, Buffer->Height, 0, 0, 0);
	DrawRentangle(Buffer, MinX, MinY, MaxX, MaxY, 255, 0, 0);
	GameOutputSound(SoundBuffer, GameState->ToneHz);
}