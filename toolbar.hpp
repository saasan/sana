// toolbar.hpp: interface for the CToolBarCtrlEx class.
//
/////////////////////////////////////////////////////////////////////////////
// Tool Bar Control Extention 
//
// Version: 1.0
// Created: April 25, 2000
//  Author: BenBurnett
//   Email:	BenBurnett@telusplanet.net
//
// Copyright (C) 2000 Ben Burnett
// All rights reserved.
//
// Thanks to Paul DiLascia for the code to add drop-down 
//  menus to toolbar buttons. (1997 MSJ)
//
// Owner-draw code for menu from, CCommandBarCtrl in the
//  WTL. See AtlCtrlw.h...
//
// The code and information is provided "as-is" without
//  warranty of any kind, either expressed or implied.
//
// Bug fixes and suggestions are welcomed, contact me 
//  at the email above.
/////////////////////////////////////////////////////////////////////////////

#if !defined(_TOOLBARCTRLEX_H_INCLUDED_)
#define _TOOLBARCTRLEX_H_INCLUDED_

#pragma once

#ifndef __cplusplus
	#error toolbar.h requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLAPP_H__
	#error toolbar.h requires atlapp.h to be included first
#endif

#ifndef __ATLCTRLS_H__
#pragma message("toolbar.h requires atlctrls.h to be included first.")
#pragma message(" Once you include it you will not get this message.")
#include <atlctrls.h>
#endif

#if (_WIN32_IE < 0x0500)
	#error toolbar.h requires _WIN32_IE >= 0x0500
#endif

/////////////////////////////////////////////////////////////////////////////
// CToolBarCtrlExImpl class

template <class T, class TBase = CToolBarCtrl, class TWinTraits = CControlWinTraits>
class CToolBarCtrlExImpl : public CWindowImpl<T, TBase, TWinTraits>
{
public:
	DECLARE_WND_SUPERCLASS(NULL, TBase::GetWndClassName())

/////////////////////////////////////////////////////////////////////////////
// Declarations
//
	struct _MenuItemData	// menu item data
	{
		DWORD dwMagic;
		LPTSTR lpstrText;
		UINT fType;
		UINT fState;
		int iButton;

		_MenuItemData() { dwMagic = 0x1908; }
		bool IsCmdBarMenuItem() { return (dwMagic == 0x1908); }
	};
	
	struct _ToolBarData		// toolbar resource data
	{ 
		WORD wVersion;
		WORD wWidth;
		WORD wHeight;
		WORD wItemCount;
		//WORD aItems[wItemCount]

		WORD* items()
			{ return (WORD*)(this + 1); }
	};	

	struct _DropDownButton	// One of these for each drop-down button
	{
		_DropDownButton* next;
		UINT uIdButton;
		UINT uIdMenu;
	};

// Constants
	enum _ToolBarCtrlExDrawConstants
	{
		s_kcxGap = 1,
		s_kcxTextMargin = 2,
		s_kcxButtonMargin = 3,
		s_kcyButtonMargin = 3
	};

	enum { _nMaxMenuItemTextLength = 100 };
	
/////////////////////////////////////////////////////////////////////////////
// Data members

	CContainedWindow m_wndParent;
	_DropDownButton* m_pDropDownButtons;

	CImageList m_ImageList;
	CSimpleValArray<WORD> m_arrCommand;	
	CSimpleStack<HMENU> m_stackMenuHandle;
	
	bool m_bImagesVisible;
	bool m_bAnimate;
		
	SIZE m_szBitmap;
	SIZE m_szButton;
	
	COLORREF m_clrMask;
	CFont m_fontMenu;

/////////////////////////////////////////////////////////////////////////////
// Constructor/Destructor

	CToolBarCtrlExImpl() : 
		m_wndParent(this, 1),
		m_ImageList(NULL),
		m_pDropDownButtons(NULL),
		m_bImagesVisible(false),
		m_bAnimate(true),		
		m_clrMask(RGB(192, 192, 192))
	{
		m_szBitmap.cx = 16;
		m_szBitmap.cy = 15;
		m_szButton.cx = m_szBitmap.cx + s_kcxButtonMargin;
		m_szButton.cy = m_szBitmap.cy + s_kcyButtonMargin;	

#if (WINVER >= 0x0500) // Win2K only
		// Check if menu animation is turned off
		::SystemParametersInfo(SPI_GETMENUANIMATION, 0, &m_bAnimate, 0);
#endif
	}

	~CToolBarCtrlExImpl()
	{
		if (m_wndParent.IsWindow())
			m_wndParent.UnsubclassWindow(); // scary!

		while (m_pDropDownButtons) 
		{
			_DropDownButton* pNext = m_pDropDownButtons->next;
			delete m_pDropDownButtons;
			m_pDropDownButtons = pNext;
		}

		if (!m_ImageList.IsNull())
			m_ImageList.Destroy();
	}

/////////////////////////////////////////////////////////////////////////////
// Attributes

	COLORREF GetImageMaskColor() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return m_clrMask;
	}

	COLORREF SetImageMaskColor(COLORREF clrMask)
	{
		ATLASSERT(::IsWindow(m_hWnd));		
		COLORREF clrOld = m_clrMask;
		m_clrMask = clrMask;		
		return clrOld;
	}
	
	bool GetAnimate() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return m_bAnimate;
	}

	bool SetAnimate(bool bAnimate = true)
	{
		ATLASSERT(::IsWindow(m_hWnd));		
		bool bOld = m_bAnimate;
		m_bAnimate = bAnimate;
		return bOld;
	}
	
	bool GetImagesVisible() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return m_bImagesVisible;
	}

	bool SetImagesVisible(bool bVisible = true)
	{
		ATLASSERT(::IsWindow(m_hWnd));		
		bool bOld = m_bImagesVisible;
		m_bImagesVisible = bVisible;
		return bOld;
	}

	void GetImageSize(SIZE& size) const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		size = m_szBitmap;
	}

	bool SetImageSize(SIZE& size)
	{
		ATLASSERT(::IsWindow(m_hWnd));

		if (!m_ImageList.IsNull())
		{
			if (m_ImageList.GetImageCount() == 0)	// empty
				m_ImageList.Destroy();
			else
				return false;		// can't set, image list exists			
		}

		if (size.cx == 0 || size.cy == 0)
			return false;

		m_szBitmap = size;
		m_szButton.cx = m_szBitmap.cx + s_kcxButtonMargin;
		m_szButton.cy = m_szBitmap.cy + s_kcyButtonMargin;

		return true;
	}	

/////////////////////////////////////////////////////////////////////////////
// Methods

	// Load the toolbar resource
	BOOL LoadToolBar(UINT uResourceID, BOOL bInitialSeparator = FALSE)
	{
		ATLASSERT(IS_INTRESOURCE(uResourceID));
		
		HINSTANCE hInst = _Module.GetResourceInstance();		
		HRSRC hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(uResourceID), RT_TOOLBAR);
				
		if (hRsrc == NULL)
			return FALSE;

		HGLOBAL hGlobal = ::LoadResource(hInst, hRsrc);
		
		if (hGlobal == NULL)
			return FALSE;

		_ToolBarData* pData = (_ToolBarData*)::LockResource(hGlobal);
		
		if (pData == NULL)
			return FALSE;

		ATLASSERT(pData->wVersion == 1);

		WORD* pItems = pData->items();
		int nItems = pData->wItemCount + (bInitialSeparator ? 1 : 0);
		TBBUTTON* pTBBtn = (TBBUTTON*)_alloca(nItems * sizeof(TBBUTTON));

		// set initial separator (half width)
		if (bInitialSeparator)
		{
			pTBBtn[0].iBitmap = 4;
			pTBBtn[0].idCommand = 0;
			pTBBtn[0].fsState = 0;
			pTBBtn[0].fsStyle = TBSTYLE_SEP;
			pTBBtn[0].dwData = 0;
			pTBBtn[0].iString = 0;
		}

		int nBmp = 0;
		for (int i = 0, j = bInitialSeparator ? 1 : 0; i < pData->wItemCount; i++, j++)
		{
			if (pItems[i] != 0)
			{
				// Toolbar item
				pTBBtn[j].iBitmap = nBmp++;
				pTBBtn[j].idCommand = pItems[i];
				pTBBtn[j].fsState = TBSTATE_ENABLED;			
				pTBBtn[j].fsStyle = TBSTYLE_BUTTON;
				pTBBtn[j].dwData = 0;
				pTBBtn[j].iString = 128;
			}
			else
			{
				// Separator
				pTBBtn[j].iBitmap = 0;
				pTBBtn[j].idCommand = 0;
				pTBBtn[j].fsState = 0;
				pTBBtn[j].fsStyle = TBSTYLE_SEP;
				pTBBtn[j].dwData = 0;
				pTBBtn[j].iString = 0;
			}
		}

		// Set default button structure size
		SetButtonStructSize();
		
		// Add the bitmaps from the resource ID and then add the buttons
		if (AddBitmap(nBmp, uResourceID) == -1 || !AddButtons(nItems, pTBBtn))
		{
			free(pTBBtn); // Only free this if not used
			return FALSE;
		}		
				
		// Set bitmap and button sizes
		SIZE szBitmap, szButton;
		szBitmap.cx = pData->wWidth;
		szBitmap.cy = pData->wHeight;
		szButton.cx = szBitmap.cx + 7;
		szButton.cy = szBitmap.cy + 7;

		if (!SetBitmapSize(szBitmap) || !SetButtonSize(szButton))
			return FALSE;
		
		return TRUE;
	}

	// Load the drop-down menu bitmaps. Actually a toolbar resource
	BOOL LoadMenuImages(_U_STRINGorID image)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		HINSTANCE hInstance = _Module.GetResourceInstance();

		HRSRC hRsrc = ::FindResource(hInstance, image.m_lpstr, (LPTSTR)RT_TOOLBAR);

		if (hRsrc == NULL)
			return FALSE;

		HGLOBAL hGlobal = ::LoadResource(hInstance, hRsrc);

		if (hGlobal == NULL)
			return FALSE;

		_ToolBarData* pData = (_ToolBarData*)::LockResource(hGlobal);

		if (pData == NULL)
			return FALSE;

		ATLASSERT(pData->wVersion == 1);

		WORD* pItems = pData->items();
		int nItems = pData->wItemCount;

		// Add bitmap to our image list (create it if it doesn't exist)
		if (m_ImageList.IsNull())
		{
			if (!m_ImageList.Create(pData->wWidth, pData->wHeight, ILC_COLOR | ILC_MASK, pData->wItemCount, 1))
				return FALSE;
		}

		CBitmap bmp;
		bmp.LoadBitmap(image.m_lpstr);
		ATLASSERT(bmp.m_hBitmap != NULL);
		
		if (bmp.IsNull())
			return FALSE;

		if (m_ImageList.Add(bmp, m_clrMask) == -1)
			return FALSE;

		// Fill the array with command IDs
		for (int i = 0; i < nItems; i++)
		{
			if (pItems[i] != 0)
				m_arrCommand.Add(pItems[i]);
		}

		ATLASSERT(m_ImageList.GetImageCount() == m_arrCommand.GetSize());

		if (m_ImageList.GetImageCount() != m_arrCommand.GetSize())
			return FALSE;

		// Set some internal stuff
		m_szBitmap.cx = pData->wWidth;
		m_szBitmap.cy = pData->wHeight;
		m_szButton.cx = m_szBitmap.cx + 2 * s_kcxButtonMargin;
		m_szButton.cy = m_szBitmap.cy + 2 * s_kcyButtonMargin;

		// Show the loaded images
		SetImagesVisible(true);

		return TRUE;
	}

	// 1997 Microsoft Systems Journal - Written by Paul DiLascia.
	// call to add drop-down menus to toolbar buttons.
	//
	// Modified so that it will work in WTL
	//
	BOOL AddDropDownButton(UINT nIDButton, UINT nIDMenu, BOOL bArrow = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));

		_DropDownButton* pb = FindDropDownButton(nIDButton);
		
		if (!pb) 
		{
			pb = new _DropDownButton;
			ATLASSERT(pb != NULL);
			pb->next = m_pDropDownButtons;
			m_pDropDownButtons = pb;
		}

		pb->uIdButton = nIDButton;
		pb->uIdMenu   = nIDMenu;

		int nIndex = CommandToIndex(nIDButton);
		ATLASSERT(nIndex != -1);
		
		// Add drop-down style to the button
		TBBUTTON tbb;		
		GetButton(nIndex, &tbb);
		tbb.fsStyle |= BTNS_DROPDOWN;
		_SetButton(nIndex, &tbb);
		
		return TRUE;
	}	
	
	// 1997 Microsoft Systems Journal - Written by Paul DiLascia.
	// Find buttons structure for given ID
	_DropDownButton* FindDropDownButton(UINT nID)
	{
		for (_DropDownButton* pb = m_pDropDownButtons; pb; pb = pb->next) 
		{
			if (pb->uIdButton == nID)
				return pb;
		}

		return NULL;
	}

	// Set new button information
	void _SetButton(int nIndex, LPTBBUTTON lpButton)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		
		// Get original button state
		TBBUTTON button;
		GetButton(nIndex, &button);

		// Nothing to do if they are the same
		if (memcmp(lpButton, &button, sizeof(TBBUTTON)) != 0)
		{
			// Don't redraw everything while setting the button
			ModifyStyle(WS_VISIBLE, 0);
			DeleteButton(nIndex);
			InsertButton(nIndex, lpButton);
			ModifyStyle(0, WS_VISIBLE);

			// Invalidate appropriate parts
			if (((lpButton->fsStyle ^ button.fsStyle) & TBSTYLE_SEP) ||
				((lpButton->fsStyle & TBSTYLE_SEP) && lpButton->iBitmap != button.iBitmap))
			{
				// Changing a separator
				Invalidate();				
			}
			else
			{
				// Invalidate just the button
				RECT rc;
				GetItemRect(nIndex, &rc);
				InvalidateRect(&rc);
			}
		}
	}

	
/////////////////////////////////////////////////////////////////////////////
// Virtual Methods

	virtual bool DoDropDownMenu(UINT uIdMenu, RECT* rc)
	{
		CMenu menu;
		CMenuHandle menuPopup;
		menu.LoadMenu(uIdMenu);
		menuPopup = menu.GetSubMenu(0);			
			
		UINT uFlags = TPM_LEFTBUTTON | TPM_VERTICAL | TPM_LEFTALIGN | TPM_TOPALIGN;

#if (_WIN32_WINNT >= 0x0500) // Win2K can flag 'no animation'
		uFlags |= (m_bAnimate ? TPM_VERPOSANIMATION : TPM_NOANIMATION);
#else
		if (m_bAnimate) 
			uFlags |= TPM_VERPOSANIMATION;
#endif
		if (!menuPopup.TrackPopupMenu(uFlags, rc->left, rc->bottom, m_hWnd, rc))
			return false;

		return true;
	}


/////////////////////////////////////////////////////////////////////////////
// Message map and handlers

	BEGIN_MSG_MAP(CToolBarCtrlExImpl)		
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitMenuPopup)
		MESSAGE_HANDLER(WM_MENUSELECT, OnMenuSelect)		
		MESSAGE_HANDLER(WM_DRAWITEM, OnDrawItem)
		MESSAGE_HANDLER(WM_MEASUREITEM, OnMeasureItem)		
	ALT_MSG_MAP(1)		// Parent window messages	
		NOTIFY_CODE_HANDLER(TBN_DROPDOWN, OnParentDropDown)
		//NOTIFY_CODE_HANDLER(TBN_HOTITEMCHANGE, OnParentHotItemChange)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		// These styles are required
		ModifyStyle(0, TBSTYLE_FLAT);

		// Add drop-down arrows to the toolbar
		SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

#if (_WIN32_IE >= 0x0501)
		// Add some cool Win2K styles
		SetExtendedStyle(GetExtendedStyle() | TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_MIXEDBUTTONS);
#endif // (_WIN32_IE >= 0x0501)
		
		// Let the toolbar initialize itself
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
		
		// Get font settings from the system
		GetSystemSettings();
		
		// Subclass parent window
		CWindow wndParent = GetParent();
		CWindow wndTopLevelParent = wndParent.GetTopLevelParent();
		m_wndParent.SubclassWindow(wndTopLevelParent);		
		
		return lRet;	
	}
	
	LRESULT OnInitMenuPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		// System menu, do nothing
		if ((BOOL)HIWORD(lParam))
		{
			bHandled = FALSE;
			return 1;
		}

		// Forward to the parent or subclassed window, so it can handle update UI
		if (m_wndParent.IsWindow())
			m_wndParent.SendMessage(uMsg, wParam, lParam);
		
		// Convert menu items to ownerdraw, add our data
		if (m_bImagesVisible)
		{
			CMenuHandle menuPopup = (HMENU)wParam;
			ATLASSERT(menuPopup.m_hMenu != NULL);

			BOOL bRet;
			TCHAR szString[_nMaxMenuItemTextLength];
			for (int i = 0; i < menuPopup.GetMenuItemCount(); i++)
			{
				CMenuItemInfo mii;
				mii.cch = _nMaxMenuItemTextLength;
				mii.fMask = MIIM_CHECKMARKS | MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_SUBMENU | MIIM_TYPE;
				mii.dwTypeData = szString;

				bRet = menuPopup.GetMenuItemInfo(i, TRUE, &mii);
				ATLASSERT(bRet);

				if (!(mii.fType & MFT_OWNERDRAW))	// Not already an ownerdraw item
				{
					mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE;
					_MenuItemData* pMI = NULL;
					
					ATLTRY(pMI = new _MenuItemData);
					ATLASSERT(pMI != NULL);

					if (pMI != NULL)
					{
						pMI->fType = mii.fType;
						pMI->fState = mii.fState;
						mii.fType |= MFT_OWNERDRAW;
						pMI->iButton = -1;

						for (int j = 0; j < m_arrCommand.GetSize(); j++)
						{
							if (m_arrCommand[j] == mii.wID)
							{
								pMI->iButton = j;
								break;
							}
						}

						pMI->lpstrText = NULL;
						
						ATLTRY(pMI->lpstrText = new TCHAR[lstrlen(szString) + 1]);
						ATLASSERT(pMI->lpstrText != NULL);
						
						if (pMI->lpstrText != NULL)
							lstrcpy(pMI->lpstrText, szString);

						mii.dwItemData = (ULONG_PTR)pMI;

						bRet = menuPopup.SetMenuItemInfo(i, TRUE, &mii);
						ATLASSERT(bRet);
					}
				}
			}

			// Add it to the list
			m_stackMenuHandle.Push(menuPopup.m_hMenu);
		}

		return 0;
	}

	LRESULT OnMenuSelect(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		// Forward to the parent or subclassed window, so it can handle update UI
		if (m_wndParent.IsWindow())
			m_wndParent.SendMessage(uMsg, wParam, lParam);
		
		// Check if a menu is closing, do a cleanup
		if (HIWORD(wParam) == 0xFFFF && lParam == NULL)	// Menu closing
		{
			// Restore the menu items to the previous state for all menus that were converted
			if (m_bImagesVisible)
			{
				HMENU hMenu;
				while ((hMenu = m_stackMenuHandle.Pop()) != NULL)
				{
					CMenuHandle menuPopup = hMenu;
					ATLASSERT(menuPopup.m_hMenu != NULL);
					
					// Restore state and delete menu item data
					BOOL bRet;
					for(int i = 0; i < menuPopup.GetMenuItemCount(); i++)
					{
						CMenuItemInfo mii;
						mii.fMask = MIIM_DATA | MIIM_TYPE;
						bRet = menuPopup.GetMenuItemInfo(i, TRUE, &mii);
						ATLASSERT(bRet);

						_MenuItemData* pMI = (_MenuItemData*)mii.dwItemData;
						ATLASSERT(pMI != NULL);

						if (pMI != NULL && pMI->IsCmdBarMenuItem())
						{
							mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_STATE;
							mii.fType = pMI->fType;
							mii.dwTypeData = pMI->lpstrText;
							mii.cch = lstrlen(pMI->lpstrText);
							mii.dwItemData = NULL;

							bRet = menuPopup.SetMenuItemInfo(i, TRUE, &mii);
							ATLASSERT(bRet);

							delete [] pMI->lpstrText;
							pMI->dwMagic = 0x6666;
							delete pMI;
						}
					}
				}
			}
		}

		bHandled = FALSE;
		return 1;
	}

	// Draws menu items
	LRESULT OnDrawItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		LPDRAWITEMSTRUCT lpDrawItemStruct = (LPDRAWITEMSTRUCT)lParam;
		_MenuItemData* pmd = (_MenuItemData*)lpDrawItemStruct->itemData;
		
		if (lpDrawItemStruct->CtlType == ODT_MENU && pmd->IsCmdBarMenuItem())
			DrawMenuItem(lpDrawItemStruct);
		else
			bHandled = FALSE;

		return TRUE;
	}

	// Helps draw menu items
	LRESULT OnMeasureItem(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
	{
		LPMEASUREITEMSTRUCT lpMeasureItemStruct = (LPMEASUREITEMSTRUCT)lParam;
		_MenuItemData* pmd = (_MenuItemData*)lpMeasureItemStruct->itemData;
				
		if (lpMeasureItemStruct->CtlType == ODT_MENU && pmd->IsCmdBarMenuItem())
			MeasureMenuItem(lpMeasureItemStruct);
		else
			bHandled = FALSE;

		return TRUE;
	}	

/////////////////////////////////////////////////////////////////////////////
// Parent window message handlers

	LRESULT OnParentDropDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		LPNMTOOLBAR lpnmtb = (LPNMTOOLBAR)pnmh;	
		
		// Check if this is from us
		if (pnmh->hwndFrom != m_hWnd)
		{
			bHandled = FALSE;
			return TBDDRET_NODEFAULT;
		}	
		
		_DropDownButton* pb = FindDropDownButton(lpnmtb->iItem);
		
		RECT rc;
		GetRect(pb->uIdButton, &rc);
		ClientToScreen(&rc);		
		
		if (pb && pb->uIdMenu)
		{
			if (DoDropDownMenu(pb->uIdMenu, &rc))
			{
				return TBDDRET_DEFAULT;
			}
		}

		return TBDDRET_NODEFAULT;
	}

	// Do not hot track if application in background
	LRESULT OnParentHotItemChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
	{
		LPNMTBHOTITEM lpnmhi = (LPNMTBHOTITEM)pnmh;
		
		// Check if this is from us
		if (pnmh->hwndFrom != m_hWnd)
		{
			bHandled = FALSE;
			return 0;
		}		
		
		DWORD dwProcessID;
		::GetWindowThreadProcessId(::GetActiveWindow(), &dwProcessID);		
		if ((!m_wndParent.IsWindowEnabled() || ::GetCurrentProcessId() != dwProcessID))
			return 1;
		else
		{
			bHandled = FALSE;
			return 0;
		}
	}

/////////////////////////////////////////////////////////////////////////////
// Implementation 

	// Implementation - ownerdraw overrideables and helpers
	void DrawMenuItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
	{
		_MenuItemData* pmd = (_MenuItemData*)lpDrawItemStruct->itemData;
		CDCHandle dc = lpDrawItemStruct->hDC;
		const RECT& rcItem = lpDrawItemStruct->rcItem;

		if (pmd->fType & MFT_SEPARATOR)
		{
			// Draw separator
			RECT rc = rcItem;
			rc.top += (rc.bottom - rc.top) / 2;	// Vertical center
			dc.DrawEdge(&rc, EDGE_ETCHED, BF_TOP);	// Draw separator line
		}
		else
		{
			// Not a separator
			BOOL bDisabled = lpDrawItemStruct->itemState & ODS_GRAYED;
			BOOL bSelected = lpDrawItemStruct->itemState & ODS_SELECTED;
			BOOL bChecked = lpDrawItemStruct->itemState & ODS_CHECKED;
			BOOL bHasImage = FALSE;

			if (LOWORD(lpDrawItemStruct->itemID) == (WORD)-1)
				bSelected = FALSE;

			RECT rcButn = // button rect
			{ 
				rcItem.left, 
				rcItem.top, 
				rcItem.left + m_szButton.cx, 
				rcItem.top + m_szButton.cy 
			};

			::OffsetRect(&rcButn, 0, ((rcItem.bottom - rcItem.top) - (rcButn.bottom - rcButn.top)) / 2);	// center vertically

			int iButton = pmd->iButton;
			if (iButton >= 0)
			{
				bHasImage = TRUE;

				// Calc drawing point
				SIZE sz = 
				{ 
					rcButn.right - rcButn.left - m_szBitmap.cx, 
					rcButn.bottom - rcButn.top - m_szBitmap.cy 
				};

				sz.cx /= 2;
				sz.cy /= 2;

				POINT point = 
				{ 
					rcButn.left + sz.cx, 
					rcButn.top + sz.cy 
				};

				// Draw disabled or normal
				if (!bDisabled)
				{
					// Normal - fill background depending on state
					if (!bChecked || bSelected)
					{
						dc.FillRect(&rcButn, (HBRUSH)LongToPtr((bChecked && !bSelected) ? 
							(COLOR_3DLIGHT + 1) : (COLOR_MENU + 1)));
					}
					else
					{
						COLORREF crTxt = dc.SetTextColor(::GetSysColor(COLOR_BTNFACE));
						COLORREF crBk = dc.SetBkColor(::GetSysColor(COLOR_BTNHILIGHT));
						CBrush hbr(CDCHandle::GetHalftoneBrush());
						dc.SetBrushOrg(rcButn.left, rcButn.top);
						dc.FillRect(&rcButn, hbr);
						
						dc.SetTextColor(crTxt);
						dc.SetBkColor(crBk);
					}

					// Draw pushed-in or popped-out edge
					if (bSelected || bChecked)
					{
						RECT rc2 = rcButn;
						dc.DrawEdge(&rc2, bChecked ? BDR_SUNKENOUTER : BDR_RAISEDINNER, BF_RECT);
					}

					// Draw the image
					m_ImageList.Draw(dc, iButton, point.x, point.y, ILD_TRANSPARENT);
				}
				else
				{
					DrawBitmapDisabled(dc, iButton, point);
				}
			}
			else
			{
				// No image - look for custom checked/unchecked bitmaps
				CMenuItemInfo info;
				info.fMask = MIIM_CHECKMARKS;
				::GetMenuItemInfo((HMENU)lpDrawItemStruct->hwndItem, 
					lpDrawItemStruct->itemID, MF_BYCOMMAND, &info);

				if (bChecked || info.hbmpUnchecked)
					bHasImage = Draw3DCheckmark(dc, rcButn, bSelected, bChecked ? info.hbmpChecked : info.hbmpUnchecked);
			}

			// Draw item text
			int cxButn = m_szButton.cx;
			COLORREF colorBG = ::GetSysColor(bSelected ? COLOR_HIGHLIGHT : COLOR_MENU);
			if (bSelected || lpDrawItemStruct->itemAction == ODA_SELECT)
			{
				RECT rcBG = rcItem;

				if (bHasImage)
					rcBG.left += cxButn + s_kcxGap;

				dc.FillRect(&rcBG, (HBRUSH)LongToPtr(bSelected ? (COLOR_HIGHLIGHT + 1) : (COLOR_MENU + 1)));
			}

			// Calc text rectangle and colors
			RECT rcText = rcItem;
			rcText.left += cxButn + s_kcxGap + s_kcxTextMargin;
			rcText.right -= cxButn;
			dc.SetBkMode(TRANSPARENT);
			COLORREF colorText = ::GetSysColor(bDisabled ? 
				COLOR_GRAYTEXT : (bSelected ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));

			// Font already selected by Windows
			if (bDisabled && (!bSelected || colorText == colorBG))
			{
				// Disabled - draw shadow text shifted down and right 1 pixel (unles selected)
				RECT rcDisabled = rcText;
				::OffsetRect(&rcDisabled, 1, 1);
				DrawMenuText(dc, rcDisabled, pmd->lpstrText, GetSysColor(COLOR_3DHILIGHT));
			}

			DrawMenuText(dc, rcText, pmd->lpstrText, colorText); // finally!
		}
	}

	void DrawMenuText(CDCHandle& dc, RECT& rc, LPCTSTR lpstrText, COLORREF color)
	{
		int nTab = -1;
		for(int i = 0; i < lstrlen(lpstrText); i++)
		{
			if (lpstrText[i] == '\t')
			{
				nTab = i;
				break;
			}
		}

		dc.SetTextColor(color);
		dc.DrawText(lpstrText, nTab, &rc, DT_SINGLELINE | DT_LEFT | DT_VCENTER);

		if (nTab != -1)
			dc.DrawText(&lpstrText[nTab + 1], -1, &rc, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
	}

	void DrawBitmapDisabled(CDCHandle& dc, int nImage, POINT point)
	{
		// Create memory DC
		CDC dcMem;
		dcMem.CreateCompatibleDC(dc);

		// Create mono or color bitmap
		CBitmap bmp;
		bmp.CreateCompatibleBitmap(dc, m_szBitmap.cx, m_szBitmap.cy);
		ATLASSERT(bmp.m_hBitmap != NULL);

		// Draw image into memory DC--fill BG white first
		HBITMAP hBmpOld = dcMem.SelectBitmap(bmp);
		dcMem.PatBlt(0, 0, m_szBitmap.cx, m_szBitmap.cy, WHITENESS);

		// If white is the text color, we can't use the normal painting since
		//  it would blend with the WHITENESS, but the mask is OK
		UINT uDrawStyle = (::GetSysColor(COLOR_BTNTEXT) == RGB(255, 255, 255)) ? ILD_MASK : ILD_NORMAL;

		m_ImageList.Draw(dcMem, nImage, 0, 0, uDrawStyle);
		dc.DitherBlt(point.x, point.y, m_szBitmap.cx, m_szBitmap.cy, dcMem, NULL, 0, 0);

		dcMem.SelectBitmap(hBmpOld); // restore
	}

	BOOL Draw3DCheckmark(CDCHandle& dc, const RECT& rc, BOOL bSelected, HBITMAP hBmpCheck)
	{
		// Get checkmark bitmap if none, use Windows standard
		CBitmapHandle bmp = hBmpCheck;
		if (hBmpCheck == NULL)
		{
			bmp.LoadOEMBitmap(OBM_CHECK);
			ATLASSERT(bmp.m_hBitmap != NULL);
		}

		// Center bitmap in caller's rectangle
		SIZE size = { 0, 0 };
		bmp.GetSize(size);
		RECT rcDest = rc;
		POINT p = { 0, 0 };
		SIZE szDelta = { (rc.right - rc.left - size.cx) / 2, 
			(rc.bottom - rc.top - size.cy) / 2 };
		
		if (rc.right - rc.left > size.cx)
		{
			rcDest.left = rc.left + szDelta.cx;
			rcDest.top = rc.top + szDelta.cy;
			rcDest.right = rcDest.left + size.cx;
			rcDest.bottom = rcDest.top + size.cy;
		}
		else
		{
			p.x -= szDelta.cx;
			p.y -= szDelta.cy;
		}
		
		// Select checkmark into memory DC
		CDC dcMem;
		dcMem.CreateCompatibleDC(dc);
		HBITMAP hBmpOld = dcMem.SelectBitmap(bmp);
		
		if (bSelected)
		{
			dc.FillRect(&rcDest, (HBRUSH)LongToPtr((bSelected ? COLOR_MENU : COLOR_3DLIGHT) + 1));
		}
		else
		{
			COLORREF crTxt = dc.SetTextColor(::GetSysColor(COLOR_BTNFACE));
			COLORREF crBk = dc.SetBkColor(::GetSysColor(COLOR_BTNHILIGHT));
			CBrush hbr(CDCHandle::GetHalftoneBrush());
			dc.SetBrushOrg(rcDest.left, rcDest.top);
			dc.FillRect(&rcDest, hbr);
			dc.SetTextColor(crTxt);
			dc.SetBkColor(crBk);
		}
		
		// Draw the check bitmap transparently
		COLORREF crOldBack = dc.SetBkColor(RGB(255, 255, 255));
		COLORREF crOldText = dc.SetTextColor(RGB(0, 0, 0));
		CDC dcTrans;
		
		// Create two memory dcs for the image and the mask
		dcTrans.CreateCompatibleDC(dc);
		
		// Create the mask bitmap	
		CBitmap bitmapTrans;	
		int nWidth = rcDest.right - rcDest.left;
		int nHeight = rcDest.bottom - rcDest.top;	
		bitmapTrans.CreateBitmap(nWidth, nHeight, 1, 1, NULL);
		
		// Select the mask bitmap into the appropriate dc
		dcTrans.SelectBitmap(bitmapTrans);
		
		// Build mask based on transparent color	
		dcMem.SetBkColor(m_clrMask);
		dcTrans.SetBkColor(RGB(0, 0, 0));
		dcTrans.SetTextColor(RGB(255, 255, 255));
		dcTrans.BitBlt(0, 0, nWidth, nHeight, dcMem, p.x, p.y, SRCCOPY);
		dc.BitBlt(rcDest.left, rcDest.top, nWidth, nHeight, dcMem, p.x, p.y, SRCINVERT);
		dc.BitBlt(rcDest.left, rcDest.top, nWidth, nHeight, dcTrans, 0, 0, SRCAND);
		dc.BitBlt(rcDest.left, rcDest.top, nWidth, nHeight, dcMem, p.x, p.y, SRCINVERT);
		
		// Restore settings	
		dc.SetBkColor(crOldBack);
		dc.SetTextColor(crOldText);			
		dcMem.SelectBitmap(hBmpOld);	// restore
		
		// Draw pushed-in hilight
		if (rc.right - rc.left > size.cx)
			::InflateRect(&rcDest, 1,1);	// inflate checkmark by one pixel all around
		
		dc.DrawEdge(&rcDest, BDR_SUNKENOUTER, BF_RECT);

		return TRUE;
	}

	void MeasureMenuItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
	{
		_MenuItemData* pmd = (_MenuItemData*)lpMeasureItemStruct->itemData;

		if (pmd->fType & MFT_SEPARATOR)	// Separator - use half system height and zero width
		{
			lpMeasureItemStruct->itemHeight = ::GetSystemMetrics(SM_CYMENU) / 2;
			lpMeasureItemStruct->itemWidth  = 0;
		}
		else
		{
			// Compute size of text - use DrawText with DT_CALCRECT
			CWindowDC dc(NULL);
			HFONT hOldFont;
			if (pmd->fState & MFS_DEFAULT)
			{
				// Need bold version of font
				LOGFONT lf;
				m_fontMenu.GetLogFont(lf);
				lf.lfWeight += 200;

				CFont fontBold;
				fontBold.CreateFontIndirect(&lf);
				ATLASSERT(fontBold.m_hFont != NULL);
				
				hOldFont = dc.SelectFont(fontBold);
			}
			else
			{
				hOldFont = dc.SelectFont(m_fontMenu);
			}

			RECT rcText = { 0, 0, 0, 0 };
			dc.DrawText(pmd->lpstrText, -1, &rcText, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_CALCRECT);

			int cx = rcText.right - rcText.left;
			dc.SelectFont(hOldFont);

			LOGFONT lf;
			m_fontMenu.GetLogFont(lf);
			
			int cy = lf.lfHeight;

			if (cy < 0)
				cy = -cy;

			cy += 8;

			// Height of item is the bigger of these two
			lpMeasureItemStruct->itemHeight = max(cy, m_szButton.cy);

			// Width is width of text plus a bunch of stuff
			cx += 2 * s_kcxTextMargin;	// L/R margin for readability
			cx += s_kcxGap;			// space between button and menu text
			cx += 2 * m_szButton.cx;	// button width (L=button; R=empty margin)

			// Windows adds 1 to returned value
			cx -= ::GetSystemMetrics(SM_CXMENUCHECK) - 1;
			lpMeasureItemStruct->itemWidth = cx;		// done deal
		}
	}

	void GetSystemSettings()
	{
		// Refresh our font
		NONCLIENTMETRICS info;
		info.cbSize = sizeof(info);
		::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(info), &info, 0);

		LOGFONT logfont;
		memset(&logfont, 0, sizeof(LOGFONT));
		
		if (m_fontMenu.m_hFont != NULL)
			m_fontMenu.GetLogFont(logfont);

		if (logfont.lfHeight != info.lfMenuFont.lfHeight ||
		   logfont.lfWidth != info.lfMenuFont.lfWidth ||
		   logfont.lfEscapement != info.lfMenuFont.lfEscapement ||
		   logfont.lfOrientation != info.lfMenuFont.lfOrientation ||
		   logfont.lfWeight != info.lfMenuFont.lfWeight ||
		   logfont.lfItalic != info.lfMenuFont.lfItalic ||
		   logfont.lfUnderline != info.lfMenuFont.lfUnderline ||
		   logfont.lfStrikeOut != info.lfMenuFont.lfStrikeOut ||
		   logfont.lfCharSet != info.lfMenuFont.lfCharSet ||
		   logfont.lfOutPrecision != info.lfMenuFont.lfOutPrecision ||
		   logfont.lfClipPrecision != info.lfMenuFont.lfClipPrecision ||
		   logfont.lfQuality != info.lfMenuFont.lfQuality ||
		   logfont.lfPitchAndFamily != info.lfMenuFont.lfPitchAndFamily ||
		   lstrcmp(logfont.lfFaceName, info.lfMenuFont.lfFaceName) != 0)
		{
			HFONT hFontMenu = ::CreateFontIndirect(&info.lfMenuFont);
			ATLASSERT(hFontMenu != NULL);

			if (hFontMenu != NULL)
			{
				if (m_fontMenu.m_hFont != NULL)
					m_fontMenu.DeleteObject();

				m_fontMenu.Attach(hFontMenu);

				SetFont(m_fontMenu);
				AddStrings(_T("NS\0"));	// for proper item height
				AutoSize();
			}
		}
	}
};

/////////////////////////////////////////////////////////////////////////////
// CToolBarCtrlEx class

class CToolBarCtrlEx : public CToolBarCtrlExImpl<CToolBarCtrlEx>
{
public:
	DECLARE_WND_SUPERCLASS(_T("BCB_ToolBarEx"), GetWndClassName())	
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif // !defined(_TOOLBARCTRLEX_H_INCLUDED_)
