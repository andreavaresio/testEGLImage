#pragma once

#include "assert.h" 
#include "os_types.h"
#include <string>
#include "string.h"

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include "CBaseThread.h"

class IAVWindowCBF
{
public:

	virtual void OnEnd() {};
};

class CAVWindow : public CBaseThread
{
	int m_cx, m_cy, m_PosX, m_PosY;
	std::wstring m_windowName;
	HWND m_hwnd;

	void OnInit()
	{
		const HINSTANCE hInstance = NULL;
		WNDCLASS            wc;

		/*
		* set up and register window class
		*/
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = WindowProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInstance;
		wc.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = m_windowName.c_str();
		RegisterClass(&wc);

		/*
		* create a window
		*/
		RECT rc = { 0, 0, m_cx, m_cy };
		AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
		m_hwnd = CreateWindowEx(
			NULL,
			m_windowName.c_str(),
			m_windowName.c_str(),
			WS_OVERLAPPEDWINDOW,
			m_PosX,
			m_PosY,
			rc.right - rc.left,
			rc.bottom - rc.top,
			NULL,
			NULL,
			hInstance,
			NULL);

		assert(m_hwnd);
		SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG)this);

		ShowWindow(m_hwnd, SW_SHOW);
	}

	void DoLoop()
	{
		MSG Msg;

		//here we force queue creation. Very important, this must be done before sending any message...
		PeekMessage(&Msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

		while (GetMessage(&Msg, NULL, 0, 0))
		{
			TranslateMessage(&Msg);
			DispatchMessage(&Msg);
		}
	};

	void OnEnd()
	{
		DestroyWindow(m_hwnd);
	}

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		CAVWindow *pThis = (CAVWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
		long r = pThis->MyWindowProc(hWnd, message, wParam, lParam);
		return r;
	}
	long MyWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		switch (message)
		{
		case WM_CLOSE:
			//PostQuitMessage(0);
			return 0;
		case WM_PAINT:
			return 0; //break; 
		}
		return (long)DefWindowProc(hWnd, message, wParam, lParam);
	}

public:
	CAVWindow()
	{
		m_hwnd = NULL;
	}
	~CAVWindow()
	{
	}

	void CreaFinestra(int cx, int cy, const std::wstring &wName, int posx, int posy)
	{
		m_cx = cx;
		m_cy = cy;
		m_windowName = wName;
		m_PosX = posx;
		m_PosY = posy;

		Start();
	}

	void ChiudiFinestra()
	{
		if (m_bThreadRunning)
		{
			PostThreadMessage(m_ThreadId, WM_QUIT, 0, 0);
			WaitThreadEnded();
		}
	}
	HWND GethWnd()
	{
		return m_hwnd;
	}
};

#else

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "EGL/egl.h"

class CAVWindow
{
	int m_cx, m_cy, m_PosX, m_PosY;
	std::wstring m_windowName;
	Colormap m_colormap;
	XVisualInfo *m_visual;
    Window m_window;
    Display* m_display;

	static Bool wait_for_map(Display *display, XEvent *event, char *windowPointer) 
    {
    	return (event->type == MapNotify && event->xmap.window == (*((Window*)windowPointer)));
    }
public:
	CAVWindow()
	{
		m_display = NULL;
	}
	~CAVWindow()
	{
		ChiudiFinestra();
	}
	void CreaFinestra(int windowWidth, int windowHeight, const std::wstring &wName, int posx, int posy)
	{
		XSetWindowAttributes windowAttributes;
		XSizeHints sizeHints;
		XEvent event;
		XVisualInfo visualInfoTemplate;

		unsigned long mask;
		long screen;

		int visualID, numberOfVisuals;

		m_display = XOpenDisplay(NULL);
		if (!m_display)
			throw "Couldn't XOpenDisplay";

		screen = DefaultScreen(m_display);

		memset(&visualInfoTemplate, 0, sizeof(visualInfoTemplate));
		visualInfoTemplate.visualid = XVisualIDFromVisual(XDefaultVisual(m_display, screen));
		visualInfoTemplate.screen = screen;
		const long vinfo_mask = VisualIDMask | VisualScreenMask;
		m_visual = XGetVisualInfo(m_display, vinfo_mask, &visualInfoTemplate, &numberOfVisuals);
		if (m_visual == NULL)
			throw "Couldn't get X visual info";

		m_colormap = XCreateColormap(m_display, RootWindow(m_display, screen), m_visual->visual, AllocNone);

		windowAttributes.colormap = m_colormap;
		windowAttributes.background_pixel = 0xFFFFFFFF;
		windowAttributes.border_pixel = 0;
		windowAttributes.event_mask = StructureNotifyMask | ExposureMask;

		mask = CWBackPixel | CWBorderPixel | CWEventMask | CWColormap;

		m_window = XCreateWindow(m_display, RootWindow(m_display, screen), 0, 0, windowWidth, windowHeight,
			0, m_visual->depth, InputOutput, m_visual->visual, mask, &windowAttributes);
		sizeHints.flags = USPosition;
		sizeHints.x = 10;
		sizeHints.y = 10;

		XSetStandardProperties(m_display, m_window, "Mali OpenGL ES SDK", "", None, 0, 0, &sizeHints);
		XMapWindow(m_display, m_window);
		XIfEvent(m_display, &event, wait_for_map, (char*)(&m_window));
		XSetWMColormapWindows(m_display, m_window, &m_window, 1);
		XFlush(m_display);

		XSelectInput(m_display, m_window, KeyPressMask | ExposureMask | EnterWindowMask
			| LeaveWindowMask | PointerMotionMask | VisibilityChangeMask | ButtonPressMask
			| ButtonReleaseMask | StructureNotifyMask);
	}

	void ChiudiFinestra()
	{
		if (m_display)
		{
			XDestroyWindow(m_display, m_window);
			XFreeColormap(m_display, m_colormap);
			XFree(m_visual);
			XCloseDisplay(m_display);
			m_display = NULL;
		}
	}
	HWND GethWnd()
	{
		return (unsigned long)m_window;
	}

	Display* GetXDisplayPtr()
	{
		return m_display;
	}
};


#endif


