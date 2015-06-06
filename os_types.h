#pragma once

#if defined(_WIN32) || defined (WIN32)

#else
	#define HWND unsigned long
	#define HDC unsigned long
	#define INFINITE ((unsigned int)-1)
	#define Sleep(x) usleep((x)*1000)
	#define THREAD_PRIORITY_NORMAL 1
	#define THREAD_PRIORITY_ABOVE_NORMAL 10
	#define LPVOID void *
	#define DWORD unsigned int
	#define WINAPI
#endif
