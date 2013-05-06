// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#pragma once

#include "WindowHelper.h"
#include "KeystrokeMessages.h"

class CSwitchListener;
class CSwitchMsgWindow;

typedef struct SwitchListenerPtr {
	CSwitchMsgWindow* p;
} SWITCH_LISTENER_PTR, *P_SWITCH_LISTENER_PTR;


class CSwitchMsgWindow
{
public:
	CSwitchMsgWindow(MultiCamFilter *parent);
	virtual ~CSwitchMsgWindow(void);
protected:
	static DWORD WINAPI CSwitchMsgWindow::doMsgLoopInThread(LPVOID lpParam);
	DWORD WINAPI CSwitchMsgWindow::NonStaticDoMsgLoopInThread();
	static LRESULT APIENTRY CSwitchMsgWindow::WndProc(
		HWND hWindow, UINT uiMessage, WPARAM uiParam, LPARAM ulParam);
	LRESULT APIENTRY CSwitchMsgWindow::nonstatic_WndProc(
		HWND hWindow, UINT uiMessage, WPARAM uiParam, LPARAM ulParam);
	P_SWITCH_LISTENER_PTR CSwitchMsgWindow::AllocateThisPointer();
	void CSwitchMsgWindow::SendDiscoverMessage();
	void CSwitchMsgWindow::SendPongMessage();


	HANDLE m_hMsgLoopThread;
	DWORD m_dwMsgLoopThreadID;
	HANDLE m_evSentDiscoverMessage;
	HWND m_hMainWindow;
	HINSTANCE m_hProcess;
	char m_sWindowClassName[WINDOW_CLASS_NAME_LENGTH];
	HANDLE m_evMulticamAccepted;
	UINT m_uiDiscoverMessageId;
	UINT m_uiAttachMessageId;
	UINT m_uiAdvanceMessageId;
	UINT m_uiKickMessageId;
	UINT m_uiPingMessageId;
	UINT m_uiPongMessageId;
	UINT m_uiResetMessageId;
	Multicam_keystroke_API m_Multicam_keystroke_status;
	HWND m_hMulticamWindow;

	MultiCamFilter *m_pParent;
};

