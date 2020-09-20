/*  Revil Tool for 3ds Max
    Copyright(C) 2019-2020 Lukas Cone

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

    Revil Tool uses RevilLib 2017-2020 Lukas Cone
*/

#include "RevilMax.h"
#include "datas/directory_scanner.hpp"
#include "resource.h"
#include <3dsmaxport.h>
#include <IPathConfigMgr.h>
#include <array>
#include <commctrl.h>
#include <iparamm2.h>

extern HINSTANCE hInstance;

const TCHAR _name[] = _T("Revil Tool");
const TCHAR _info[] = _T("\n" RevilMax_COPYRIGHT "Lukas Cone\nVersion " RevilMax_VERSION);
const TCHAR _license[] =
    _T("Revil Tool uses RevilLib, Copyright(C) 2017-2020 Lukas Cone.");
const TCHAR _homePage[] =
    _T("https://lukascone.wordpress.com/2019/05/02/revil-3ds-max-plugin/");

#include "MAXex/win/AboutDlg.h"

RevilMax::RevilMax()
    : CFGFile(nullptr), hWnd(nullptr), comboHandle(nullptr),
      IDConfigValue(IDC_EDIT_SCALE)(1.0f), IDConfigIndex(IDC_CB_MOTION)(0),
      IDConfigIndex(IDC_CB_FRAMERATE)(1),
      flags(IDC_RD_ANISEL_checked, IDC_CB_MOTION_enabled) {}

void RevilMax::BuildCFG() {
  cfgpath = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
  cfgpath.append(_T("\\RevilMaxSettings.ini"));
  CFGFile = cfgpath.c_str();
}

void RevilMax::LoadCFG() {
  BuildCFG();
  TCHAR buffer[CFGBufferSize];

  GetCFGValue(IDC_EDIT_SCALE);
  GetCFGIndex(IDC_CB_MOTION);
  GetCFGIndex(IDC_CB_FRAMERATE);
  GetCFGChecked(IDC_RD_ANISEL);
  GetCFGChecked(IDC_RD_ANIALL);
  GetCFGChecked(IDC_CH_RESAMPLE);
  GetCFGChecked(IDC_CH_ADDITIVE);
  GetCFGChecked(IDC_CH_NO_CACHE);
  GetCFGChecked(IDC_CH_NOLOGBONES);
  GetCFGEnabled(IDC_CB_MOTION);
}

void RevilMax::SaveCFG() {
  BuildCFG();
  TCHAR buffer[CFGBufferSize];

  SetCFGIndex(IDC_CB_MOTION);
  SetCFGValue(IDC_EDIT_SCALE);
  SetCFGIndex(IDC_CB_FRAMERATE);
  SetCFGChecked(IDC_RD_ANISEL);
  SetCFGChecked(IDC_RD_ANIALL);
  SetCFGChecked(IDC_CH_RESAMPLE);
  SetCFGChecked(IDC_CH_ADDITIVE);
  SetCFGChecked(IDC_CH_NO_CACHE);
  SetCFGChecked(IDC_CH_NOLOGBONES);
  SetCFGEnabled(IDC_CB_MOTION);
}

static INT_PTR CALLBACK DialogCallbacks(HWND hWnd, UINT message, WPARAM wParam,
                                        LPARAM lParam) {
  RevilMax *imp = DLGetWindowLongPtr<RevilMax *>(hWnd);

  switch (message) {
  case WM_INITDIALOG: {
    CenterWindow(hWnd, GetParent(hWnd));
    imp = reinterpret_cast<RevilMax *>(lParam);
    DLSetWindowLongPtr(hWnd, lParam);
    imp->hWnd = hWnd;
    imp->comboHandle = GetDlgItem(hWnd, IDC_CB_MOTION);
    imp->LoadCFG();
    SetupIntSpinner(hWnd, IDC_SPIN_SCALE, IDC_EDIT_SCALE, 0, 5000,
                    imp->IDC_EDIT_SCALE_value);
    SetWindowText(hWnd, _T("Revil Motion Import v" RevilMax_VERSION));

    if (imp->instanceDialogType == RevilMax::DLGTYPE_LMT) {
      HWND fpsHandle = GetDlgItem(hWnd, IDC_CB_FRAMERATE);
      EnableWindow(fpsHandle, true);
      SendMessage(fpsHandle, CB_ADDSTRING, 0, (LPARAM) _T("30"));
      SendMessage(fpsHandle, CB_ADDSTRING, 0, (LPARAM) _T("60"));
      SendMessage(fpsHandle, CB_SETCURSEL, imp->IDC_CB_FRAMERATE_index, 0);
      EnableWindow(GetDlgItem(hWnd, IDC_CH_DISABLEIK), true);
    }

    HWND butt = GetDlgItem(hWnd, IDC_BT_DONE);
    RECT buttRect;
    RECT rekt;

    GetWindowRect(hWnd, &rekt);
    imp->windowSize = rekt.right - rekt.left;
    GetClientRect(hWnd, &rekt);

    GetWindowRect(butt, &buttRect);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&buttRect, 2);

    imp->button1Distance = rekt.right - buttRect.left;

    butt = GetDlgItem(hWnd, IDC_BT_CANCEL);
    GetWindowRect(butt, &buttRect);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&buttRect, 2);
    imp->button2Distance = rekt.right - buttRect.left;

    if (imp->motionNames.size()) {
      for (auto &p : imp->motionNames)
        SendMessage(imp->comboHandle, CB_ADDSTRING, 0, (LPARAM)p.c_str());

      if (imp->IDC_CB_MOTION_index >= imp->motionNames.size())
        imp->IDC_CB_MOTION_index = 0;

      SendMessage(imp->comboHandle, CB_SETCURSEL, imp->IDC_CB_MOTION_index, 0);

      EnableWindow(imp->comboHandle,
                   imp->flags[RevilMax::IDC_RD_ANISEL_checked]);
    }

    return TRUE;
  }

  case WM_SIZE: {
    HWND butt = GetDlgItem(hWnd, IDC_BT_DONE);
    RECT buttRect;
    RECT rekt;

    GetClientRect(hWnd, &rekt);
    GetWindowRect(butt, &buttRect);
    MapWindowPoints(HWND_DESKTOP, hWnd, (LPPOINT)&buttRect, 2);

    SetWindowPos(butt, 0, rekt.right - imp->button1Distance, buttRect.top, 0, 0,
                 SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER);

    butt = GetDlgItem(hWnd, IDC_BT_CANCEL);
    SetWindowPos(butt, 0, rekt.right - imp->button2Distance, buttRect.top, 0, 0,
                 SWP_NOZORDER | SWP_NOSIZE | SWP_NOOWNERZORDER);

    butt = GetDlgItem(hWnd, IDC_CB_MOTION);
    GetClientRect(butt, &buttRect);

    SetWindowPos(butt, 0, 0, 0, rekt.right - rekt.left - 30, buttRect.bottom,
                 SWP_NOZORDER | SWP_NOMOVE | SWP_NOOWNERZORDER);

    break;
  }

  case WM_GETMINMAXINFO: {
    LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
    lpMMI->ptMinTrackSize.x = imp->windowSize;
    break;
  }

  case WM_NCHITTEST: {
    LRESULT lRes = DefWindowProc(hWnd, message, wParam, lParam);

    if (lRes == HTBOTTOMLEFT || lRes == HTBOTTOMRIGHT || lRes == HTTOPLEFT ||
        lRes == HTTOPRIGHT || lRes == HTTOP || lRes == HTBOTTOM ||
        lRes == HTSIZE)
      return TRUE;

    return FALSE;
  }

  case WM_CLOSE:
    EndDialog(hWnd, 0);
    imp->SaveCFG();
    return 1;

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
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

      MSGCheckbox(IDC_CH_RESAMPLE);
      break;

      MSGCheckbox(IDC_CH_ADDITIVE);
      break;

      MSGCheckbox(IDC_CH_NO_CACHE);
      break;

      MSGCheckbox(IDC_CH_NOLOGBONES);
      break;

      MSGCheckbox(IDC_CH_DISABLEIK);
      break;

      MSGCheckbox(IDC_RD_ANIALL);
      imp->flags -= RevilMax::IDC_RD_ANISEL_checked;
      MSGEnable(IDC_RD_ANISEL, IDC_CB_MOTION);
      break;

      MSGCheckbox(IDC_RD_ANISEL);
      imp->flags -= RevilMax::IDC_RD_ANIALL_checked;
      MSGEnable(IDC_RD_ANISEL, IDC_CB_MOTION);
      break;

    case IDC_CB_MOTION: {
      switch (HIWORD(wParam)) {
      case CBN_SELCHANGE: {
        const LRESULT curSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
        imp->IDC_CB_MOTION_index = curSel;
        return TRUE;
      } break;
      }
      break;
    }
    case IDC_CB_FRAMERATE: {
      switch (HIWORD(wParam)) {
      case CBN_SELCHANGE: {
        const LRESULT curSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
        imp->IDC_CB_FRAMERATE_index = curSel;
        return TRUE;
      } break;
      }
      break;
    }
    }

  case CC_SPINNER_CHANGE:
    switch (LOWORD(wParam)) {
    case IDC_SPIN_SCALE:
      imp->IDC_EDIT_SCALE_value =
          reinterpret_cast<ISpinnerControl *>(lParam)->GetFVal();
      break;
    }
  }
  return 0;
}

int RevilMax::SpawnDialog() {
  return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_REMOTION_IMPORT),
                        GetActiveWindow(), DialogCallbacks, (LPARAM)this);
}
