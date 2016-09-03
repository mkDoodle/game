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
			//Layout in memory is 0PlayerX PlayerXPlayerX RR GG BB
			*Pixel++ = (R << 16) | (G << 8)| (B);
		}

		Row += Buffer->Pitch;
	}
}


//TODO FiPlayerX issue where sound suddenly outputs at a higher frequency
internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
	/*local_persist real32 tSine = 0;
	int ToneVolume = 3000;
	//Having this as an integer will cause inaccuracies but fuck it this is test code
	int WavePeriodSamples = SoundBuffer->SamplesPerSecond / ToneHz;

	int16 *SampleOut = SoundBuffer->Samples;

	for (int SampleIndePlayerX = 0;
		SampleIndePlayerX < SoundBuffer->SampleCount;
		++SampleIndePlayerX)
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
	}*/
}


/*internal void
RenderAnimatedGradient(game_offscreen_buffer *Buffer, int RedOffset, int GreenOffset)
{
	uint8 *Row = (uint8 *)Buffer->Memory;

	for (int Y = 0; Y < Buffer->Height; Y++)
	{
		uint32 *PiPlayerXel = (uint32 *)Row;
		for (int PlayerX = 0; PlayerX < Buffer->Width; PlayerX++)
		{
			uint8 Green = (uint8)(PlayerX + RedOffset);
			uint8 Red = (uint8)(Y + GreenOffset);
			 
			//Layout in memory is 0PlayerX PlayerXPlayerX RR GG BB
			*PiPlayerXel++ = (Red << 16) | (Green << 8);
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
#if 0
		char *Filename = __FILE__;

		debug_read_file_result File = DEBUGPlatformReadEntireFile(Filename);
		if(File.Contents)
		{
			DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
			DEBUGPlatformFreeFileMemory(File.Contents);
		}
#endif
		GameState->ToneHz = 256;

		//this might be more appropriate to do in the platform layer
		Memory->IsInitialised = true;
	}

	//some player variables
	local_persist real32 PlayerX = 10;
	local_persist real32 PlayerY = 10;
	int PlayerWidth = 20;
	int PlayerHeight = 20;

	int PlayerSpeed = 5;

	local_persist real32 PlayerVelocityX = 0;
	local_persist real32 PlayerVelocityY = 0;

	local_persist bool32 Jumping = false;

	//temp hardcoded latch between keyboarda and controller
	Input->IsController = true;

	if(Input->IsController)
	{
		game_controller_input ControllerInput = Input->Controller;

		if(ControllerInput.IsAnalogue)
		{
			//Use analogue movement tuning

			PlayerVelocityX += (ControllerInput.StickX * PlayerSpeed);
			PlayerVelocityY += (ControllerInput.StickY * PlayerSpeed);
#if 0
			if(ControllerInput.StickX > 0.5f)
			{
				dX = 1;
			}
			else if (ControllerInput.StickX > -0.5f && ControllerInput.StickX < 0.5f)
			{
				dX = 0;
			}
			else if (ControllerInput.StickX < -0.5f)
			{
				dX = -1;
			}

			if(ControllerInput.StickY > 0.5f)
			{
				dY = 1;
			}
			else if (ControllerInput.StickY > -0.5f && ControllerInput.StickY < 0.5f)
			{
				dY = 0;
			}
			else if (ControllerInput.StickY < -0.5f)
			{
				dY = -1;
			}
#endif
			
			

			//GameState->RedOffset +=  (int)(4.0f * (Input0->StickX));
			//GameState->ToneHz = 256 + (int)(128.0f * (Input0->StickY));  
		}
		else
		{
			//use digital movement tuning
		}

		if(ControllerInput.A.EndedDown)
		{
			if(!Jumping)
			{
				Jumping = true;
				PlayerVelocityY += 100;	
			}
		}

		if(ControllerInput.B.EndedDown)
		{
			//GameState->GreenOffset += 1;
			GlobalRunning = false;
		}
	}
	else
	{
		game_keyboard_input *Keyboard = &Input->Keyboard;

		if(Keyboard->Up.EndedDown)
		{
			PlayerY -= 10;
		}
		else if(Keyboard->Down.EndedDown)
		{
			PlayerY += 10;
		}
		else if(Keyboard->Left.EndedDown)
		{
			PlayerX -= 10;
		}
		else if(Keyboard->Right.EndedDown)
		{
			PlayerX += 10;
		}

	}

	//player physics update
	real32 Timestep = (1.0f / 60.0f);

	real32 GravityAcceleration = 5.0f;

	real32 FrictionX = (0.5f * PlayerVelocityX);



	PlayerVelocityY -= GravityAcceleration;

	PlayerX += PlayerVelocityX;

	PlayerY -=  PlayerVelocityY;

	if(Jumping && PlayerY + PlayerHeight >= Buffer->Height)
	{
		Jumping = false;
	}

	

	//border collisions
	if(PlayerX < 0)
	{
		PlayerX = 0;
	}

	if((PlayerX + PlayerWidth) > Buffer->Width)
	{
		PlayerX = (real32)(Buffer->Width - PlayerWidth);
	}

	if(PlayerY < 0)
	{
		PlayerY = 0;
	}

	if((PlayerY + PlayerHeight) > Buffer->Height)
	{
		PlayerY = (real32)(Buffer->Height - PlayerHeight);
	}



	//RenderAnimatedGradient(Buffer, GameState->RedOffset, GameState->GreenOffset);
	
	
	DrawRentangle(Buffer, 0, 0, Buffer->Width, Buffer->Height, 0, 0, 0);
	//this now will cause a bit of shit since players pos is in real32 and actual image is an 
	//int approximation
	DrawRentangle(Buffer, (int)PlayerX, (int)PlayerY, (int)(PlayerX + PlayerWidth), (int)(PlayerY + PlayerHeight), 255, 0, 0);
	GameOutputSound(SoundBuffer, GameState->ToneHz);
}