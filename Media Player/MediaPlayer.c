#include "MediaPlayer.h"
#include <windowsx.h>

#include <CommCtrl.h>
#pragma comment(lib, "Comctl32")

#include <tchar.h>
#include <assert.h>

#define MediaWindowName _T("Media Player")
#define MediaClassName _T("Media Class")
#define VideoClassName _T("Video Class")

LRESULT CALLBACK MediaWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK VideoWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL MediaRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX WindowClassEx;
	WindowClassEx.cbSize = sizeof(WNDCLASSEX);
	WindowClassEx.style = CS_HREDRAW | CS_VREDRAW;
	WindowClassEx.cbClsExtra = 0;
	WindowClassEx.cbWndExtra = 0;
	WindowClassEx.hInstance = hInstance;
	WindowClassEx.hIcon = NULL;
	WindowClassEx.hCursor = (HCURSOR)LoadImage((HINSTANCE)NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED);
	WindowClassEx.hbrBackground = NULL;
	WindowClassEx.lpszMenuName = NULL;
	WindowClassEx.hIconSm = NULL;

	WindowClassEx.lpfnWndProc = MediaWindowProc;
	WindowClassEx.lpszClassName = MediaClassName;
	if (0 == RegisterClassEx(&WindowClassEx))
	{
		return FALSE;
	}

	WindowClassEx.lpfnWndProc = VideoWindowProc;
	WindowClassEx.lpszClassName = VideoClassName;
	if (0 == RegisterClassEx(&WindowClassEx))
	{
		return FALSE;
	}

	INITCOMMONCONTROLSEX icex =
	{
		sizeof(INITCOMMONCONTROLSEX),
		ICC_USEREX_CLASSES
	};

	BOOL bResult = InitCommonControlsEx(&icex);
	if (FALSE == bResult)
	{
		return FALSE;
	}

	return  TRUE;
}

int MediaDeviceComboBoxUpdate(HWND DevicehWnd, MediaDeviceSet *pDeviceSet)
{
	//HFONT hFont = CreateFont(0, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
	//	CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Microsoft JhengHei"));
	//if (NULL == hFont)
	//{
	//	return FALSE;
	//}
	//SetWindowFont(DevicehWnd, hFont, FALSE);

	//COMBOBOXEXITEM cbei;

	int Result = ComboBox_ResetContent(DevicehWnd);
	if (CB_ERR == Result || CB_ERRSPACE == Result)
	{
		return Result;
	}

	LPWSTR *pDeviceName = pDeviceSet->pDeviceName;
	UINT32 DeviceCount = pDeviceSet->DeviceCount;
	for (UINT32 DeviceIndex = 0U; DeviceIndex < DeviceCount; DeviceIndex++)
	{

		Result = (int)SendMessageW(DevicehWnd, CB_ADDSTRING, (WPARAM)0, (LPARAM)pDeviceName[DeviceIndex]);
		if (CB_ERR == Result || CB_ERRSPACE == Result)
		{
			return Result;
		}
	}

	HDC hdc = GetWindowDC(DevicehWnd);
	if (NULL == hdc)
	{
		return CB_ERR;
	}

	//HGDIOBJ hgdiobj = SelectObject(hdc, (HGDIOBJ)hFont);
	//if (NULL == hgdiobj)
	//{
	//	return CB_ERR;
	//}

	SIZE size = { 0 };
	BOOL bResult = GetTextExtentPoint32W(hdc, *pDeviceName, lstrlenW(*pDeviceName), &size);
	if (FALSE == bResult)
	{
		return CB_ERR;
	}

	//(void)SelectObject(hdc, (HGDIOBJ)hFont);
	Result = ReleaseDC(DevicehWnd, hdc);
	if (0 == Result)
	{
		return CB_ERR;
	}

	//Result = DeleteObject(hFont);
	//if (0 == Result)
	//{
	//	return CB_ERR;
	//}

	LONG width = size.cx + 2 * GetSystemMetrics(SM_CXSIZEFRAME);
	Result = (int)SendMessage(DevicehWnd, CB_SETDROPPEDWIDTH, (WPARAM)width, (LPARAM)0);
	if (CB_ERR == Result || CB_ERRSPACE == Result)
	{
		return Result;
	}

	return CB_OKAY;
}

int MediaFormatComboBoxUpdate(HWND FormathWnd, MediaDeviceSet *pDeviceSet, int DeviceIndex)
{
	if (CB_ERR == DeviceIndex)
	{
		ComboBox_Enable(FormathWnd, FALSE);
	}
	else
	{
		ComboBox_Enable(FormathWnd, TRUE);

		int Result = ComboBox_ResetContent(FormathWnd);
		if (CB_ERR == Result || CB_ERRSPACE == Result)
		{
			return Result;
		}

		LPWSTR *pTypeFormat = pDeviceSet->pDeviceType[DeviceIndex].pTypeFormat;
		UINT32 FormatCount = pDeviceSet->pDeviceType[DeviceIndex].TypeCount;
		for (UINT32 FormatIndex = 0U; FormatIndex < FormatCount; FormatIndex++)
		{
			Result = (int)SendMessageW(FormathWnd, CB_ADDSTRING, (WPARAM)0, (LPARAM)pTypeFormat[FormatIndex]);
			if (CB_ERR == Result || CB_ERRSPACE == Result)
			{
				return Result;
			}
		}
	}
	return CB_OKAY;
}

BOOL MediaCreateWindow(MediaPlayer *pPlayer, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HINSTANCE hInstance)
{
	MediaDeviceSet *pDeviceSet = &pPlayer->DeviceSet;
	MediaAsyncCallback *pAsyncCallback = &pPlayer->AsyncCallback;
	MediaWindow *pWindow = &pPlayer->Window;

	HWND MediahWnd = CreateWindowEx(0UL, MediaClassName, MediaWindowName,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE, X, Y, nWidth, nHeight, hWndParent, NULL, hInstance, NULL);
	if (NULL == MediahWnd)
	{
		return FALSE;
	}

	HWND ToolbarhWnd = CreateWindowEx(0UL, TOOLBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | TBSTYLE_WRAPABLE, 0, 0, 0, 0, MediahWnd, NULL, hInstance, NULL);
	if (NULL == ToolbarhWnd)
	{
		return FALSE;
	}

	//long dwBaseUnits = GetDialogBaseUnits();
	HWND DevicehWnd = CreateWindowEx(0UL, WC_COMBOBOX, NULL,
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL, 0, 0, 200, 400, ToolbarhWnd, NULL, hInstance, NULL);
	if (NULL == DevicehWnd)
	{
		return FALSE;
	}

	HWND FormathWnd = CreateWindowEx(0UL, WC_COMBOBOX, NULL,
		WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL, 201, 0, 200, 400, ToolbarhWnd, NULL, hInstance, NULL);
	if (NULL == FormathWnd)
	{
		return FALSE;
	}

	//(void)SendMessage(ToolbarhWnd, TB_AUTOSIZE, (WPARAM)0, (LPARAM)0);


	RECT Rect = { 0 };
	BOOL bResult = GetWindowRect(ToolbarhWnd, &Rect);
	if (FALSE == bResult)
	{
		return FALSE;
	}

	LONG ToolbarHeight = Rect.bottom - Rect.top;
	HWND VideohWnd = CreateWindowEx(0UL, VideoClassName, NULL,
		WS_CHILD | WS_VISIBLE, 0, ToolbarHeight, 0, 0, MediahWnd, NULL, hInstance, NULL);
	if (NULL == VideohWnd)
	{
		return FALSE;
	}

	int Result = MediaDeviceComboBoxUpdate(DevicehWnd, pDeviceSet);
	if (CB_ERR == Result)
	{
		return FALSE;
	}

	Result = MediaFormatComboBoxUpdate(FormathWnd, pDeviceSet, CB_ERR);
	if (CB_ERR == Result)
	{
		return FALSE;
	}

	pWindow->MediahWnd = MediahWnd;
	pWindow->ToolbarhWnd = ToolbarhWnd;
	pWindow->DevicehWnd = DevicehWnd;
	pWindow->FormathWnd = FormathWnd;
	pWindow->VideohWnd = VideohWnd;
	pWindow->ToolbarHeight = ToolbarHeight;
	pWindow->DeviceIndex = CB_ERR;
	pWindow->FormatIndex = CB_ERR;

	if (0 == SetWindowLongPtr(MediahWnd, GWLP_USERDATA, (LONG_PTR)pPlayer) && 0 != GetLastError())
	{
		return FALSE;
	}

	if (0 == SetWindowLongPtr(VideohWnd, GWLP_USERDATA, (LONG_PTR)pAsyncCallback) && 0 != GetLastError())
	{
		return FALSE;
	}


	return TRUE;
}

BOOL MediaPlayerCreate(MediaPlayer *pPlayer, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HINSTANCE hInstance)
{
	HRESULT hResult = MediaDeviceSetCreate(&pPlayer->DeviceSet);
	if (FAILED(hResult))
	{
		return FALSE;
	}

	hResult = MediaAsyncCallbackCreate(&pPlayer->AsyncCallback);
	if (FAILED(hResult))
	{
		return FALSE;
	}

	BOOL bResult = MediaCreateWindow(pPlayer, X, Y, nWidth, nHeight, hWndParent, hInstance);
	if (FALSE == bResult)
	{
		return FALSE;
	}

	return TRUE;
}

LONG_PTR SetMediaWindowLongPtr(HWND hWnd, LPARAM lParam)
{
	LONG_PTR dwNewLong = (LONG_PTR)((LPCREATESTRUCTW)lParam)->lpCreateParams;
	if (0 == dwNewLong)
	{
		return 0;
	}

	if (0 == SetWindowLongPtr(hWnd, GWLP_USERDATA, dwNewLong))
	{
		if (0 != GetLastError())
		{
			return 0;
		}
	}
	return dwNewLong;
}

BOOL AutoSetWindowPos(HWND hWnd, LONG Width, LONG Height)
{
	DWORD dwStyle = GetWindowStyle(hWnd);
	if (0 == dwStyle)
	{
		return FALSE;
	}

	BOOL bResult = FALSE;
	if (dwStyle & WS_CHILD || NULL == GetMenu(hWnd))
	{
		bResult = FALSE;
	}
	else
	{
		bResult = TRUE;
	}

	RECT Rect = { 0L, 0L, Width, Height };
	bResult = AdjustWindowRect(&Rect, dwStyle, bResult);
	if (FALSE == bResult)
	{
		return FALSE;
	}

	int X = (int)(Rect.right - Rect.left);
	int Y = (int)(Rect.bottom - Rect.top);

	bResult = SetWindowPos(hWnd, HWND_TOP, 0, 0, X, Y, SWP_NOMOVE | SWP_NOOWNERZORDER);
	if (FALSE == bResult)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL MediaWindowSize(MediaWindow *pWindow, LPARAM lParam)
{
	LONG Width = (LONG)LOWORD(lParam);
	LONG Height = (LONG)HIWORD(lParam) - pWindow->ToolbarHeight;

	BOOL bResult = AutoSetWindowPos(pWindow->ToolbarhWnd, Width, 0L);
	if (FALSE == bResult)
	{
		return FALSE;
	}

	bResult = AutoSetWindowPos(pWindow->VideohWnd, Width, Height);
	if (FALSE == bResult)
	{
		return FALSE;
	}

	return TRUE;
}

int MediaComboBoxSelectChange(MediaPlayer *pPlayer, LPARAM lParam)
{
	MediaDeviceSet *pDeviceSet = &pPlayer->DeviceSet;
	MediaAsyncCallback *pAsyncCallback = &pPlayer->AsyncCallback;
	MediaWindow *pWindow = &pPlayer->Window;
	HWND hwndCtl = (HWND)lParam;
	int Index = ComboBox_GetCurSel(hwndCtl);

	if (pWindow->DevicehWnd == hwndCtl)
	{
		int Result = MediaFormatComboBoxUpdate(pWindow->FormathWnd, pDeviceSet, Index);
		if (CB_ERR == Result || CB_ERRSPACE == Result)
		{
			return Result;
		}

		pWindow->DeviceIndex = Index;
	}
	else if (pWindow->FormathWnd == hwndCtl)
	{
		LONG Width = pDeviceSet->pDeviceType[pWindow->DeviceIndex].pTypeFrame[Index].Width;
		LONG Height = pDeviceSet->pDeviceType[pWindow->DeviceIndex].pTypeFrame[Index].Height;

		BOOL bResult = AutoSetWindowPos(pWindow->MediahWnd, Width, Height + pWindow->ToolbarHeight);
		if (FALSE == bResult)
		{
			return CB_ERR;
		}

		HRESULT hResult;
		if (CB_ERR != pWindow->FormatIndex)
		{
			hResult = MediaAsyncCallback_Shutdown(pAsyncCallback);
			if (FAILED(hResult))
			{
				return CB_ERR;
			}
			MediaAsyncCallback_Release(pAsyncCallback);

			hResult = MediaDeviceSetRelease(pDeviceSet);
			if (FAILED(hResult))
			{
				return CB_ERR;
			}

			hResult = MediaDeviceSetCreate(pDeviceSet);
			if (FAILED(hResult))
			{
				return CB_ERR;
			}

			hResult = MediaAsyncCallbackCreate(pAsyncCallback);
			if (FAILED(hResult))
			{
				return CB_ERR;
			}
		}

		IMFMediaSource *pSource = NULL;
		hResult = MediaDeviceSetSource(pDeviceSet, pWindow->DeviceIndex, Index, &pSource);
		if (FAILED(hResult))
		{
			return CB_ERR;
		}

		hResult = MediaAsyncCallback_Open(pAsyncCallback, pSource, pWindow->VideohWnd);
		if (FAILED(hResult))
		{
			return CB_ERR;
		}

		pWindow->FormatIndex = Index;
	}
	else
	{
		return CB_ERR;
	}

	return CB_OKAY;
}

LRESULT CALLBACK MediaWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MediaPlayer *pPlayer = (MediaPlayer*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (NULL == pPlayer)
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	BOOL bResult = FALSE;
	switch (uMsg)
	{
	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:
			MediaComboBoxSelectChange(pPlayer, lParam);
			break;
		}
		break;

	case WM_SIZE:
		bResult = MediaWindowSize(&pPlayer->Window, lParam);
		if (FALSE == bResult)
		{
			assert(FALSE);
		}
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK VideoWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MediaAsyncCallback *pAsyncCallback = (MediaAsyncCallback*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
	if (NULL == pAsyncCallback)
	{
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg)
	{
	case WM_PAINT:
		if (Close != pAsyncCallback->State)
		{
			MediaAsyncCallback_Repaint(pAsyncCallback);
		}
		break;

	case WM_SIZE:
		if (Close != pAsyncCallback->State)
		{
			WORD Width = LOWORD(lParam);
			WORD Height = HIWORD(lParam);

			HRESULT hResult = MediaAsyncCallback_SetVideoPosition(pAsyncCallback, Width, Height);
			if (FAILED(hResult))
			{
				assert(FALSE);
			}
		}
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}