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
#include "datas/reflector_xml.hpp"
#include "resource.h"
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#include <3dsmaxport.h>
#include <IPathConfigMgr.h>
#include <array>
#include <commctrl.h>
#include <iparamm2.h>

extern HINSTANCE hInstance;

const TCHAR _name[] = _T("Revil Tool");
const TCHAR _info[] =
    _T("\n" RevilMax_COPYRIGHT "Lukas Cone\nVersion " RevilMax_VERSION);
const TCHAR _license[] =
    _T("Revil Tool uses RevilLib, Copyright(C) 2017-2020 Lukas Cone.");
const TCHAR _homePage[] =
    _T("https://lukascone.wordpress.com/2019/05/02/revil-3ds-max-plugin/");

#include "MAXex/win/AboutDlg.h"

RevilMax::RevilMax()
    : hWnd(nullptr), comboHandle(nullptr), objectScale(1.0f), motionIndex(),
      frameRateIndex(1), checked(Checked::RD_ANISEL),
      visible(Visible::CB_MOTION) {
  RegisterReflectedTypes<Visible, Checked>();
}

REFLECTOR_CREATE(RevilMax, 1, VARNAMES, objectScale, motionIndex,
                 frameRateIndex, checked, visible);

static auto GetConfig() {
  TSTRING cfgpath = IPathConfigMgr::GetPathConfigMgr()->GetDir(APP_PLUGCFG_DIR);
  return cfgpath + _T("/RevilMaxSettings.xml");
}

void RevilMax::LoadCFG() {
  pugi::xml_document doc;
  auto conf = GetConfig();
  if (doc.load_file(conf.data())) {
    ReflectorWrap<RevilMax> rWrap(this);
    ReflectorXMLUtil::Load(rWrap, doc);
  }

  CheckDlgButton(hWnd, IDC_CH_ADDITIVE, checked[Checked::CH_ADDITIVE]);
  CheckDlgButton(hWnd, IDC_CH_DISABLEIK, checked[Checked::CH_DISABLEIK]);
  CheckDlgButton(hWnd, IDC_CH_NO_CACHE, checked[Checked::CH_NO_CACHE]);
  CheckDlgButton(hWnd, IDC_CH_NOLOGBONES, checked[Checked::CH_NOLOGBONES]);
  CheckDlgButton(hWnd, IDC_CH_RESAMPLE, checked[Checked::CH_RESAMPLE]);
  CheckDlgButton(hWnd, IDC_RD_ANIALL, checked[Checked::RD_ANIALL]);
  CheckDlgButton(hWnd, IDC_RD_ANISEL, checked[Checked::RD_ANISEL]);
  EnableWindow(comboHandle, visible[Visible::CB_MOTION]);
}

void RevilMax::SaveCFG() {
  pugi::xml_document doc;
  ReflectorWrap<RevilMax> rWrap(this);
  ReflectorXMLUtil::Save(rWrap, doc);
  auto conf = GetConfig();
  doc.save_file(conf.data());
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
                    imp->objectScale);
    SetWindowText(hWnd, _T("Revil Motion Import v" RevilMax_VERSION));

    if (imp->instanceDialogType == RevilMax::DLGTYPE_LMT) {
      HWND fpsHandle = GetDlgItem(hWnd, IDC_CB_FRAMERATE);
      EnableWindow(fpsHandle, true);
      SendMessage(fpsHandle, CB_ADDSTRING, 0, (LPARAM) _T("30"));
      SendMessage(fpsHandle, CB_ADDSTRING, 0, (LPARAM) _T("60"));
      SendMessage(fpsHandle, CB_SETCURSEL, imp->frameRateIndex, 0);
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

      if (imp->motionIndex >= imp->motionNames.size())
        imp->motionIndex = 0;

      SendMessage(imp->comboHandle, CB_SETCURSEL, imp->motionIndex, 0);

      EnableWindow(imp->comboHandle, imp->visible[Visible::CB_MOTION]);
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

    case IDC_CH_RESAMPLE:
      imp->checked.Set(Checked::CH_RESAMPLE,
                       IsDlgButtonChecked(hWnd, IDC_CH_RESAMPLE) != 0);
      break;

    case IDC_CH_ADDITIVE:
      imp->checked.Set(Checked::CH_ADDITIVE,
                       IsDlgButtonChecked(hWnd, IDC_CH_ADDITIVE) != 0);
      break;

    case IDC_CH_NO_CACHE:
      imp->checked.Set(Checked::CH_NO_CACHE,
                       IsDlgButtonChecked(hWnd, IDC_CH_NO_CACHE) != 0);
      break;

    case IDC_CH_NOLOGBONES:
      imp->checked.Set(Checked::CH_NOLOGBONES,
                       IsDlgButtonChecked(hWnd, IDC_CH_NOLOGBONES) != 0);
      break;

    case IDC_CH_DISABLEIK:
      imp->checked.Set(Checked::CH_DISABLEIK,
                       IsDlgButtonChecked(hWnd, IDC_CH_DISABLEIK) != 0);
      break;

    case IDC_RD_ANIALL:
      imp->checked += Checked::RD_ANIALL;
      imp->checked -= Checked::RD_ANISEL;
      imp->visible -= Visible::CB_MOTION;
      EnableWindow(imp->comboHandle, false);
      break;

    case IDC_RD_ANISEL:
      imp->checked -= Checked::RD_ANIALL;
      imp->checked += Checked::RD_ANISEL;
      imp->visible += Visible::CB_MOTION;
      EnableWindow(imp->comboHandle, true);
      break;

    case IDC_CB_MOTION: {
      switch (HIWORD(wParam)) {
      case CBN_SELCHANGE: {
        const LRESULT curSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
        imp->motionIndex = curSel;
        return TRUE;
      } break;
      }
      break;
    }
    case IDC_CB_FRAMERATE: {
      switch (HIWORD(wParam)) {
      case CBN_SELCHANGE: {
        const LRESULT curSel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
        imp->frameRateIndex = curSel;
        return TRUE;
      } break;
      }
      break;
    }
    }

  case CC_SPINNER_CHANGE:
    switch (LOWORD(wParam)) {
    case IDC_SPIN_SCALE:
      imp->objectScale = reinterpret_cast<ISpinnerControl *>(lParam)->GetFVal();
      break;
    }
  }
  return 0;
}

int RevilMax::SpawnDialog() {
  return DialogBoxParam(hInstance, MAKEINTRESOURCE(IDD_REMOTION_IMPORT),
                        GetActiveWindow(), DialogCallbacks, (LPARAM)this);
}
