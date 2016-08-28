#define global static
#define internal static
#define local_persist static

#define PI32 3.14159265359f

#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include <math.h>

#include "game.h"
#include "game.cpp"

#include <windows.h>
#include <dsound.h>
#include <xinput.h>

#include "win32_game.h"

#define XINPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef XINPUT_GET_STATE(xinput_get_state);
XINPUT_GET_STATE(XInputGetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global xinput_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define XINPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef XINPUT_SET_STATE(xinput_set_state);
XINPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
global xinput_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_ 

internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *Filename)
{
	debug_read_file_result Result = {};

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			if(Result.Contents)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
					(FileSize32 == BytesRead))
				{
					//File read successfully
					Result.ContentsSize = FileSize32;
				}
				else
				{
					DEBUGPlatformFreeFileMemory(Result.Contents);
					Result.Contents = 0;
				}
			}

		}

		CloseHandle(FileHandle);
	}

	return Result;
}

internal void
DEBUGPlatformFreeFileMemory(void *Memory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

internal bool32
DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory)
{
	bool32 Result = false;

	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		DWORD BytesWritten;
		if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
		{
			//File written to successfully
			Result = (BytesWritten == MemorySize);
		}
		CloseHandle(FileHandle);
	}

	return Result;
}

internal void
Win32LoadXInput()
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");

	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	} 

	if(XInputLibrary)
	{
		XInputGetState = (xinput_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if(!XInputGetState)
		{
			XInputGetState = XInputGetStateStub;
		}

		XInputSetState = (xinput_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if(!XInputSetState)
		{
			XInputSetState = XInputSetStateStub;
		}
	}
}


#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

global bool GlobalRunning;
global win32_offscreen_buffer GlobalBuffer;
global LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global bool GlobalSoundIsPlaying;

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);

	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}
internal void 
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	if (Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Width;
	Buffer->Info.bmiHeader.biHeight = -Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	Buffer->Width = Width;
	Buffer->Height = Height;
	int BytesPerPixel = 4;
	Buffer->Pitch = Width * BytesPerPixel;
	int BufferSize = (Width*Height) * BytesPerPixel;

	Buffer->Memory = VirtualAlloc(0, BufferSize, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32InitDSound(HWND Window, uint32 SamplesPerSecond, uint32 BufferSize)
{
	//Load the Library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary)
	{
		//Create DirectSound object
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};

			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				//Create Primary buffer
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						//Primary Buffer's format set!
					}
				}
			}

			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			//Create Secondary buffer
			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
			{
				//Secondary Buffer created!
			}
		}
	}
}

internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer Buffer, win32_window_dimension Dimension, HDC DeviceContext)
{
	StretchDIBits(
		DeviceContext,
		0, 0, Dimension.Width, Dimension.Height,
		0, 0, Buffer.Width, Buffer.Height,
		Buffer.Memory,
		&Buffer.Info,
		DIB_RGB_COLORS,
		SRCCOPY
		);
}

internal void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
		0, SoundOutput->SecondaryBufferSize,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		DWORD Region1SampleCount = Region1Size;
		uint8 *DestSample = (uint8 *)Region1;
		for (uint32 SampleIndex = 0;
			SampleIndex < Region1SampleCount;
			SampleIndex++)
		{
			*DestSample++ = 0;
		}

		DWORD Region2SampleCount = Region2Size;
		DestSample = (uint8 *)Region2;
		for (uint32 SampleIndex = 0;
			SampleIndex < Region2SampleCount;
			SampleIndex++)
		{
			*DestSample++ = 0;
		}
	}

	GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, game_sound_output_buffer *SourceBuffer, DWORD ByteToLock, DWORD BytesToWrite)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
		ByteToLock, BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0)))
	{
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16 *DestSample = (int16 *)Region1;
		int16 *SourceSample = (int16 *)SourceBuffer->Samples;
		for (uint32 SampleIndex = 0;
			SampleIndex < Region1SampleCount;
			SampleIndex++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			SoundOutput->RunningSampleIndex++;
		}

		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (int16 *)Region2;
		for (uint32 SampleIndex = 0;
			SampleIndex < Region2SampleCount;
			SampleIndex++)
		{
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			SoundOutput->RunningSampleIndex++;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}



internal LRESULT CALLBACK 
WindowProc(
	HWND   Window,
	UINT   Message,
	WPARAM WParam,
	LPARAM LParam
	)
{
	LRESULT Result = 0;
	
	switch (Message) 
	{
		case WM_DESTROY:
		{
			//could be an error? maybe try recreating the window?
			GlobalRunning = false;
		} break;

		case WM_CLOSE:
		{
			//a user has tried to close the window, could post "are you sure" message to user
			GlobalRunning = false;
		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);

			Win32DisplayBufferInWindow(GlobalBuffer, Dimension, DeviceContext);
			
			EndPaint(Window, &Paint);
		} break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			Assert(!"Keyboard input came in through a non-dispatch message!")
		}

		default:
		{
			Result = DefWindowProcA(Window, Message, WParam, LParam);
		} break;
	}

	return(Result);
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
								game_button_state *OldState, DWORD ButtonBit,
								game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->TransitionCount = (OldState->EndedDown != NewState->EndedDown ? 1 :0);
}

internal void
Win32ProcessKeyboardDigitalButton(game_button_state *NewState, bool32 IsDown)
{
	NewState->EndedDown = IsDown;
	++NewState->TransitionCount;
}
internal void
Win32ProcessPendingMessages(HWND Window, game_keyboard_input *Keyboard)
{
	MSG Message;
	while(PeekMessageA(&Message, Window, 0, 0, PM_REMOVE))
	{
		if (Message.message == WM_QUIT)
		{
			GlobalRunning = false;
		}

		switch(Message.message)
		{
			case WM_QUIT:
			{
				GlobalRunning = false;
			} break;

			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_KEYDOWN:
			case WM_KEYUP:
			{
				uint32 VKCode = (uint32)Message.wParam;
				bool32 WasDown = ((Message.lParam & 1 << 30) != 0);
				bool32 IsDown = ((Message.lParam & 1 << 31) == 0);
		
				if(WasDown != IsDown)
				{
					if(VKCode == 'W')
					{
						Win32ProcessKeyboardDigitalButton(&Keyboard->Up, IsDown);
					}
					else if(VKCode == 'A')
					{
						Win32ProcessKeyboardDigitalButton(&Keyboard->Left, IsDown);
					}
					else if(VKCode == 'S')
					{
						Win32ProcessKeyboardDigitalButton(&Keyboard->Down, IsDown);
					}
					else if(VKCode == 'D')
					{
						Win32ProcessKeyboardDigitalButton(&Keyboard->Right, IsDown);
					}	
				}
			} break;

			default:
			{
				TranslateMessage(&Message);
				DispatchMessageA(&Message);
			}
		}
	}
}

int CALLBACK 
WinMain(HINSTANCE Instance,
		HINSTANCE PrevInstance,
		LPSTR     lpCmdLine,
		int       nCmdShow) 
{
	Win32LoadXInput();

	WNDCLASSA WindowClass = {};
	
	Win32ResizeDIBSection(&GlobalBuffer, 1280, 720);
	
	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = WindowProc;
	WindowClass.hInstance = Instance;
	//WindowClass.hIcon = ;
	WindowClass.lpszClassName = "My Window Class";
 
	if (RegisterClassA(&WindowClass)) {

		HWND Window = CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Game",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0);

		if (Window) 
		{
			//Since I specified CS_OWNDC I can just get one device context and use it forever
			//because I'm not sharing it with anyone
			//no release required
			HDC DeviceContext = GetDC(Window);

			
			//Sound test
			win32_sound_output SoundOutput = {};
			
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearSoundBuffer(&SoundOutput);

			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);


			GlobalRunning = true;

			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, 
										  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			//Sould I do the thing like in handmade hero where he uses a different base address for VirtualAlloc
			//depending of whether GAME_INTERNAL is 1?
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.PermanentStorage = VirtualAlloc(0, GameMemory.PermanentStorageSize, 
										  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			GameMemory.TransientStorageSize = Gigabytes(4);
			GameMemory.TransientStorage = VirtualAlloc(0, GameMemory.TransientStorageSize, 
										  MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

			if(Samples && GameMemory.PermanentStorage)
			{

				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				while(GlobalRunning) 
				{
					Win32ProcessPendingMessages(Window, &NewInput->Keyboard);

					int MaxControllerCount = XUSER_MAX_COUNT;
					if(MaxControllerCount > ArrayCount(NewInput->Controllers))
					{
						MaxControllerCount = ArrayCount(NewInput->Controllers);
					}	

					for(DWORD ControllerIndex = 0;
						ControllerIndex < XUSER_MAX_COUNT;
						ControllerIndex++)
					{
						game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];

						XINPUT_STATE ControllerState;
						if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
						{
							//This controller is plugged in
							XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

							bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
							bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
							bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
							bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

							NewController->IsAnalogue = true;
							NewController->StartX = OldController->EndX;
							NewController->StartY = OldController->EndY;

							real32 X = 0;

							if(Pad->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
							{
								//normalise the stick input 
								X = (real32)((Pad->sThumbLX + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32768.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));  	
							}
							else if (Pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
							{
								X = (real32)((Pad->sThumbLX - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32767.0f  - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
							}
							NewController->MinX = NewController->MaxX = NewController->EndX = X;

							real32 Y = 0;
							
							if(Pad->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
							{
								//normalise the stick input 
								Y = (real32)((Pad->sThumbLY + XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32768.0f - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));  	
							}
							else if (Pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
							{
								Y = (real32)((Pad->sThumbLY - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE) / (32767.0f  - XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE));
							}
							NewController->MinY = NewController->MaxY = NewController->EndY = Y;

							//TODO: implement Deadzone	
							int16 LeftStickX = Pad->sThumbLX;
							int16 LeftStickY = Pad->sThumbLY;
							int16 RightStickX = Pad->sThumbRX;
							int16 RightStickY = Pad->sThumbRY;
							
							//Not sure if I like this, maybe to start with only use analoguye stick in controller for movement and only 
							//WASD on keyboard
							//Eliminate ABXY on controller for now??
#if 0
							Win32ProcessXInputDigitalButton(Pad->wButtons, 
															&OldController->A, XINPUT_GAMEPAD_A,
															&NewController->A);
							Win32ProcessXInputDigitalButton(Pad->wButtons, 
															&OldController->B, XINPUT_GAMEPAD_B,
															&NewController->B);
							Win32ProcessXInputDigitalButton(Pad->wButtons, 
															&OldController->X, XINPUT_GAMEPAD_X,
															&NewController->X);
							Win32ProcessXInputDigitalButton(Pad->wButtons, 
															&OldController->Y, XINPUT_GAMEPAD_Y,
															&NewController->Y);
#endif															
							Win32ProcessXInputDigitalButton(Pad->wButtons, 
															&OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
															&NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton(Pad->wButtons, 
															&OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
															&NewController->RightShoulder);

							//bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
							//bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
					
							//TODO: implement Deadzone
							//uint8 LeftTrigger = Pad->bLeftTrigger;
							//uint8 RightTrigger = Pad->bRightTrigger;
						}
					}


					DWORD ByteToLock = 0;
					DWORD TargetCursor = 0;
					DWORD BytesToWrite = 0;  
					DWORD PlayCursor = 0;
					DWORD WriteCursor = 0;
					bool32 SoundIsValid = false;

					if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
					{

						ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % 
										SoundOutput.SecondaryBufferSize;

						TargetCursor = (PlayCursor + 
									    (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample))
									      % SoundOutput.SecondaryBufferSize;
						
						if (ByteToLock > TargetCursor)
						{
							BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
							BytesToWrite += TargetCursor;
						}	
						else
						{
							BytesToWrite = TargetCursor - ByteToLock; 
						}

						SoundIsValid = true;
					}

					game_sound_output_buffer SoundBuffer = {};
					SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
					SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
					SoundBuffer.Samples = Samples;
					
					//Graphics Code
					game_offscreen_buffer Buffer = {};
					Buffer.Memory = GlobalBuffer.Memory;
					Buffer.Width = GlobalBuffer.Width;
					Buffer.Height = GlobalBuffer.Height;
					Buffer.Pitch = GlobalBuffer.Pitch;
					
					GameUpdateAndRender(&GameMemory ,NewInput, &Buffer, &SoundBuffer);

					if(SoundIsValid)
					{
						Win32FillSoundBuffer(&SoundOutput, &SoundBuffer, ByteToLock, BytesToWrite);
					}
					
					win32_window_dimension Dimension = Win32GetWindowDimension(Window);
					Win32DisplayBufferInWindow(GlobalBuffer, Dimension, DeviceContext);

					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;

					//clear NewInput for use next frame
					*NewInput = {};
				}
			}

		}
	}
	return 0;
}