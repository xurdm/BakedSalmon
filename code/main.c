#include <windows.h>
#include <stdint.h>

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

// TODO: Will not work as well in future.
global_variable BOOL Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;


win32_window_dimension Win32GetWindowDimension(HWND hWindow)
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
	uint8 *Row = (uint8*)Buffer.Memory;
	int min = 8; int Width = Buffer.Width;
	for(int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32 *Pixel = (uint32*)Row;
		for(int X = 0; X < Buffer.Width; ++X)
		{
			uint8 Blue = (uint8)(X + XOffset);
			uint8 Green = (uint8)(X + YOffset);

			/*
				Memory:    BB GG RR xx   (xx = pad)
				Register:  xx RR GG BB

				Pixel (32-bit)
			*/

			*Pixel++ = ((Green << 8) | Blue);
		}

		if(Buffer.Width != 1)
		{
			Buffer.Width = Buffer.Width / 2;
		}
		else
		{
			Buffer.Width = Width;
		}

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
	StretchDIBits(DeviceContext,
				  /*
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  */
				  0, 0, Buffer.Width, Buffer.Height,
				  0, 0, WindowWidth, WindowHeight,
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
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
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
	WNDCLASS WindowClass = {0};

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