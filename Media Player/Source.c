#include "MediaPlayer.h"
#include <assert.h>
#include <tchar.h>

#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore")


int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	HRESULT hResult = SetProcessDpiAwareness(PROCESS_SYSTEM_DPI_AWARE);
	if (FAILED(hResult))
	{
		assert(FALSE);
	}

	hResult = MFStartup(MF_VERSION, MFSTARTUP_FULL);
	if (FAILED(hResult))
	{
		assert(FALSE);
	}

	BOOL bResult = MediaRegisterClass(hInstance);
	if (FALSE == bResult)
	{
		assert(FALSE);
	}

	MediaPlayer Player = { 0 };
	bResult = MediaPlayerCreate(&Player, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, hInstance);
	if (FALSE == bResult)
	{
		assert(FALSE);
	}

	MSG msg;
	while (FALSE != (bResult = GetMessage(&msg, NULL, 0, 0)))
	{
		if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	hResult = MediaAsyncCallback_Shutdown(&Player.AsyncCallback);
	if (FAILED(hResult))
	{
		assert(FALSE);
	}
	MediaAsyncCallback_Release(&Player.AsyncCallback);

	hResult = MediaDeviceSetRelease(&Player.DeviceSet);
	if (FAILED(hResult))
	{
		assert(FALSE);
	}

	hResult = MFShutdown();
	if (FAILED(hResult))
	{
		assert(FALSE);
	}

	return (int)msg.wParam;
}
