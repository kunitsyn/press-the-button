// Some code is taken from http://www.cs.rit.edu/~ncs/Courses/570/UserGuide/OpenGLonWin-11.html.

#include <Windows.h>
#include <tchar.h>
#include <inttypes.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include "glew.h"

#include "text.h"

#ifdef __cplusplus
#error Compile it as C.
#endif

#define MAX_FPS 120

TCHAR *className  = _T("PressTheButton");
TCHAR *windowName = _T("Press The Button");
int winWidth = 300, winHeight = 300;

HDC hDC;
HGLRC hGLRC;
HPALETTE hPalette;
BOOL buttonPressed;
LARGE_INTEGER iterStart;
GLint texture;
unsigned int texw, texh;

static int PHASE_LEN = 10000000;

struct Vector2
{
	GLfloat x;
	GLfloat y;
};

void
init()
{
	if (glewInit() != GLEW_OK)
		exit(-1);
	if (!GLEW_VERSION_1_3)
		exit(-1);

	glDrawBuffer(GL_FRONT);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	if (glGetError() != GL_NO_ERROR)
		exit(-1);

	if (init_text())
		exit(-1);

	if (render_text("testing", &texture, &texw, &texh))
		exit(-1);
}

void
redraw()
{
	int phase = iterStart.QuadPart % PHASE_LEN;
	float r, g, b;
	if (phase < PHASE_LEN / 3)
	{
		g = phase / (PHASE_LEN / 3.);
		r = 1. - g;
		b = 0;
	}
	else if (phase < PHASE_LEN / 3 * 2)
	{
		b = (phase - PHASE_LEN / 3.) / (PHASE_LEN / 3.);
		g = 1. - b;
		r = 0;
	}
	else
	{
		r = (phase - PHASE_LEN / 3. * 2) / (PHASE_LEN / 3.);
		b = 1. - r;
		g = 0;
	}

	/* gamma correction (luminance to RGB) */
	r = pow(r, 1/2.2);
	g = pow(g, 1/2.2);
	b = pow(b, 1/2.2);

	glClearColor(r, g, b, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	glBegin(GL_TRIANGLE_FAN);

	const int CircleFaces = 200;
	const int CircleRadius = 500;
	const struct Vector2 circle_center = {.x = winWidth / 2, .y = winHeight / 2};
	glVertex2f(circle_center.x, circle_center.y);

	for (int i = 0; i < CircleFaces + 1; i++)
	{
		float angle = i * M_PI * 2 / CircleFaces;
		const struct Vector2 circle_point = {
			circle_center.x + sin(angle) * CircleRadius,
			circle_center.y - cos(angle) * CircleRadius
		};

		glVertex2f(circle_point.x, circle_point.y);

		if (i == 30)
		{
			glEnd();
			glColor4f(0, 1, 0, 1);
			glBegin(GL_TRIANGLE_FAN);
			glVertex2f(circle_center.x, circle_center.y);
			glVertex2f(circle_point.x, circle_point.y);
		}
	}

	glEnd();

	glColor4f(1, 1, 0, 1);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2f(0.0f, 0.0f);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2f(0.0f, texh);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2f(texw, texh);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2f(texw, 0.0f);
	glEnd();

	glFinish();
}

void
resize()
{
	glViewport(0, 0, winWidth, winHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, winWidth, winHeight, 0, -1.0, 1.0);

	glMatrixMode(GL_MODELVIEW);
}

void
setupPixelFormat(
	HDC hDC)
{
	const PIXELFORMATDESCRIPTOR pfd = {
		.nSize      = sizeof(PIXELFORMATDESCRIPTOR),
		.nVersion   = 1,
		.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 16,
		.cDepthBits = 16,
		.iLayerType = PFD_MAIN_PLANE
	};

	const int pixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (pixelFormat == 0)
	{
		MessageBox(
			WindowFromDC(hDC),
			"ChoosePixelFormat failed.",
			"Error",
			MB_ICONERROR | MB_OK);
		exit(1);
	}

	if (!SetPixelFormat(hDC, pixelFormat, &pfd))
	{
		MessageBox(
			WindowFromDC(hDC),
			"SetPixelFormat failed.",
			"Error",
			MB_ICONERROR | MB_OK);
		exit(1);
	}
}

void
setupPalette(
	HDC hDC)
{
	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(hDC, GetPixelFormat(hDC), sizeof(PIXELFORMATDESCRIPTOR), &pfd);

	if (!(pfd.dwFlags & PFD_NEED_PALETTE))
		return;

	const int paletteSize = 1 << pfd.cColorBits;

	LOGPALETTE* pPal = malloc(sizeof(LOGPALETTE) + paletteSize * sizeof(PALETTEENTRY));
	pPal->palVersion = 0x300;
	pPal->palNumEntries = paletteSize;

	/* build a simple RGB color palette */
	{
		const int redMask   = (1 << pfd.cRedBits  ) - 1;
		const int greenMask = (1 << pfd.cGreenBits) - 1;
		const int blueMask  = (1 << pfd.cBlueBits ) - 1;

		for (int i = 0; i < paletteSize; ++i)
		{
			pPal->palPalEntry[i] = (PALETTEENTRY){
				(((i >> pfd.cRedShift  ) & redMask  ) * 255) / redMask,
				(((i >> pfd.cGreenShift) & greenMask) * 255) / greenMask,
				(((i >> pfd.cBlueShift ) & blueMask ) * 255) / blueMask,
				0
			};
		}
	}

	hPalette = CreatePalette(pPal);
	free(pPal);

	if (hPalette)
	{
		SelectPalette(hDC, hPalette, FALSE);
		RealizePalette(hDC);
	}
}

LRESULT APIENTRY
WndProc(
	HWND hWnd,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (message)
	{
		case WM_CREATE:
		{
			/* initialize OpenGL rendering */
			hDC = GetDC(hWnd);
			setupPixelFormat(hDC);
			setupPalette(hDC);
			hGLRC = wglCreateContext(hDC);
			wglMakeCurrent(hDC, hGLRC);
			init();
			return 0;
		}
		case WM_DESTROY:
		{
			/* finish OpenGL rendering */
			if (hGLRC)
			{
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(hGLRC);
			}
			if (hPalette)
				DeleteObject(hPalette);

			ReleaseDC(hWnd, hDC);
			PostQuitMessage(0);
			return 0;
		}
		case WM_SIZE:
		{
			/* track window size changes */
			if (hGLRC)
			{
				winWidth = (int)LOWORD(lParam);
				winHeight = (int)HIWORD(lParam);
				resize();
				return 0;
			}
			break;
		}
		case WM_PALETTECHANGED:
		{
			/* realize palette if this is *not* the current window */
			if (hGLRC && hPalette && (HWND)wParam != hWnd)
			{
				UnrealizeObject(hPalette);
				SelectPalette(hDC, hPalette, FALSE);
				RealizePalette(hDC);
				redraw();
				break;
			}
			break;
		}
		case WM_QUERYNEWPALETTE:
		{
			/* realize palette if this is the current window */
			if (hGLRC && hPalette)
			{
				UnrealizeObject(hPalette);
				SelectPalette(hDC, hPalette, FALSE);
				RealizePalette(hDC);
				redraw();
				return TRUE;
			}
			break;
		}
		case WM_PAINT:
		{
			/* we must handle repaint by ourselves */
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_ERASEBKGND:
		{
			return 1;
		}
		case WM_CHAR:
		{
			/* handle keyboard input */
			switch ((int)wParam)
			{
				case VK_ESCAPE:
				{
					DestroyWindow(hWnd);
					return 0;
				}
			}
			break;
		}
		case WM_KEYDOWN:
		{
			if (wParam == 0x57 /* w */)
				buttonPressed = TRUE;
			break;
		}
		case WM_KEYUP:
		{
			if (wParam == 0x57 /* w */)
				buttonPressed = FALSE;
			break;
		}
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int APIENTRY
WinMain(
	HINSTANCE hCurrentInst,
	HINSTANCE hPreviousInst,
	LPSTR lpszCmdLine,
	int nCmdShow)
{
	HWND hWnd;

	/* register window class */
	const WNDCLASS wndClass = {
		.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc   = WndProc,
		.hInstance     = hCurrentInst,
		.hIcon         = LoadIcon(NULL, IDI_APPLICATION),
		.hCursor       = LoadCursor(NULL, IDC_ARROW),
		.hbrBackground = NULL,
		.lpszClassName = className
	};
	if (!RegisterClass(&wndClass))
	{
		MessageBox(
			NULL,
			"RegisterClass failed.",
			"Error",
			MB_ICONERROR | MB_OK);
		exit(1);
	}

	winWidth  = GetSystemMetrics(SM_CXSCREEN);
	winHeight = GetSystemMetrics(SM_CYSCREEN);

	hWnd = CreateWindowEx(
		WS_EX_WINDOWEDGE | WS_EX_APPWINDOW,
		className,
		windowName,
		WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0, winWidth, winHeight,
		NULL, NULL, hCurrentInst, NULL);

	if (!hWnd)
	{
		MessageBox(
			NULL,
			"CreateWindow failed.",
			"Error",
			MB_ICONERROR | MB_OK);
		exit(1);
	}

	/* display window */
	ShowWindow(hWnd, nCmdShow);

	/* game loop */
	while (1)
	{
		/* query time */
		QueryPerformanceCounter(&iterStart);

		/* process Windows messages */
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				return msg.wParam;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		redraw();

		/* wait for MAX_FPS threshold */
		const int64_t sleepTill = iterStart.QuadPart + (int64_t)(1. / MAX_FPS * 1000000);
		LARGE_INTEGER iterEnd;
		QueryPerformanceCounter(&iterEnd);
		
		if (sleepTill > iterEnd.QuadPart)
			Sleep((sleepTill - iterEnd.QuadPart) / 1000);
	}
}
