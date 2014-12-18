/** TODO: This is far from a final platform

	- Save state directory
	- Handle to current executable
	- Asset directory
	- Threading
	- Raw input (multiple keyboards/pads)
	- Sleep
	- ClipCursor() (multiple monitors)
	- Fullscreen
	- WM_SETCURSOR (cursor visibility)
	- QueryCancelAutoplay
	- WM_ACTIVEAPP (alt-tabbed?)
	- Blit speed improvement
	- Hardware acceleration
	- GetKeyboardLayout (international)

	Very partial "roadmap"
*/


#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static
#define true TRUE
#define false FALSE
#define pi32 3.14159265359

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32_t bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

//typedef BOOL bool;

#include "bakedsalmon.cpp"

typedef struct s_win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
} win32_offscreen_buffer;

typedef struct s_win32_window_dimension
{
	int Width;
	int Height;
} win32_window_dimension;

typedef struct s_win32_pixel_color
{
	uint8 Blue;
	uint8 Green;
	uint8 Red;
} win32_pixel_color;

struct win32_sound_output
{
	// sound test
	int SamplesPerSecond;
	int Frequency;
	int16 ToneVolume;
	int WavePeriod;
	int BytesPerSample;
	uint32 RunningSampleIndex;
	int SecondBufferSize;
};

// TODO: Globals for now.
global_variable BOOL GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondBuffer;

/** support xinput 1.3 **/
// MOTE: XInputGetState
// define a stub to handle disconnected controllers instead of crashing
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
};
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// MOTE: XInputSetState
// define a stub to handle disconnected controllers instead of crashing
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_DEVICE_NOT_CONNECTED;
};
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

void *
PlatformLoadFile(char *FileName)
{
	// TODO: obviously implement this
	return 0;
}

internal void
Win32LoadXInput(void)
{
	// try loading xinput 1.4 for Windows 8 compatibility
	HMODULE XInputLibrary;
	if(!(XInputLibrary = LoadLibraryA("xinput1_4.dll")))
	{
		// TODO: Diagnostic 
		// if xinput 1.4 is not present, try xinput 1.3
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}

	if(XInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		if(!XInputGetState) { XInputGetState = XInputGetStateStub; }
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
		if(!XInputSetState) { XInputSetState = XInputSetStateStub; }

		// TODO: Diagnostic 
	}
	else
	{
		// TODO: Diagonostic
	}
}

internal void
Win32InitDSound(HWND hWindow, int32 SamplesPerSecond, int32 BufferSize)
{
	// NOTE: Load DirectSound library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if(DSoundLibrary) 
	{
		// NOTE: Get a DirectSound object
		direct_sound_create *DirectSoundCreate = (direct_sound_create*)
			GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		// TODO: Test whether or not this is compatible with WinXP - if not, use DirectSound7.
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {0};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2; // stereo
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16; // 16-bit audio
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if(SUCCEEDED(DirectSound->SetCooperativeLevel(hWindow, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {0};
				BufferDescription.dwSize = sizeof(BufferDescription);
				//DSBUFFERDESC BufferDescription = {sizeof(BufferDescription)};
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER; 
				
				// NOTE: Create primary buffer
				// TODO: Consider DSBCAPS_GLOBALFOCUS
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					HRESULT ErrorCode;
					if(SUCCEEDED(ErrorCode = PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						// NOTE: Format successfully set
						OutputDebugStringA("Primary buffer format set successfully");
					}
					else
					{
						// TODO: Diagnostic
					}
				}
				else
				{
					// TODO: Diagnostic
				}

			}
			else
			{
				// TODO: Diagnostic
			}

			// NOTE: Create a secondary buffer
			// TODO: DSBCAPS_GETCURRENTPOSITION2
			//DSBUFFERDESC BufferDescription = {sizeof(BufferDescription)};
			DSBUFFERDESC BufferDescription = {0};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 8;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			HRESULT ErrorCode;
			if(SUCCEEDED(ErrorCode = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondBuffer, 0 )))
			{
				// NOTE: Start playing
				OutputDebugStringA("Secondary buffer created successfully");
			}
			

			// NOTE: Start playing
		}
		else
		{
			// TODO: Diagnostic
		}
	}

}

internal win32_window_dimension 
Win32GetWindowDimension(HWND hWindow)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(hWindow, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return Result;
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, LONG Width, LONG Height)
{
	// TODO: Bulletproof this
	// Don't free first; free after then free first if fail

	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	int BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BytesPerPixel = 4;
	int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	//TODO: Clear to black

	Buffer->Pitch = Width*BytesPerPixel;
}

internal void
Win32DisplayBuffer(win32_offscreen_buffer *Buffer,
				   HDC DeviceContext, int WindowWidth, int WindowHeight)
{
	/// TODO: Aspect ratio correction
	StretchDIBits(DeviceContext,
				  /*
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  */
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer->Width, Buffer->Height,
				  Buffer->Memory,
				  &Buffer->Info,
				  DIB_RGB_COLORS, SRCCOPY);

}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
	// my attempt to illustrate the buffer
	// [int16 int16][int16 int16] . . .
	// [LEFT  RIGHT][LEFT  RIGHT] . . . 
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;

	if(SUCCEEDED(GlobalSecondBuffer->Lock(ByteToLock, BytesToWrite,
							&Region1, &Region1Size,
							&Region2, &Region2Size,
							0)))
	{
		// TODO: validate sizes
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;						
		int16 *SampleOut = (int16*)Region1;

		for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			real32 T = 2.0f * pi32 *(real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
			real32 SineValue = sinf(T);
			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue; // left channel
			*SampleOut++ = SampleValue; // right channel
			++SoundOutput->RunningSampleIndex;
		}

		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
		SampleOut = (int16*)Region2;
		for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			real32 T = 2.0f * pi32 *(real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput-> WavePeriod;
			real32 SineValue = sinf(T);
			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);							
			*SampleOut++ = SampleValue; // left channel
			*SampleOut++ = SampleValue; // right channel
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
				   UINT Message,
				   WPARAM WParam,
				   LPARAM LParam) 
{
	LRESULT Result = 0;

	switch(Message)
	{
		case WM_SIZE:
		{
		} break;

		case WM_CLOSE:
		{
			// TODO: Handle this with user
			GlobalRunning = FALSE;
			OutputDebugStringA("WM_CLOSE\n");
		} break;

		case WM_DESTROY:
		{
			// TODO: Handle this as an error - recreate window?
			GlobalRunning = FALSE;
			OutputDebugStringA("WM_DESTROY\n");
		} break;

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown  = ((LParam & (1 << 31)) == 0);

			if(WasDown != IsDown)
			{
				switch(VKCode)
				{
					case 'W':
					{
					} break;
					case 'A':
					{
					} break;
					case 'S':
					{
					} break;
					case 'D':
					{
					} break;
					case VK_UP:
					{
					} break;
					case VK_DOWN:
					{
					} break;
					case VK_LEFT:
					{
					} break;
					case VK_RIGHT:
					{
					} break;
					case VK_ESCAPE:
					{
					} break;
					case VK_SPACE:
					{
					} break;
				}
			}

			// handle ALT+F4
			bool32 AltKeyWasDown = (LParam & (1 << 29));
			if((VKCode == VK_F4) && AltKeyWasDown)
			{
				GlobalRunning = FALSE;
			}


		} break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			LONG X = Paint.rcPaint.left;
			LONG Y = Paint.rcPaint.top;
			LONG Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			LONG Width  = Paint.rcPaint.right - Paint.rcPaint.left;

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBuffer(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &Paint);
		} break;

		default:
		{
			//OutputDebugStringA("default\n");
			Result = DefWindowProc(Window, Message, WParam, LParam);
		} break;
	}

	return Result;
}

int CALLBACK
WinMain(HINSTANCE hInstance,
		HINSTANCE hPrevInstance,
		LPSTR lpCmdLine,
		int nCmdShow)
{
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart; // more practical representation

	Win32LoadXInput();

	WNDCLASS WindowClass = {};

	//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = hInstance;
	//WindowClass.hIcon =
	WindowClass.lpszClassName = "GameWindowClass";
	
	if(RegisterClassA(&WindowClass))
	{
		HWND hWindow =
			CreateWindowEx(
				0,
				WindowClass.lpszClassName,
				"Baked Salmon",
				WS_OVERLAPPEDWINDOW|WS_VISIBLE,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				CW_USEDEFAULT,
				0,
				0,
				hInstance,
				0);

		if(hWindow)
		{
			// graphics test
			int XOffset = 0;
			int YOffset = 0;

			// sound test
			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;	// 48 KHz
			SoundOutput.Frequency = 256; 			// 256 Hz
			SoundOutput.ToneVolume = 1000;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.Frequency;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.SecondBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			Win32InitDSound(hWindow, SoundOutput.SamplesPerSecond, SoundOutput.SecondBufferSize);
			Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondBufferSize);
			GlobalSecondBuffer->Play(0, 0, DSBPLAY_LOOPING);
			
			GlobalRunning = TRUE;

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			uint64 LastCPUCycleCount = __rdtsc();
			while(GlobalRunning)
			{
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						GlobalRunning = FALSE;
					}

					TranslateMessage(&Message);
					DispatchMessage(&Message);					
				} 

				// TODO: Pull frequency?
				for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT;
					++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
					{
						//NOTE: plugged in
						//TODO: check ControllerState.dwPacketNumber frequency
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						BOOL DPadUp 	= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						BOOL DPadRight 	= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						BOOL DPadLeft 	= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						BOOL DPadDown 	= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						BOOL Start 		= (Pad->wButtons & XINPUT_GAMEPAD_START);
						BOOL Back 		= (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						BOOL LeftThumb  = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_THUMB);
						BOOL RightThumb = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB);
						BOOL AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						BOOL BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						BOOL XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						BOOL YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;

						// lots of bullshit 
						XINPUT_VIBRATION Vibration;
						Vibration.wLeftMotorSpeed = 60000;
						Vibration.wRightMotorSpeed = 60000;
						XInputSetState(0, &Vibration);

						// TODO: Handle deadzone
						// XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
						// XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE

						XOffset += StickX / 4096;
						YOffset += StickY / 4096;

						SoundOutput.Frequency = 512 + (int)(256.0f * ((real32)StickY / 30000.0f));
						SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.Frequency;
					}
					else
					{
						// unplugged
					}
				}

				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed = 60000;
				Vibration.wRightMotorSpeed = 60000;
				XInputSetState(0, &Vibration);		
				HDC DeviceContext = GetDC(hWindow);

				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackbuffer.Memory;
				Buffer.Width = GlobalBackbuffer.Width;
				Buffer.Height = GlobalBackbuffer.Height;
				Buffer.Pitch = GlobalBackbuffer.Pitch;
				GameUpdateAndRender(&Buffer, XOffset, YOffset);

				// NOTE: DirectSound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if(SUCCEEDED(GlobalSecondBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD ByteToLock = ((SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % 
										SoundOutput.SecondBufferSize);
					DWORD BytesToWrite;
					if(BytesToWrite > SoundOutput.SecondBufferSize)
					{
						BytesToWrite = SoundOutput.SecondBufferSize - ByteToLock;
						BytesToWrite += PlayCursor;
					}
					else
					{
						BytesToWrite = PlayCursor - ByteToLock;
					}

					Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
				}
				
				win32_window_dimension Dimension = Win32GetWindowDimension(hWindow);
				Win32DisplayBuffer(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
				ReleaseDC(hWindow, DeviceContext);

				int64 EndCPUCycleCount = __rdtsc();

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);

				// TODO: Display value
				uint64 CPUCyclesElapsed = EndCPUCycleCount - LastCPUCycleCount;
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				real32 MSPerFrame = (real32)(1000*CounterElapsed) / PerfCountFrequency;
				real32 FPS = PerfCountFrequency / CounterElapsed;
				real32 MCPF = (real32)(CPUCyclesElapsed / (1000*1000)); // mega CPU cycles per frame

				// TODO: DEBUG-ONLY
#if 0
				char Buffer[256];
				sprintf(Buffer, "%.02fms/f - %.02ff/s - %.02fmc/f\n", MSPerFrame, FPS, MCPF);
				OutputDebugStringA(Buffer);
#endif

				LastCounter = EndCounter;
			}

		}
		else
		{
			// TODO: Logging
		}
	}
	else
	{
		// TODO: Logging
	}

	return 0;
}