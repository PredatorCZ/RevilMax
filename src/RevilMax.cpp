/*	Revil Tool for 3ds Max
	Copyright(C) 2019 Lukas Cone

	This program is free software : you can redistribute it and / or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.If not, see <https://www.gnu.org/licenses/>.
	
	Revil Tool uses RevilLib 2017-2019 Lukas Cone
*/

#include "RevilMax.h"
#include "resource.h"
#include <commctrl.h>
#include <IPathConfigMgr.h>
#include <3dsmaxport.h>
#include <iparamm2.h>
#include "datas/DirectoryScanner.hpp"
#include <array>


extern HINSTANCE hInstance;

const TCHAR _name[] = _T("Revil Tool");
const TCHAR _info[] = _T("\nCopyright (C) 2019 Lukas Cone\nVersion 1");
const TCHAR _license[] = _T("Revil Tool uses RevilLib, Copyright(C) 2017-2019 Lukas Cone.");
const TCHAR _homePage[] = _T("https://lukascone.wordpress.com/2019/05/02/revil-3ds-max-plugin/");

#include "MAXex/win/AboutDlg.h"

RevilMax::RevilMax() :
	CFGFile(nullptr), hWnd(nullptr), comboHandle(nullptr),
	IDConfigValue(IDC_EDIT_SCALE)(1.0f), IDConfigIndex(IDC_CB_MOTION)(0)
{}

void RevilMax::BuildCFG()
{
	cfgpath = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
	cfgpath.append(_T("\\RevilMaxSettings.ini"));
	CFGFile = cfgpath.c_str();
}

void RevilMax::LoadCFG()
{
	BuildCFG();
	TCHAR buffer[CFGBufferSize];

	GetCFGValue(IDC_EDIT_SCALE);
	GetCFGIndex(IDC_CB_MOTION);
}

void RevilMax::SaveCFG()
{
	BuildCFG();
	TCHAR buffer[CFGBufferSize];

	SetCFGIndex(IDC_CB_MOTION);
	SetCFGValue(IDC_EDIT_SCALE);
}

static INT_PTR CALLBACK DialogCallbacks(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	RevilMax *imp = DLGetWindowLongPtr<RevilMax *>(hWnd);

	switch (message) {
	case WM_INITDIALOG:
	{
		CenterWindow(hWnd, GetParent(hWnd));
		imp = reinterpret_cast<RevilMax *>(lParam);
		DLSetWindowLongPtr(hWnd, lParam);
		imp->hWnd = hWnd;
		imp->comboHandle = GetDlgItem(hWnd, IDC_CB_MOTION);
		imp->LoadCFG();
		SetupIntSpinner(hWnd, IDC_SPIN_SCALE, IDC_EDIT_SCALE, 0, 5000, imp->IDC_EDIT_SCALE_value);
		SetWindowText(hWnd, _T("RE Engine Motion Import v1"));

		if (imp->motionNames.size())
		{
			for (auto &p : imp->motionNames)
				SendMessage(imp->comboHandle, CB_ADDSTRING, 0, (LPARAM)p.c_str());

			if (imp->IDC_CB_MOTION_index >= imp->motionNames.size())
				imp->IDC_CB_MOTION_index = 0;

			SendMessage(imp->comboHandle, CB_SETCURSEL, imp->IDC_CB_MOTION_index, 0);

			EnableWindow(imp->comboHandle, true);
		}

		return TRUE; 
	}

	case WM_CLOSE:
		EndDialog(hWnd, 0);
		imp->SaveCFG();
		return 1;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BT_DONE:
			EndDialog(hWnd, 1);
			imp->SaveCFG();
			return 1;
		case IDC_BT_ABOUT:
			ShowAboutDLG(hWnd);
			return 1;
		case IDC_BT_CANCEL:
			EndDialog(hWnd, 0);
			imp->SaveCFG();
			return 1;
		}

	case CC_SPINNER_CHANGE:
		switch (LOWORD(wParam))
		{
		case IDC_SPIN_SCALE:
			imp->IDC_EDIT_SCALE_value = reinterpret_cast<ISpinnerControl *>(lParam)->GetFVal();
			break;
		}

	case IDC_CB_MOTION:
	{
		switch (HIWORD(wParam))
		{
		case CBN_SELCHANGE:
		{
			const LRESULT curSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
			imp->IDC_CB_MOTION_index = curSel;
			return TRUE;
		}
		break;
		}
		break;
	}
	}
	return 0;
}

int RevilMax::SpawnDialog()
{
	return DialogBoxParam(hInstance,
		MAKEINTRESOURCE(IDD_REMOTION_IMPORT),
		GetActiveWindow(),
		DialogCallbacks, (LPARAM)this);
}