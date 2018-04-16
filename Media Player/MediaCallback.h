#pragma once

#define COBJMACROS
#include <mfapi.h>
#pragma comment(lib, "mf")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")

#include <evr.h>
#pragma comment(lib, "Strmiids")

typedef struct TypeFrame
{
	UINT32 Height;
	UINT32 Width;
	UINT32 Denominator;
	UINT32 Numerator;
}TypeFrame;

typedef struct DeviceType
{
	IMFMediaTypeHandler *pTypeHandler;
	IMFMediaType **ppMediaType;
	TypeFrame *pTypeFrame;
	LPWSTR *pTypeFormat;
	UINT32 TypeCount;
}DeviceType;

typedef struct MediaDeviceSet
{
	IMFActivate **ppActivate;
	LPWSTR *pDeviceName;
	IMFMediaSource **ppSource;
	DeviceType *pDeviceType;
	UINT32 DeviceCount;
}MediaDeviceSet;

HRESULT MediaDeviceSetCreate(MediaDeviceSet *pDeviceSet);
HRESULT MediaDeviceSetRelease(MediaDeviceSet *pDeviceSet);
void	MediaDeviceSetName(MediaDeviceSet *pDeviceSet, LPWSTR **ppDeviceName, UINT32 *pDeviceCount);
void	MediaDeviceSetTypeFormat(MediaDeviceSet *pDeviceSet, UINT32 DeviceIndex, LPWSTR **ppTypeFormat, UINT32 *pTypeCount);
HRESULT MediaDeviceSetSource(MediaDeviceSet *pDeviceSet, UINT32 DeviceIndex, UINT32 TypeIndex, IMFMediaSource **ppSource);


typedef interface MediaAsyncCallback MediaAsyncCallback;

typedef struct MediaAsyncCallbackVtbl
{
	HRESULT(STDMETHODCALLTYPE *QueryInterface)(MediaAsyncCallback *pAsyncCallback, REFIID riid, void **ppvObject);
	ULONG(STDMETHODCALLTYPE *AddRef)(MediaAsyncCallback *pAsyncCallback);
	ULONG(STDMETHODCALLTYPE *Release)(MediaAsyncCallback *pAsyncCallback);
	HRESULT(STDMETHODCALLTYPE *GetParameters)(MediaAsyncCallback *pAsyncCallback, DWORD *pdwFlags, DWORD *pdwQueue);
	HRESULT(STDMETHODCALLTYPE *Invoke)(MediaAsyncCallback *pAsyncCallback, IMFAsyncResult* pAsyncResult);
	HRESULT(STDMETHODCALLTYPE *Start)(MediaAsyncCallback *pAsyncCallback);
	HRESULT(STDMETHODCALLTYPE *Shutdown)(MediaAsyncCallback *pAsyncCallback);
	HRESULT(STDMETHODCALLTYPE *Open)(MediaAsyncCallback *pAsyncCallback, IMFMediaSource *pSource, HWND hwndVideo);
	HRESULT(STDMETHODCALLTYPE *Repaint)(MediaAsyncCallback *pAsyncCallback);
	HRESULT(STDMETHODCALLTYPE *SetVideoPosition)(MediaAsyncCallback *pAsyncCallback, const WORD Width, const WORD Height);
}MediaAsyncCallbackVtbl;

typedef enum MediaState
{
	Close,
	Open,
	Start
}MediaState;

interface MediaAsyncCallback
{
	CONST_VTBL struct MediaAsyncCallbackVtbl *lpVtbl;
	ULONG RefCount;
	IMFMediaSession *pSession;
	IMFMediaSource *pSource;
	IMFVideoDisplayControl *pDisplayControl;
	HANDLE CloseEvent;
	MediaState State;
};

HRESULT MediaAsyncCallbackCreate(MediaAsyncCallback *pAsyncCallback);
#define MediaAsyncCallback_QueryInterface(This, riid, ppvObject) ((This)->lpVtbl->QueryInterface(This, riid, ppvObject))
#define MediaAsyncCallback_AddRef(This) ((This)->lpVtbl->AddRef(This))
#define MediaAsyncCallback_Release(This) ((This)->lpVtbl->Release(This))
#define MediaAsyncCallback_GetParameters(This, pdwFlags, pdwQueue) ((This)->lpVtbl->GetParameters(This, pdwFlags, pdwQueue))
#define MediaAsyncCallback_Invoke(This, pAsyncResult) ((This)->lpVtbl->Invoke(This, pAsyncResult))
#define MediaAsyncCallback_Start(This) ((This)->lpVtbl->Start(This))
#define MediaAsyncCallback_Shutdown(This) ((This)->lpVtbl->Shutdown(This))
#define MediaAsyncCallback_Open(This, pSource, hwndVideo) ((This)->lpVtbl->Open(This, pSource, hwndVideo))
#define MediaAsyncCallback_Repaint(This) ((This)->lpVtbl->Repaint(This))
#define MediaAsyncCallback_SetVideoPosition(This, Width, Height) ((This)->lpVtbl->SetVideoPosition(This, Width, Height))
