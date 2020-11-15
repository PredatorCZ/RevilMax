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

#pragma once
#include "MAXex/3DSMaxSDKCompat.h"
#include <iparamb2.h>
#include <iparamm2.h>
#include <istdplug.h>
#include <maxtypes.h>
// SIMPLE TYPE

#include <commdlg.h>
#include <direct.h>
#include <impexp.h>
#undef min
#undef max
#include "project.h"
#include "datas/reflector.hpp"

#include "datas/flags.hpp"

#include <vector>

extern HINSTANCE hInstance;

static constexpr int REVILMAX_VERSIONINT =
    RevilMax_VERSION_MAJOR * 100 + RevilMax_VERSION_MINOR;

static const Matrix3 corMat = {{1.0f, 0.0f, 0.0f},
                               {0.0f, 0.0f, 1.0f},
                               {0.0f, -1.0f, 0.0f},
                               {0.0f, 0.0f, 0.0f}};

REFLECTOR_CREATE(Checked, ENUM, 2, CLASS, 8, RD_ANIALL, RD_ANISEL, CH_RESAMPLE,
                 CH_ADDITIVE, CH_DISABLEIK, CH_NO_CACHE, CH_NOLOGBONES);

REFLECTOR_CREATE(Visible, ENUM, 2, CLASS, 8, CB_MOTION);

class RevilMax : public ReflectorInterface<RevilMax> {
public:
  enum DLGTYPE_e { DLGTYPE_unknown, DLGTYPE_MOT, DLGTYPE_LMT };

  es::Flags<Checked> checked;
  es::Flags<Visible> visible;
  float objectScale;
  uint32 motionIndex, frameRateIndex;

  DLGTYPE_e instanceDialogType;
  HWND comboHandle;
  HWND hWnd;
  std::vector<TSTRING> motionNames;
  int windowSize, button1Distance, button2Distance;

  void LoadCFG();
  void BuildCFG();
  void SaveCFG();
  int SpawnDialog();

  RevilMax();
  virtual ~RevilMax() {}
};

void ShowAboutDLG(HWND hWnd);
