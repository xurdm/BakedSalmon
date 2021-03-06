#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

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

/** Avoiding compatibility issues with <Windows Vista, trust me. **/
// MOTE: XInputGetState
// define stub for cases where XInputGetState is not available
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return 0;
};
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// MOTE: XInputSetState
// define stub for cases where XInputSetState is not available
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return 0;
};
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
Win32LoadXInput(void)
{
	// loading XInput 1.3 for compatibility
	HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
	if(XInputLibrary)
	{
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

// TODO: Will not work as well in future.
global_variable BOOL Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;


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
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
	int XThresh = 1;
	int XCur = 1;
	uint8 *Row = (uint8*)Buffer.Memory;
	win32_pixel_color PixelColor;

	for(int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32 *Pixel = (uint32*)Row;
		for(int X = 0; X < Buffer.Width; ++X)
		{
			//uint8 Blue = (uint8)(X+XOffset);
			//uint8 Green = (uint8)(X+YOffset);
			/*uint8 Blue = (uint8)0;
			uint8 Green = (uint8)0;
			uint8 Red = (uint8)0;*/
			/*
				Memory:    BB GG RR xx   (xx = pad)
				Register:  xx RR GG BB

				Pixel (32-bit)
			*/

			if(XCur < XThresh)
			{
				PixelColor.Blue = (uint8)0;
				PixelColor.Green = (uint8)0;
				PixelColor.Red = (uint8)0;
				*Pixel++ = ((PixelColor.Red << 16) | (PixelColor.Green << 8) | PixelColor.Blue);
			}
			else
			{
				PixelColor.Blue = (uint8)(X+XOffset);
				PixelColor.Green = (uint8)(Y+YOffset);
				*Pixel++ = ((PixelColor.Green << 8) | PixelColor.Blue);
			}

				/**Pixel++ = ((Green << 8) | Blue);
				XCur = XCur / 2;

				*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
				XCur = 32;*/
				XCur++;
		}
		XThresh = XThresh == 256 ? 1 : XThresh*2;
		XCur = XCur > XThresh ? 1 : XCur;
		Row += Buffer.Pitch;
	}
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
	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BytesPerPixel = 4;
	int BitmapMemorySize = (Buffer->Width*Buffer->Height)*Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	//TODO: Clear to black

	Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void
Win32DisplayBuffer(HDC DeviceContext,
				   int WindowWidth, int WindowHeight,
				   win32_offscreen_buffer Buffer, 
				   LONG X, LONG Y, LONG Width, LONG Height)
{
	/// TODO: Aspect ratio correction
	StretchDIBits(DeviceContext,
				  /*
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  */
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer.Width, Buffer.Height,
				  Buffer.Memory,
				  &Buffer.Info,
				  DIB_RGB_COLORS, SRCCOPY);

}

LRESULT CALLBACK
MainWindowCallback(HWND Window,
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
			Running = FALSE;
			OutputDebugStringA("WM_CLOSE\n");
		} break;

		case WM_DESTROY:
		{
			// TODO: Handle this as an error - recreate window?
			Running = FALSE;
			OutputDebugStringA("WM_DESTROY\n");
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
			Win32DisplayBuffer(DeviceContext, Dimension.Width, Dimension.Height,
							  GlobalBackbuffer, X, Y, Width, Height);
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
	Win32LoadXInput();

	WNDCLASS WindowClass = {0};

	//win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);

	WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
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
			int XOffset = 0;
			int YOffset = 0;

			Running = TRUE;
			while(Running)
			{
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						Running = FALSE;
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

						//lots of bullshit 
						XINPUT_VIBRATION Vibration;
						Vibration.wLeftMotorSpeed = 60000;
						Vibration.wRightMotorSpeed = 60000;
						XInputSetState(0, &Vibration);

						if(AButton)
						{
							YOffset += 2;
						}
					}
					else
					{
						//unplugged
					}
				}

				XINPUT_VIBRATION Vibration;
				Vibration.wLeftMotorSpeed = 60000;
				Vibration.wRightMotorSpeed = 60000;
				XInputSetState(0, &Vibration);

				RenderWeirdGradient(GlobalBackbuffer, ++XOffset, YOffset);
				HDC DeviceContext = GetDC(hWindow);

				win32_window_dimension Dimension = Win32GetWindowDimension(hWindow);
				Win32DisplayBuffer(DeviceContext, Dimension.Width, Dimension.Height,
										 GlobalBackbuffer,
										 0, 0, Dimension.Width, Dimension.Height);
				ReleaseDC(hWindow, DeviceContext);
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