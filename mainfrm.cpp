// MainFrm.cpp : implmentation of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "sanaView.h"
#include "MainFrm.h"

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;

	if(m_TreeView.PreTranslateMessage(pMsg))
		return TRUE;

	return m_tListView.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle()
{
	UIUpdateToolBar();
	UISetCheck(ID_VIEW_TREEVIEW, (m_vSplit.GetSinglePaneMode() == SPLIT_PANE_NONE));
	UISetCheck(ID_VIEW_LOGVIEW, (m_hSplit.GetSinglePaneMode() == SPLIT_PANE_NONE));

	return FALSE;
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// create command bar window
	HWND hWndCmdBar = m_CmdBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_CMDBAR_PANE_STYLE);
	// attach menu
	m_CmdBar.AttachMenu(GetMenu());
	// load command bar images
	m_CmdBar.LoadImages(IDR_MAINFRAME);
	// remove old menu
	SetMenu(NULL);

	//HWND hWndToolBar = CreateSimpleToolBarCtrl(m_hWnd, IDR_MAINFRAME, FALSE, ATL_SIMPLE_TOOLBAR_PANE_STYLE);
	HWND hWndToolBar = m_ToolBar.Create(m_hWnd, rcDefault, NULL, ATL_SIMPLE_TOOLBAR_PANE_STYLE | CCS_ADJUSTABLE | TBSTYLE_ALTDRAG, 0, ATL_IDW_TOOLBAR);
	InitToolBar(hWndToolBar, IDR_MAINFRAME);
	m_ToolBar.LoadToolBar(IDR_MAINFRAME);
	m_ToolBar.LoadMenuImages(IDR_MAINFRAME);
	m_ToolBar.AddDropDownButton(ID_FILE_OPEN, IDR_POPUP_OPEN);

	CreateSimpleReBar(MTL_SIMPLE_REBAR_STYLE | RBS_DBLCLKTOGGLE);
	AddSimpleReBarBand(hWndCmdBar);
	AddSimpleReBarBand(hWndToolBar, NULL, TRUE);

	CRect rc;
	rc.left = 0;
	rc.top = 0;
	rc.right = 300;
	rc.bottom = 300;

	HWND hSelectComboBox = m_SelectComboBox.Create(m_hWndToolBar, rc, NULL,
		WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
		CBS_AUTOHSCROLL | CBS_DROPDOWN,
		0,
		IDC_SELECTION_COMBO);
	m_SelectComboBox.SetFont(static_cast<HFONT>(::GetStockObject(DEFAULT_GUI_FONT)));
	m_SelectComboBox.AddString(_T(".txt$"));

	AddSimpleReBarBand(hSelectComboBox, _T("選択"));
	// RBBS_GRIPPERALWAYSがないとツールバーの固定時に表示が変になるっぽい。。。
	AddBandStyle(RBBS_GRIPPERALWAYS);
	SizeSimpleReBarBands();

	CreateSimpleStatusBar();

	// subclass the status bar as multipane
	m_StatusBar.SubclassWindow(m_hWndStatusBar);

	// set status bar panes
	int arrPanes[] = { ID_DEFAULT_PANE, IDR_INFO_PANE };
	m_StatusBar.SetPanes(arrPanes, sizeof(arrPanes) / sizeof(int), false);

	// set status bar pane widths using local workaround
	int arrWidths[] = { 0, 300 };
	SetPaneWidths(arrWidths, sizeof(arrWidths) / sizeof(int));
	//m_StatusBar.SetPaneWidth(IDR_INFO_PANE, 300);

	m_hWndClient = CreateClient();

	UIAddToolBar(hWndToolBar);
	UISetCheck(ID_VIEW_TOOLBAR, 1);
	UISetCheck(ID_VIEW_STATUS_BAR, 1);
	UISetCheck(ID_VIEW_SELECTION_BAR, 1);
	UISetCheck(ID_VIEW_TREEVIEW, 1);
	UISetCheck(ID_VIEW_LOGVIEW, 1);
	UpdateLayout();

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	return 0;
}

LRESULT CMainFrame::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
	HWND hWndChild = (HWND)wParam;
	POINT pt;
	pt.x = LOWORD(lParam);
	pt.y = HIWORD(lParam);

	if (hWndChild == m_hWndToolBar)
	{
		CMenu menu;
		menu.LoadMenu(IDR_MAINFRAME);
		CMenuHandle popup = menu.GetSubMenu(2);
		popup.TrackPopupMenu(TPM_LEFTALIGN, pt.x, pt.y, m_hWnd);
	}
	return 0;
}

HWND CMainFrame::CreateClient()
{
// vertical splitter setup
	// client rect for vertical splitter
	CRect rcVert;
	GetClientRect(&rcVert);

	// create the vertical splitter
	m_vSplit.Create(m_hWnd, rcVert, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	// set the vertical splitter parameters
	m_vSplit.m_cxyMin = 35; // minimum size
	m_vSplit.SetSplitterPos(rcVert.Width() / 4); // from left
	m_vSplit.m_bFullDrag = false; // ghost bar enabled

// horizontal splitter setup
	// client rect for horizontal splitter
	CRect rcHorz;
	GetClientRect(&rcHorz);

	// create the horizontal splitter. Note that vSplit is parent of hzSplit
	m_hSplit.Create(m_vSplit.m_hWnd, rcHorz, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);

	// set the horizontal splitter parameters
	m_hSplit.m_cxyMin = 35; // minimum size
	m_hSplit.SetSplitterPos(rcVert.Height() * 3 / 4); // from top
	m_hSplit.m_bFullDrag = false; // ghost bar enabled

	// add the horizontal splitter to right pane of vertical splitter
	m_vSplit.SetSplitterPane(SPLIT_PANE_RIGHT, m_hSplit);

// left pane container setup
	// create the left container
	m_lPane.Create(m_vSplit.m_hWnd);

	// add container to left pane (SPLIT_PANE_LEFT) of splitter
	m_vSplit.SetSplitterPane(SPLIT_PANE_LEFT, m_lPane);

	// set the left pane title
	m_lPane.SetTitle(_T("フォルダ"));

	m_TreeView.Create(m_lPane.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, WS_EX_STATICEDGE);
	m_lPane.SetClient(m_TreeView.m_hWnd);

// top list view setup
	// create a list view
	m_tListView.Create(m_hSplit.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS, WS_EX_STATICEDGE | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_FLATSB);

	// add list view to top pane (SPLIT_PANE_TOP) of horizontal splitter
	m_hSplit.SetSplitterPane(SPLIT_PANE_TOP, m_tListView.m_hWnd);

// bottom pane container setup
	// create the bottom container
	m_bPane.Create(m_hSplit.m_hWnd);

	// add container to bottom pane (SPLIT_PANE_BOTTOM) of horizontal splitter
	m_hSplit.SetSplitterPane(SPLIT_PANE_BOTTOM, m_bPane);

	// set the bottom container's header style to vertical
	m_bPane.SetPaneContainerExtendedStyle(PANECNT_VERTICAL);

	// create a list view
	m_bListView.Create(m_bPane.m_hWnd, rcDefault, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | LVS_REPORT | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS | LVS_NOSORTHEADER, WS_EX_STATICEDGE | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_FLATSB);

	// assign the edit to the bottom container
	m_bPane.SetClient(m_bListView.m_hWnd);

	SetColumn();

	return m_vSplit.m_hWnd;
}

LRESULT CMainFrame::OnPaneClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/)
{
	// hide the container whose Close button was clicked. Use 
	// DestroyWindow(hWndCtl) instead if you want to totally 
	// remove the container instead of just hiding it
	::ShowWindow(hWndCtl, SW_HIDE);

	// find the container's parent splitter
	HWND hWnd = ::GetParent(hWndCtl);
	CSplitterWindow* pWnd;
	pWnd = (CSplitterWindow*)::GetWindowLong(hWnd, GWL_ID);

	// take the container that was Closed out of the splitter.
	// Use SetSplitterPane(nPane, NULL) if you want to stay in
	// multipane mode instead of changing to single pane mode
	int nCount = pWnd->m_nPanesCount;
	for(int nPane = 0; nPane < nCount; nPane++)
	{
		if (hWndCtl == pWnd->m_hWndPane[nPane])
		{
			pWnd->SetSinglePaneMode(nCount - nPane - 1);
			break;
		}
	}

	return 0;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: add code to initialize document

	return 0;
}

LRESULT CMainFrame::OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	TCHAR filter[] = _T("すべての書庫(*.LZH;*.ZIP;*.CAB)\0")
		_T("*.lzh;*.zip;*.cab;\0")
		_T("すべてのファイル(*.*)\0")
		_T("*.*\0\0");

	CFileDialog fileDialog(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST, filter);

	fileDialog.DoModal();

	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(ATL_IDW_TOOLBAR);
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_TOOLBAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewSelectionBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bVisible = TRUE;	// initially visible
	bVisible = !bVisible;
	CReBarCtrl rebar = m_hWndToolBar;
	int nBandIndex = rebar.IdToIndex(IDC_SELECTION_COMBO);
	rebar.ShowBand(nBandIndex, bVisible);
	UISetCheck(ID_VIEW_SELECTION_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !::IsWindowVisible(m_hWndStatusBar);
	::ShowWindow(m_hWndStatusBar, bVisible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, bVisible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnViewTreeView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !(m_vSplit.GetSinglePaneMode() == SPLIT_PANE_NONE);
	m_vSplit.SetSinglePaneMode((bVisible ? SPLIT_PANE_NONE : SPLIT_PANE_RIGHT));
	return 0;
}

LRESULT CMainFrame::OnViewLogView(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	BOOL bVisible = !(m_hSplit.GetSinglePaneMode() == SPLIT_PANE_NONE);
	m_hSplit.SetSinglePaneMode((bVisible ? SPLIT_PANE_NONE : SPLIT_PANE_TOP));
	return 0;
}

LRESULT CMainFrame::OnViewCustomizeToolbar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	m_ToolBar.Customize();
	return 0;
}

LRESULT CMainFrame::OnViewToolbarFixed(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	static BOOL bFixed = FALSE;	// initially visible
	bFixed = !bFixed;
	SetReBarFixed(bFixed);
	UISetCheck(ID_VIEW_TOOLBAR_FIXED, bFixed);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

VOID CMainFrame::SetColumn()
{
	int i = 0;

	m_tListView.InsertColumn(i,	_T("名前"),			LVCFMT_LEFT,	120,	i++);
	m_tListView.InsertColumn(i,	_T("サイズ"),		LVCFMT_RIGHT,	70,		i++);
	m_tListView.InsertColumn(i,	_T("格納サイズ"),	LVCFMT_RIGHT,	70,		i++);
	m_tListView.InsertColumn(i,	_T("圧縮率"),		LVCFMT_RIGHT,	70,		i++);
	m_tListView.InsertColumn(i,	_T("種類"),			LVCFMT_LEFT,	150,	i++);
	m_tListView.InsertColumn(i,	_T("更新日時"),		LVCFMT_LEFT,	120,	i++);
	m_tListView.InsertColumn(i,	_T("パス"),			LVCFMT_LEFT,	150,	i++);

	i = 0;

	m_bListView.InsertColumn(i,	_T("状態"),			LVCFMT_LEFT,	120,	i++);
	m_bListView.InsertColumn(i,	_T("日時"),			LVCFMT_LEFT,	120,	i++);
	m_bListView.InsertColumn(i,	_T("情報"),			LVCFMT_LEFT,	300,	i++);
}

// this workaround solves a bug in CMultiPaneStatusBarCtrl
// (in SetPanes() method) that limits the size of all panes
// after the default pane to a combined total of 100 pixels  
void CMainFrame::SetPaneWidths(int* arrWidths, int nPanes)
{
	// find the size of the borders
	int arrBorders[3];
	m_StatusBar.GetBorders(arrBorders);

	// calculate right edge of default pane
	arrWidths[0] += arrBorders[2];
	for (int i = 1; i < nPanes; i++)
		arrWidths[0] += arrWidths[i];

	// calculate right edge of remaining panes
	for (int j = 1; j < nPanes; j++)
		arrWidths[j] += arrBorders[2] + arrWidths[j - 1];

	// set the pane widths
	m_StatusBar.SetParts(m_StatusBar.m_nPanes, arrWidths);
}

// ツールバーを固定する
#if (_WIN32_IE >= 0x0400)
void CMainFrame::SetReBarFixed(BOOL bFixed)
{
	ATLASSERT(::IsWindow(m_hWndToolBar));	// must be an existing rebar

	int nCount = (int)::SendMessage(m_hWndToolBar, RB_GETBANDCOUNT, 0, 0L);

	for (int i = 0; i < nCount; i++)
	{
		REBARBANDINFO rbBand;
		rbBand.cbSize = sizeof(REBARBANDINFO);
		rbBand.fMask = RBBIM_STYLE;
		BOOL bRet = (BOOL)::SendMessage(m_hWndToolBar, RB_GETBANDINFO, i, (LPARAM)&rbBand);
		ATLASSERT(bRet);
		if (bFixed)
		{
			rbBand.fStyle |= RBBS_NOGRIPPER;
			rbBand.fStyle &= ~RBBS_GRIPPERALWAYS;
		}
		else
		{
			rbBand.fStyle |= RBBS_GRIPPERALWAYS;
			rbBand.fStyle &= ~RBBS_NOGRIPPER;
		}
		bRet = (BOOL)::SendMessage(m_hWndToolBar, RB_SETBANDINFO, i, (LPARAM)&rbBand);
		ATLASSERT(bRet);
	}
}
#endif //(_WIN32_IE >= 0x0400)

void CMainFrame::AddBandStyle(UINT fStyle)
{
	ATLASSERT(::IsWindow(m_hWndToolBar));	// must be an existing rebar

	int nCount = (int)::SendMessage(m_hWndToolBar, RB_GETBANDCOUNT, 0, 0L);

	for (int i = 0; i < nCount; i++)
	{
		REBARBANDINFO rbBand;
		rbBand.cbSize = sizeof(REBARBANDINFO);
		rbBand.fMask = RBBIM_STYLE;
		BOOL bRet = (BOOL)::SendMessage(m_hWndToolBar, RB_GETBANDINFO, i, (LPARAM)&rbBand);
		ATLASSERT(bRet);
		rbBand.fStyle |= fStyle;
		bRet = (BOOL)::SendMessage(m_hWndToolBar, RB_SETBANDINFO, i, (LPARAM)&rbBand);
		ATLASSERT(bRet);
	}
}
