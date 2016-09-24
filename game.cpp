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
		player *Player = &GameState->Player;

		Player->X = 10.0f;
		Player->Y = 10.0f;
		Player->Width = 20.0f;
		Player->Height = 20.0f;

		Player->NextX = 0;
		Player->NextY = 0;

		Player->XVelocity = 0.0f;
		Player->YVelocity = 0.0f;

		Player->MaxXVelocity = 35.0f;
		Player->MaxYVelocity = 30.0f;

		Player->XAcceleration = 1.0f;

		//this might be more appropriate to do in the platform layer
		Memory->IsInitialised = true;
	}

	//
	player *Player = &GameState->Player;

	//int PlayerSpeed = 2;

	local_persist bool32 AButtonPressedLastFrame = false;

	local_persist bool32 Jumping = false;

	local_persist bool32 OnLeftWall = false;
	local_persist bool32 OnRightWall = false;

	real32 WallStickFallSpeed = -2; 

	//temp hardcoded latch between keyboarda and controller
	Input->IsController = true;

	if(Input->IsController)
	{
		game_controller_input ControllerInput = Input->Controller;

		//Stick
		if(ControllerInput.IsAnalogue)
		{
			//Use analogue movement tuning
			Player->XVelocity += (ControllerInput.StickX * Player->XAcceleration);
		}
		else
		{
			//use digital movement tuning
		}

		//Buttons
		if(ControllerInput.A.Down)
		{
			if(!Jumping && !AButtonPressedLastFrame)
			{
				Jumping = true;
				Player->YVelocity += 30;
			}

			if(OnLeftWall && !AButtonPressedLastFrame)
			{
				Player->XVelocity += 30;
				Player->YVelocity += 30;
			}

			if(OnRightWall && !AButtonPressedLastFrame)
			{
				Player->XVelocity -= 30;
				Player->YVelocity += 30;
			}

			AButtonPressedLastFrame = true;
		}
		else
		{
			AButtonPressedLastFrame = false;
		}

		if(ControllerInput.B.Down)
		{
			GlobalRunning = false;
		}
	}
	else
	{
	}

	//player physics update
	//real32 Timestep = (1.0f / 60.0f);


	//gravity and ground resistance
	real32 GravityAcceleration = 1.0f;

	real32 UpwardForce = 0.0f;
	real32 DownwardForce = 0.0f;

	//Deal with Y Velocity stuff
	DownwardForce = GravityAcceleration;

	if(Player->Y + Player->Height >= Buffer->Height)
	{
		UpwardForce = GravityAcceleration;
	}
	else if(OnLeftWall || OnRightWall)
	{
		UpwardForce = 0.5f * DownwardForce;
		Player->MaxYVelocity = 20.0f;
	}
	else
	{
		UpwardForce = 0.0f;
		Player->MaxYVelocity = 30.0f;
	}

	//(GravityAcceleration - UpwardForce)

	Player->YVelocity += (UpwardForce - DownwardForce);
	
	//enforce max velocity
	if(Player->YVelocity > 0)
	{	
		if(Player->YVelocity > Player->MaxYVelocity)
		{
			Player->YVelocity = Player->MaxYVelocity;
		}		
	}
	else if (Player->YVelocity < 0)
	{
		if(Player->YVelocity < -Player->MaxYVelocity)
		{
			Player->YVelocity = -Player->MaxYVelocity;
		}
	}

	real32 ResistanceX = ((Player->XVelocity / Player->MaxXVelocity) * Player->XAcceleration);

	//enforce max velocity
	if(Player->XVelocity > 0)
	{
		Player->XVelocity -= ResistanceX;

		if(Player->XVelocity > Player->MaxXVelocity)
		{
			Player->XVelocity = Player->MaxXVelocity;
		}
	}
	else if(Player->XVelocity < 0)
	{
		Player->XVelocity -= ResistanceX;

		if(Player->XVelocity < -Player->MaxXVelocity)
		{
			Player->XVelocity = -Player->MaxXVelocity;
		}
	}

	Player->NextX = Player->X + Player->XVelocity;
	Player->NextY = Player->Y - Player->YVelocity;
	
	real32 ObstacleX = 300.0f;
	real32 ObstacleY = 300.0f;
	real32 ObstacleWidth = 500.0f;
	real32 ObstacleHeight = 350.0f;

	bool32 Collided = CheckAABBCollision(Player->NextX, Player->NextY, Player->Width, Player->Height,
										 ObstacleX, ObstacleY, ObstacleWidth, ObstacleHeight);


	//push player as close to wall as possible
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

			PossibleNextX += 0.1f * Player->XVelocity;
			PossibleNextY -= 0.1f * Player->YVelocity;

			Collided = CheckAABBCollision(PossibleNextX, PossibleNextY, Player->Width, Player->Height,
							   			  ObstacleX, ObstacleY, ObstacleWidth, ObstacleHeight);

			if(!Collided)
			{
				Player->NextX = PossibleNextX;
				Player->NextY = PossibleNextY;
			}
		}
	}

	bool32 OnTopOfObstacle = false;

	if(Collided)
	{
		if((Player->NextX + Player->Height) < ObstacleY)
		{
			OnTopOfObstacle = true;

			Jumping = false;
		}

		if(Player->X > (ObstacleX + ObstacleWidth))
		{
			OnRightWall = true;		}
	}

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
	
	/*real32 NextPositionX = Player->X + Player->XVelocity;
	real32 NextPositionY = Player->Y + Player->YVelocity;

	if(NextPositionX < 0)
	{
		//Colided with Left edge of screen
		Player->XVelocity = 0; 

		//get as close as possible to the edge
		while(NextPositionX >= 0.1)
		{
			NextPositionX -= 0.1;
		}
	}

	if((NextPositionX + Player->Width) > Buffer->Width)
	{
		//Collided with Right edge of screen
		Player->XVelocity = 0;
	} 

	if(NextPositionY < 0)
	{
		Player->YVelocity = 0;
	}

	if((Player->Y + Player->Height) > Buffer->Height)
	{
		Player->YVelocity = 0;
	}*/ 
	//Player->X += Player->XVelocity;
	//Player->Y -= Player->YVelocity;

	Player->X = Player->NextX;
	Player->Y = Player->NextY;
	
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
		
		Player->YVelocity = 0;
	}


	if(Jumping && (Player->Y + Player->Height >= Buffer->Height))
	{
		Jumping = false;
	}

	//Player->X = NextPositionX;
	//Player->Y = NextPositionY;
	
	
	//Fill Backgorund Black
	DrawRentangle(Buffer, 0, 0, Buffer->Width, Buffer->Height, 0, 0, 0);
	//TODO: Fix square changing size due to this rounding
	//Draw Player
	DrawRentangle(Buffer, RoundReal32ToInt(Player->X), RoundReal32ToInt(Player->Y), 
				  RoundReal32ToInt((Player->X + Player->Width)), RoundReal32ToInt((Player->Y + Player->Height)), 
				  255, 0, 0);

	//Draw Obstacles
	DrawRentangle(Buffer, RoundReal32ToInt(ObstacleX), RoundReal32ToInt(ObstacleY), 
				  RoundReal32ToInt(ObstacleX + ObstacleWidth), RoundReal32ToInt(ObstacleY + ObstacleHeight),
				  0, 0, 255);
	//DrawRentangle(Buffer, 800, 150, 850, 600, 0, 0, 255);

	//currently does nothing
	GameOutputSound(SoundBuffer, 256);
}