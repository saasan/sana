// sana.cpp : main source file for sana.exe
//

#include "stdafx.h"

#include "resource.h"

#include "sanaView.h"
#include "aboutdlg.h"
#include "MainFrm.h"

CAppModule _Module;

int Run(LPTSTR /*lpstrCmdLine*/ = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;

	if(wndMain.CreateEx() == NULL)
	{
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	typedef BOOL (WINAPI *PCCL)();

	HINSTANCE hCCLDll;	// CCL.DLLÇÃÉnÉìÉhÉã

	HRESULT hRes = ::CoInitialize(NULL);
// If you are running on NT 4.0 or higher you can use the following call instead to 
// make the EXE free threaded. This means that calls come in on a random RPC thread.
//	HRESULT hRes = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	ATLASSERT(SUCCEEDED(hRes));

	// this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
	::DefWindowProc(NULL, 0, 0, 0L);

	AtlInitCommonControls(ICC_COOL_CLASSES | ICC_BAR_CLASSES);	// add flags to support other controls

	// CoolControlLibrary
	hCCLDll = LoadLibrary(_T("CCL.DLL"));
	if (hCCLDll)
	{
		PCCL fpInstall = (PCCL)GetProcAddress(hCCLDll, "InstallHook");
		if (fpInstall)
		{
			(*fpInstall)();
		}
	}

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	// CoolControlLibrary Hookâèú
	if (hCCLDll)
	{
		PCCL fpUninstall = (PCCL)GetProcAddress(hCCLDll, "UninstallHook");
		if (fpUninstall)
		{
			(*fpUninstall)();
		}
		FreeLibrary(hCCLDll);
	}

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
