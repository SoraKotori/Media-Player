#define _CRT_SECURE_NO_WARNINGS

#include <wchar.h>
#include <Mferror.h>
#include "MediaCallback.h"

HRESULT MediaEnumDeviceSources(IMFActivate ***pppActivate, UINT32 *pActivateCount)
{
	IMFAttributes *pAttributes = NULL;
	HRESULT hResult = MFCreateAttributes(&pAttributes, 1);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFAttributes_SetGUID(pAttributes, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hResult))
	{
		return hResult;
	}

	IMFActivate **ppActivate = NULL;
	UINT32  ActivateCount = 0;
	hResult = MFEnumDeviceSources(pAttributes, &ppActivate, &ActivateCount);
	if (FAILED(hResult))
	{
		return hResult;
	}
	IMFAttributes_Release(pAttributes);

	if (0 == ActivateCount)
	{
		return MF_E_NOT_FOUND;
	}

	*pActivateCount = ActivateCount;
	*pppActivate = ppActivate;
	return hResult;
}

HRESULT MediaSourceTypeHandler(IMFMediaSource *pSource, IMFMediaTypeHandler **ppTypeHandler)
{
	IMFPresentationDescriptor *PresentationDescriptor = NULL;
	HRESULT hResult = IMFMediaSource_CreatePresentationDescriptor(pSource, &PresentationDescriptor);
	if (FAILED(hResult))
	{
		return hResult;
	}

	BOOL fSelected = FALSE;
	IMFStreamDescriptor *StreamDescriptor = NULL;
	hResult = IMFPresentationDescriptor_GetStreamDescriptorByIndex(PresentationDescriptor, 0UL, &fSelected, &StreamDescriptor);
	if (FAILED(hResult))
	{
		return hResult;
	}
	IMFPresentationDescriptor_Release(PresentationDescriptor);

	IMFMediaTypeHandler *pTypeHandler = NULL;
	hResult = IMFStreamDescriptor_GetMediaTypeHandler(StreamDescriptor, &pTypeHandler);
	if (FAILED(hResult))
	{
		return hResult;
	}
	IMFStreamDescriptor_Release(StreamDescriptor);

	*ppTypeHandler = pTypeHandler;
	return hResult;
}

HRESULT MediaTypeFrame(IMFMediaType *pMediaType, TypeFrame *pTypeFrame, LPWSTR *pTypeFormat)
{
	TypeFrame TypeFrame = { 0 };
	HRESULT hResult = IMFMediaType_GetUINT64(pMediaType, &MF_MT_FRAME_SIZE, (UINT64*)&TypeFrame.Height);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFMediaType_GetUINT64(pMediaType, &MF_MT_FRAME_RATE, (UINT64*)&TypeFrame.Denominator);
	if (FAILED(hResult))
	{
		return hResult;
	}

	int length = _snwprintf(NULL, 0U, L"%4u¡Ñ%-4u %ufps",
		TypeFrame.Width, TypeFrame.Height, TypeFrame.Numerator / TypeFrame.Denominator);
	if (0 > length)
	{
		return S_FALSE;
	}

	LPWSTR TypeFormat = (LPWSTR)HeapAlloc(GetProcessHeap(), 0UL, sizeof(WCHAR) * ++length);
	if (NULL == TypeFormat)
	{
		return S_FALSE;
	}

	length = _snwprintf(TypeFormat, length, L"%4u¡Ñ%-4u %ufps",
		TypeFrame.Width, TypeFrame.Height, TypeFrame.Numerator / TypeFrame.Denominator);
	if (0 > length)
	{
		return S_FALSE;
	}

	*pTypeFrame = TypeFrame;
	*pTypeFormat = TypeFormat;
	return S_OK;
}

HRESULT MediaDeviceTypeCreate(IMFMediaSource *pSource, DeviceType *pDeviceType)
{
	IMFMediaTypeHandler *pTypeHandler = NULL;
	HRESULT hResult = MediaSourceTypeHandler(pSource, &pTypeHandler);
	if (FAILED(hResult))
	{
		return hResult;
	}

	DWORD dwTypeCount = 0UL;
	hResult = IMFMediaTypeHandler_GetMediaTypeCount(pTypeHandler, &dwTypeCount);
	if (FAILED(hResult))
	{
		return hResult;
	}
	UINT32 TypeCount = (UINT32)dwTypeCount;

	IMFMediaType **ppMediaType = (IMFMediaType**)HeapAlloc(GetProcessHeap(), 0UL, sizeof(IMFMediaType*) * TypeCount);
	if (NULL == ppMediaType)
	{
		return S_FALSE;
	}

	TypeFrame *pTypeFrame = (TypeFrame*)HeapAlloc(GetProcessHeap(), 0UL, sizeof(TypeFrame) * TypeCount);
	if (NULL == pTypeFrame)
	{
		return S_FALSE;
	}

	LPWSTR *pTypeFormat = (LPWSTR*)HeapAlloc(GetProcessHeap(), 0UL, sizeof(TypeFrame) * TypeCount);
	if (NULL == pTypeFormat)
	{
		return S_FALSE;
	}

	for (UINT32 TypeIndex = 0; TypeIndex < TypeCount; TypeIndex++)
	{
		hResult = IMFMediaTypeHandler_GetMediaTypeByIndex(pTypeHandler, TypeIndex, &ppMediaType[TypeIndex]);
		if (FAILED(hResult))
		{
			return hResult;
		}

		hResult = MediaTypeFrame(ppMediaType[TypeIndex], &pTypeFrame[TypeIndex], &pTypeFormat[TypeIndex]);
		if (FAILED(hResult))
		{
			return hResult;
		}
	}

	pDeviceType->pTypeHandler = pTypeHandler;
	pDeviceType->ppMediaType = ppMediaType;
	pDeviceType->pTypeFrame = pTypeFrame;
	pDeviceType->pTypeFormat = pTypeFormat;
	pDeviceType->TypeCount = TypeCount;
	return hResult;
}

HRESULT MediaDeviceActivate(IMFActivate *pActivate, LPWSTR *pDeviceName, IMFMediaSource **ppSource, DeviceType *pDeviceType)
{
	LPWSTR DeviceName = NULL;
	UINT32 cchLength = 0;

	HRESULT hResult = IMFActivate_GetAllocatedString(pActivate, &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &DeviceName, &cchLength);
	if (FAILED(hResult))
	{
		return hResult;
	}

	IMFMediaSource *pSource = NULL;
	hResult = IMFActivate_ActivateObject(pActivate, &IID_IMFMediaSource, &pSource);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = MediaDeviceTypeCreate(pSource, pDeviceType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	*pDeviceName = DeviceName;
	*ppSource = pSource;
	return hResult;
}

HRESULT MediaDeviceSetCreate(MediaDeviceSet *pDeviceSet)
{
	IMFActivate **ppActivate = NULL;
	UINT32 ActivateCount = 0;

	HRESULT hResult = MediaEnumDeviceSources(&ppActivate, &ActivateCount);
	if (FAILED(hResult))
	{
		return hResult;
	}

	LPWSTR *pDeviceName = (LPWSTR*)HeapAlloc(GetProcessHeap(), 0UL, sizeof(LPWSTR) * ActivateCount);
	if (NULL == pDeviceName)
	{
		return S_FALSE;
	}

	IMFMediaSource **ppSource = (IMFMediaSource**)HeapAlloc(GetProcessHeap(), 0UL, sizeof(IMFMediaSource*) * ActivateCount);
	if (NULL == ppSource)
	{
		return S_FALSE;
	}

	DeviceType *pDeviceType = (DeviceType*)HeapAlloc(GetProcessHeap(), 0UL, sizeof(DeviceType) * ActivateCount);
	if (NULL == pDeviceType)
	{
		return S_FALSE;
	}

	for (UINT32 ActivateIndex = 0; ActivateIndex < ActivateCount; ActivateIndex++)
	{
		hResult = MediaDeviceActivate(ppActivate[ActivateIndex], &pDeviceName[ActivateIndex], &ppSource[ActivateIndex], &pDeviceType[ActivateIndex]);
		if (FAILED(hResult))
		{
			return hResult;
		}
		IMFActivate_Release(ppActivate[ActivateIndex]);
	}
	CoTaskMemFree(ppActivate);

	pDeviceSet->ppSource = ppSource;
	pDeviceSet->pDeviceName = pDeviceName;
	pDeviceSet->pDeviceType = pDeviceType;
	pDeviceSet->DeviceCount = ActivateCount;
	return hResult;
}

HRESULT MediaDeviceTypeRelease(DeviceType *pDeviceType)
{
	IMFMediaTypeHandler *pTypeHandler = pDeviceType->pTypeHandler;
	IMFMediaType **ppMediaType = pDeviceType->ppMediaType;
	TypeFrame *pTypeFrame = pDeviceType->pTypeFrame;
	LPWSTR *pTypeFormat = pDeviceType->pTypeFormat;
	UINT32 TypeCount = pDeviceType->TypeCount;

	BOOL Result = S_OK;
	for (UINT32 TypeIndex = 0; TypeIndex < TypeCount; TypeIndex++)
	{
		IMFMediaType_Release(ppMediaType[TypeIndex]);

		Result = HeapFree(GetProcessHeap(), 0UL, pTypeFormat[TypeIndex]);
		if (FALSE == Result)
		{
			return S_FALSE;
		}
	}

	IMFMediaTypeHandler_Release(pTypeHandler);

	Result = HeapFree(GetProcessHeap(), 0UL, ppMediaType);
	if (FALSE == Result)
	{
		return S_FALSE;
	}

	Result = HeapFree(GetProcessHeap(), 0UL, pTypeFrame);
	if (FALSE == Result)
	{
		return S_FALSE;
	}

	Result = HeapFree(GetProcessHeap(), 0UL, pTypeFormat);
	if (FALSE == Result)
	{
		return S_FALSE;
	}

	return S_OK;
}

HRESULT MediaDeviceSetRelease(MediaDeviceSet *pDeviceSet)
{
	LPWSTR *pDeviceName = pDeviceSet->pDeviceName;
	IMFMediaSource **ppSource = pDeviceSet->ppSource;
	DeviceType *pDeviceType = pDeviceSet->pDeviceType;
	UINT32 DeviceCount = pDeviceSet->DeviceCount;

	for (UINT32 DeviceIndex = 0; DeviceIndex < DeviceCount; DeviceIndex++)
	{
		CoTaskMemFree(pDeviceName[DeviceIndex]);

		HRESULT hResult = IMFMediaSource_Shutdown(ppSource[DeviceIndex]);
		if (FAILED(hResult))
		{
			return hResult;
		}
		IMFMediaSource_Release(ppSource[DeviceIndex]);

		hResult = MediaDeviceTypeRelease(&pDeviceType[DeviceIndex]);
		if (FAILED(hResult))
		{
			return hResult;
		}
	}

	BOOL Result = HeapFree(GetProcessHeap(), 0UL, pDeviceName);
	if (FALSE == Result)
	{
		return S_FALSE;
	}

	Result = HeapFree(GetProcessHeap(), 0UL, ppSource);
	if (FALSE == Result)
	{
		return S_FALSE;
	}

	Result = HeapFree(GetProcessHeap(), 0UL, pDeviceType);
	if (FALSE == Result)
	{
		return S_FALSE;
	}

	return S_OK;
}

void MediaDeviceSetName(MediaDeviceSet *pDeviceSet, LPWSTR **ppDeviceName, UINT32 *pDeviceCount)
{
	*ppDeviceName = pDeviceSet->pDeviceName;
	*pDeviceCount = pDeviceSet->DeviceCount;
}

void MediaDeviceSetTypeFormat(MediaDeviceSet *pDeviceSet, UINT32 DeviceIndex, LPWSTR **ppTypeFormat, UINT32 *pTypeCount)
{
	*ppTypeFormat = pDeviceSet->pDeviceType[DeviceIndex].pTypeFormat;
	*pTypeCount = pDeviceSet->pDeviceType[DeviceIndex].TypeCount;
}

HRESULT MediaDeviceSetSource(MediaDeviceSet *pDeviceSet, UINT32 DeviceIndex, UINT32 TypeIndex, IMFMediaSource **ppSource)
{
	IMFMediaTypeHandler *pTypeHandler = pDeviceSet->pDeviceType[DeviceIndex].pTypeHandler;
	IMFMediaType *pMediaType = pDeviceSet->pDeviceType[DeviceIndex].ppMediaType[TypeIndex];

	HRESULT hResult = IMFMediaTypeHandler_SetCurrentMediaType(pTypeHandler, pMediaType);
	if (FAILED(hResult))
	{
		return hResult;
	}

	*ppSource = pDeviceSet->ppSource[DeviceIndex];
	return hResult;
}



HRESULT MediaCreateActivate(IMFStreamDescriptor *pStreamDescriptor, HWND hwndVideo, IMFActivate **ppActivate)
{
	IMFMediaTypeHandler *pMediaTypeHandler = NULL;
	HRESULT hResult = IMFStreamDescriptor_GetMediaTypeHandler(pStreamDescriptor, &pMediaTypeHandler);
	if (FAILED(hResult))
	{
		return hResult;
	}

	GUID guidMajorType;
	hResult = IMFMediaTypeHandler_GetMajorType(pMediaTypeHandler, &guidMajorType);
	if (FAILED(hResult))
	{
		return hResult;
	}
	IMFMediaTypeHandler_Release(pMediaTypeHandler);

	IMFActivate *pActivate = NULL;
	if (TRUE == IsEqualGUID(&MFMediaType_Audio, &guidMajorType))
	{
		hResult = MFCreateAudioRendererActivate(&pActivate);
	}
	else if (TRUE == IsEqualGUID(&MFMediaType_Video, &guidMajorType))
	{
		hResult = MFCreateVideoRendererActivate(hwndVideo, &pActivate);
	}
	else
	{
		hResult = E_FAIL;
	}

	if (FAILED(hResult))
	{
		return hResult;
	}

	*ppActivate = pActivate;
	return hResult;
}

HRESULT MediaTopologyAddStream(IMFTopology *pTopology, IMFMediaSource *pSource, IMFPresentationDescriptor *pPresentationDescriptor, IMFStreamDescriptor *pStreamDescriptor, HWND hwndVideo)
{
	IMFTopologyNode *pSourceNode = NULL;
	HRESULT hResult = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pSourceNode);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFTopologyNode_SetUnknown(pSourceNode, &MF_TOPONODE_SOURCE, (IUnknown*)pSource);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFTopologyNode_SetUnknown(pSourceNode, &MF_TOPONODE_PRESENTATION_DESCRIPTOR, (IUnknown*)pPresentationDescriptor);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFTopologyNode_SetUnknown(pSourceNode, &MF_TOPONODE_STREAM_DESCRIPTOR, (IUnknown*)pStreamDescriptor);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFTopology_AddNode(pTopology, pSourceNode);
	if (FAILED(hResult))
	{
		return hResult;
	}

	IMFTopologyNode *pOutputNode = NULL;
	hResult = MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pOutputNode);
	if (FAILED(hResult))
	{
		return hResult;
	}

	IMFActivate *pActivate = NULL;
	hResult = MediaCreateActivate(pStreamDescriptor, hwndVideo, &pActivate);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFTopologyNode_SetObject(pOutputNode, (IUnknown*)pActivate);
	if (FAILED(hResult))
	{
		return hResult;
	}
	IMFActivate_Release(pActivate);

	hResult = IMFTopologyNode_SetUINT32(pOutputNode, &MF_TOPONODE_STREAMID, 0UL);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFTopologyNode_SetUINT32(pOutputNode, &MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFTopology_AddNode(pTopology, pOutputNode);
	if (FAILED(hResult))
	{
		return hResult;
	}

	hResult = IMFTopologyNode_ConnectOutput(pSourceNode, 0UL, pOutputNode, 0UL);
	if (FAILED(hResult))
	{
		return hResult;
	}

	IMFTopologyNode_Release(pSourceNode);
	IMFTopologyNode_Release(pOutputNode);
	return hResult;
}

HRESULT MediaSessionSetTopology(IMFMediaSession *pSession, IMFMediaSource *pSource, HWND hwndVideo)
{
	IMFTopology *pTopology = NULL;
	HRESULT hResult = MFCreateTopology(&pTopology);
	if (FAILED(hResult))
	{
		return hResult;
	}

	IMFPresentationDescriptor *pPresentationDescriptor = NULL;
	hResult = IMFMediaSource_CreatePresentationDescriptor(pSource, &pPresentationDescriptor);
	if (FAILED(hResult))
	{
		return hResult;
	}

	DWORD StreamCount = 0UL;
	hResult = IMFPresentationDescriptor_GetStreamDescriptorCount(pPresentationDescriptor, &StreamCount);
	if (FAILED(hResult))
	{
		return hResult;
	}

	for (DWORD StreamIndex = 0UL; StreamIndex < StreamCount; StreamIndex++)
	{
		BOOL fSelected = FALSE;
		IMFStreamDescriptor *pStreamDescriptor = NULL;
		hResult = IMFPresentationDescriptor_GetStreamDescriptorByIndex(pPresentationDescriptor, StreamIndex, &fSelected, &pStreamDescriptor);
		if (FAILED(hResult))
		{
			return hResult;
		}

		if (TRUE == fSelected)
		{
			hResult = MediaTopologyAddStream(pTopology, pSource, pPresentationDescriptor, pStreamDescriptor, hwndVideo);
			if (FAILED(hResult))
			{
				return hResult;
			}
		}
		IMFStreamDescriptor_Release(pStreamDescriptor);
	}
	IMFPresentationDescriptor_Release(pPresentationDescriptor);

	hResult = IMFMediaSession_SetTopology(pSession, 0UL, pTopology);
	if (FAILED(hResult))
	{
		return hResult;
	}
	IMFTopology_Release(pTopology);

	return hResult;
}

STDMETHODIMP_(HRESULT) Media_QueryInterface(MediaAsyncCallback *pAsyncCallback, REFIID riid, void **ppvObject);
STDMETHODIMP_(ULONG) Media_AddRef(MediaAsyncCallback *pAsyncCallback);
STDMETHODIMP_(ULONG) Media_Release(MediaAsyncCallback *pAsyncCallback);
STDMETHODIMP_(HRESULT) Media_GetParameters(MediaAsyncCallback *pAsyncCallback, DWORD *pdwFlags, DWORD *pdwQueue);
STDMETHODIMP_(HRESULT) Media_Invoke(MediaAsyncCallback *pAsyncCallback, IMFAsyncResult* pAsyncResult);
STDMETHODIMP_(HRESULT) Media_Start(MediaAsyncCallback *pAsyncCallback);
STDMETHODIMP_(HRESULT) Media_Shutdown(MediaAsyncCallback *pAsyncCallback);
STDMETHODIMP_(HRESULT) Media_Open(MediaAsyncCallback *pAsyncCallback, IMFMediaSource *pSource, HWND hwndVideo);
STDMETHODIMP_(HRESULT) Media_Repaint(MediaAsyncCallback *pAsyncCallback);
STDMETHODIMP_(HRESULT) Media_SetVideoPosition(MediaAsyncCallback *pAsyncCallback, WORD Width, WORD Height);

MediaAsyncCallbackVtbl MediaVtbl =
{
	Media_QueryInterface,
	Media_AddRef,
	Media_Release,
	Media_GetParameters,
	Media_Invoke,
	Media_Start,
	Media_Shutdown,
	Media_Open,
	Media_Repaint,
	Media_SetVideoPosition
}, *lpMediaVtbl = &MediaVtbl;

HRESULT MediaAsyncCallbackCreate(MediaAsyncCallback *pAsyncCallback)
{
	IMFMediaSession *pSession = NULL;
	HRESULT hResult = MFCreateMediaSession(NULL, &pSession);
	if (FAILED(hResult))
	{
		return hResult;
	}

	HANDLE CloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (NULL == CloseEvent)
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}

	pAsyncCallback->lpVtbl = lpMediaVtbl;
	pAsyncCallback->RefCount = 1UL;
	pAsyncCallback->pSession = pSession;
	pAsyncCallback->pDisplayControl = NULL;
	pAsyncCallback->CloseEvent = CloseEvent;

	hResult = IMFMediaSession_BeginGetEvent(pSession, (IMFAsyncCallback*)pAsyncCallback, NULL);
	if (FAILED(hResult))
	{
		return hResult;
	}

	return hResult;
}

STDMETHODIMP_(HRESULT) Media_QueryInterface(MediaAsyncCallback *pAsyncCallback, REFIID riid, void **ppvObject)
{
	if (NULL == pAsyncCallback || NULL == ppvObject)
	{
		return E_INVALIDARG;
	}

	if (FALSE == IsEqualIID(riid, &IID_IUnknown) ||
		FALSE == IsEqualIID(riid, &IID_IMFAsyncCallback))
	{
		return E_NOINTERFACE;
	}

	pAsyncCallback->lpVtbl->AddRef(pAsyncCallback);
	*ppvObject = pAsyncCallback;
	return NOERROR;
}

STDMETHODIMP_(ULONG) Media_AddRef(MediaAsyncCallback *pAsyncCallback)
{
	return InterlockedIncrement((LONG*)&pAsyncCallback->RefCount);
}

STDMETHODIMP_(ULONG) Media_Release(MediaAsyncCallback *pAsyncCallback)
{
	return InterlockedDecrement((LONG*)&pAsyncCallback->RefCount);
}

STDMETHODIMP_(HRESULT) Media_GetParameters(MediaAsyncCallback *pAsyncCallback, DWORD *pdwFlags, DWORD *pdwQueue)
{
	UNREFERENCED_PARAMETER(pAsyncCallback);
	UNREFERENCED_PARAMETER(pdwFlags);
	UNREFERENCED_PARAMETER(pdwQueue);
	return E_NOTIMPL;
}

STDMETHODIMP_(HRESULT) Media_Invoke(MediaAsyncCallback *pAsyncCallback, IMFAsyncResult* pAsyncResult)
{
	IMFMediaEvent *pEvent = NULL;
	HRESULT hResult = IMFMediaSession_EndGetEvent(pAsyncCallback->pSession, pAsyncResult, &pEvent);
	if (FAILED(hResult))
	{
		return hResult;
	}

	MediaEventType EventType = MEUnknown;
	hResult = IMFMediaEvent_GetType(pEvent, &EventType);
	if (FAILED(hResult))
	{
		IMFMediaEvent_Release(pEvent);
		return hResult;
	}

	MF_TOPOSTATUS TopologyStatus = MF_TOPOSTATUS_INVALID;
	switch (EventType)
	{
	case MESessionTopologyStatus:
		hResult = IMFMediaEvent_GetUINT32(pEvent, &MF_EVENT_TOPOLOGY_STATUS, (UINT32*)&TopologyStatus);
		if (FAILED(hResult))
		{
			IMFMediaEvent_Release(pEvent);
			return hResult;
		}

		switch (TopologyStatus)
		{
		case MF_TOPOSTATUS_READY:
			hResult = MFGetService((IUnknown*)pAsyncCallback->pSession, &MR_VIDEO_RENDER_SERVICE, &IID_IMFVideoDisplayControl, &pAsyncCallback->pDisplayControl);
			if (FAILED(hResult))
			{
				IMFMediaEvent_Release(pEvent);
				return hResult;
			}

			hResult = MediaAsyncCallback_Start(pAsyncCallback);
			if (FAILED(hResult))
			{
				IMFMediaEvent_Release(pAsyncCallback);
				return hResult;
			}
			break;
		}
		break;

	case MESessionClosed:
		IMFMediaEvent_Release(pEvent);
		return TRUE == SetEvent(pAsyncCallback->CloseEvent) ? S_OK : S_FALSE;
	}

	IMFMediaEvent_Release(pEvent);
	return IMFMediaSession_BeginGetEvent(pAsyncCallback->pSession, (IMFAsyncCallback*)pAsyncCallback, NULL);
}

STDMETHODIMP_(HRESULT) Media_Start(MediaAsyncCallback *pAsyncCallback)
{
	PROPVARIANT varStartPosition;
	PropVariantInit(&varStartPosition);

	HRESULT hResult = IMFMediaSession_Start(pAsyncCallback->pSession, &GUID_NULL, &varStartPosition);
	if (FAILED(hResult))
	{
		PropVariantClear(&varStartPosition);
		return hResult;
	}

	pAsyncCallback->State = Start;
	return PropVariantClear(&varStartPosition);
}

STDMETHODIMP_(HRESULT) Media_Shutdown(MediaAsyncCallback *pAsyncCallback)
{
	if (Close == pAsyncCallback->State)
	{
		return S_OK;
	}

	IMFVideoDisplayControl_Release(pAsyncCallback->pDisplayControl);
	HRESULT hResult = IMFMediaSession_Close(pAsyncCallback->pSession);
	if (FAILED(hResult))
	{
		return hResult;
	}
	if (WAIT_OBJECT_0 != WaitForSingleObject(pAsyncCallback->CloseEvent, INFINITE))
	{
		return HRESULT_FROM_WIN32(GetLastError());
	}
	if (FALSE == CloseHandle(pAsyncCallback->CloseEvent))
	{
		return S_FALSE;
	}

	hResult = IMFMediaSession_Shutdown(pAsyncCallback->pSession);
	if (FAILED(hResult))
	{
		return hResult;
	}
	IMFMediaSession_Release(pAsyncCallback->pSession);

	pAsyncCallback->State = Close;
	return S_OK;
}

STDMETHODIMP_(HRESULT) Media_Open(MediaAsyncCallback *pAsyncCallback, IMFMediaSource *pSource, HWND hwndVideo)
{
	return MediaSessionSetTopology(pAsyncCallback->pSession, pSource, hwndVideo);
}

STDMETHODIMP_(HRESULT) Media_Repaint(MediaAsyncCallback *pAsyncCallback)
{
	return IMFVideoDisplayControl_RepaintVideo(pAsyncCallback->pDisplayControl);
}

STDMETHODIMP_(HRESULT) Media_SetVideoPosition(MediaAsyncCallback *pAsyncCallback, WORD Width, WORD Height)
{
	RECT Rect = { 0L, 0L, Width, Height };
	return IMFVideoDisplayControl_SetVideoPosition(pAsyncCallback->pDisplayControl, NULL, &Rect);
}