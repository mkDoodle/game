internal int
RoundReal32ToInt(real32 Value)
{
	int Result;

	if((Value - (int)Value) >= 0.5)
	{
		Result = (int)Value + 1;
	}
	else 
	{
		Result = (int)Value;
	}
	return Result;
}

internal bool32
CheckAABBCollision(real32 X1, real32 Y1, real32 Width1, real32 Height1,
				   real32 X2, real32 Y2, real32 Width2, real32 Height2)
{
	bool32 Collided;

	//Very basic AABB Collision
	if((Y1 + Height1) < Y2)
	{
		Collided = false;
	}
	else if(Y1 > (Y2 + Height2))
	{
		Collided = false;
	}
	else if (X1 > (X2 + Width2)) 
	{
		Collided = false;
	}
	else if ((X1 + Width1) < X2)
	{
		Collided = false;
	}
	else 
	{
		Collided = true;
	}

	return Collided;
}


internal void
DrawRectangle(game_offscreen_buffer *Buffer, 
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
		player *Player = &GameState->Player;

		Player->Centre.X = 100.0f;
		Player->Centre.Y = 100.0f;
		
		//TODO: consider removing these
		Player->X = 500.0f;
		Player->Y = 300.0f;
		
		Player->Width = 20.0f;
		Player->Height = 20.0f;
		Player->HalfWidth = 10.0f;
		Player->HalfHeight = 10.0f;


		Player->NextCentre.X = 0.0f;
		Player->NextCentre.Y = 0.0f;

		/*Player->NextX = 0;
		Player->NextY = 0;*/

		Player->Velocity.X = 0.0f;
		Player->Velocity.Y = 0.0f;

		Player->XAcceleration = 10.0f;
		Player->YAcceleration = 10.0f;

		//this might be more appropriate to do in the platform layer
		Memory->IsInitialised = true;
	}

	player *Player = &GameState->Player;

	//temp hardcoded latch between keyboarda and controller
	Input->IsController = true;

	if(Input->IsController)
	{
		game_controller_input ControllerInput = Input->Controller;
		
		//Reset Player's Velocity
		Player->Velocity = {};

		//Stick
		if(ControllerInput.IsAnalogue)
		{
			//Use analogue movement tuning
			Player->Velocity.X = (ControllerInput.StickX * Player->XAcceleration);
			Player->Velocity.Y = (ControllerInput.StickY * Player->YAcceleration);
		}
		else
		{
			//use digital movement tuning		
		}

		//Buttons
		if(ControllerInput.B.IsDown)
		{
			GlobalRunning = false;
		}
	}

	if(!Input->IsController)
	{
		game_keyboard_input KeyboardInput = Input->Keyboard;

		//Reset Player's Velocity
		Player->Velocity = {};

		//TODO: get this to work with holding keys down
		if(KeyboardInput.Up.IsDown)
		{
			Player->Velocity.Y = Player->YAcceleration; 
		}
		if(KeyboardInput.Down.IsDown)
		{
			Player->Velocity.Y = -Player->YAcceleration;
		}
		if(KeyboardInput.Left.IsDown)
		{
			Player->Velocity.X = -Player->XAcceleration;
		}
		if(KeyboardInput.Right.IsDown)
		{
			Player->Velocity.X = Player->XAcceleration;
		}
	}
	


	Player->NextCentre.X = Player->Centre.X + Player->Velocity.X;
	Player->NextCentre.Y = Player->Centre.Y + Player->Velocity.Y;

	/*Player->NextX = Player->X + Player->Velocity.X;
	Player->NextY = Player->Y - Player->Velocity.Y;*/
	

	real32 ObstacleX = 1000.0f;
	real32 ObstacleY = 200.0f;
	real32 ObstacleWidth = 100.0f;
	real32 ObstacleHeight = 400.0f;

#if 0
	real32 ObstacleX = 300.0f;
	real32 ObstacleY = 150.0f;
	real32 ObstacleWidth = 50.0f;
	real32 ObstacleHeight = 450.0f;


	bool32 Collided = CheckAABBCollision(Player->NextX, Player->NextY, Player->Width, Player->Height,
										 ObstacleX, ObstacleY, ObstacleWidth, ObstacleHeight);






	if(Collided)
	{
		OutputDebugStringA("Collided\n");

		Player->NextX = Player->X;
		Player->NextY = Player->Y;

		Collided = false;

		while(!Collided)
		{
			local_persist bool32 FirstTime = true;
			if(FirstTime)
			{	
				Player->NextX = Player->X;
				Player->NextY = Player->Y;
				FirstTime = false;
			}

			real32 PossibleNextX = Player->NextX;
			real32 PossibleNextY = Player->NextY;

			if(Player->XVelocity > 0)
			{
				PossibleNextX += 0.1f;
			}
			else if(Player->XVelocity < 0)
			{
				PossibleNextX -= 0.1f;
			}

			if(Player->YVelocity > 0)
			{
				PossibleNextY -= 0.1f;
			}
			else if(Player->YVelocity < 0)
			{
				PossibleNextY += 0.1f;
			}

			Collided = CheckAABBCollision(Player->NextX, Player->NextY, Player->Width, Player->Height,
							   ObstacleX, ObstacleY, ObstacleWidth, ObstacleHeight);

			if(!Collided)
			{
				Player->NextX = PossibleNextX;
				Player->NextY = PossibleNextY;
			}
		}

		/*for(Collided; 
			!Collided;
			Collided = CheckAABBCollision(Player->NextX, Player->NextY, Player->Width, Player->Height,
							  			  ObstacleX, ObstacleY, ObstacleWidth, ObstacleHeight))
		{
			if(Player->XVelocity > 0)
			{
				Player->NextX += 0.1f;
			}
			else if(Player->XVelocity < 0)
			{
				Player->NextX -= 0.1f;
			}
		}*/
		//move 1
		//check collision
		//if not collided move 0.1(?) again
		//else if collided again do not move
	}
#endif

#if 0
	//wall stick stuff
	if(Player->X <= 0)
	{
		OnLeftWall = true;
	}
	else
	{
		OnLeftWall = false;
	}

	if((Player->X + Player->Width) >= Buffer->Width)
	{
		OnRightWall = true;
	}
	else 
	{
		OnRightWall = false;
	}
#endif



	Player->Centre.X = Player->NextCentre.X;
	Player->Centre.Y = Player->NextCentre.Y;
	
	//TODO: Change these to dealing with player centre
	//border collisions
	//Left Wall
	if(Player->X < 0)
	{
		Player->X = 0;
	}

	//Right Wall
	if((Player->X + Player->Width) > Buffer->Width)
	{
		Player->X = (real32)(Buffer->Width - Player->Width);
	}

	//Top
	if(Player->Y < 0)
	{
		Player->Y = 0;
	}

	//Bottom
	if((Player->Y + Player->Height) > Buffer->Height)
	{
		Player->Y = (real32)(Buffer->Height - Player->Height);
		
		Player->Velocity.Y = 0;
	}


	//Player->X = NextPositionX;
	//Player->Y = NextPositionY;
	
	
	//Fill Backgorund Black
	DrawRectangle(Buffer, 0, 0, Buffer->Width, Buffer->Height, 0, 0, 0);
	//TODO: Fix square changing size due to this rounding
	//Draw Player
	DrawRectangle(Buffer, RoundReal32ToInt(Player->Centre.X), RoundReal32ToInt(Player->Y), 
				  RoundReal32ToInt((Player->X + Player->Width)), RoundReal32ToInt((Player->Y + Player->Height)), 
				  255, 0, 0);

	//Draw Obstacles

	DrawRectangle(Buffer, RoundReal32ToInt(ObstacleX), RoundReal32ToInt(ObstacleY), 
				  RoundReal32ToInt(ObstacleX + ObstacleWidth), RoundReal32ToInt(ObstacleY + ObstacleHeight),
				  0, 0, 255);
	//DrawRectangle(Buffer, 800, 150, 850, 600, 0, 0, 255);

	/*DrawRentangle(Buffer, RoundReal32ToInt(ObstacleX), RoundReal32ToInt(ObstacleY), 
				  RoundReal32ToInt(ObstacleX + ObstacleWidth), RoundReal32ToInt(ObstacleY + ObstacleHeight),
				  0, 0, 255);*/
	//DrawRentangle(Buffer, 800, 150, 850, 600, 0, 0, 255);


	//currently does nothing
	GameOutputSound(SoundBuffer, 256);
}