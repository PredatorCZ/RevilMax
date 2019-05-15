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

#pragma once
#include "MAXex/3DSMaxSDKCompat.h"
#include <istdplug.h>
#include <iparamb2.h>
#include <iparamm2.h>
#include <maxtypes.h>
//SIMPLE TYPE

#include <impexp.h>
#include <direct.h>
#include <commdlg.h>

#include "MAXex/win/CFGMacros.h"
#include <vector>

extern TCHAR *GetString(int id);
extern HINSTANCE hInstance;

#define REVILMAXVERSION "1.1"
#define REVILMAXVERSIONINT 110

static const Matrix3 corMat =
{
	{1.0f, 0.0f, 0.0f},
	{0.0f, 0.0f, 1.0f},
	{0.0f, -1.0f, 0.0f},
	{0.0f, 0.0f, 0.0f}
};
class RevilMax
{
public:
	HWND comboHandle;
	HWND hWnd;
	TSTRING cfgpath;
	const TCHAR *CFGFile;
	std::vector<TSTRING> motionNames;

	NewIDConfigValue(IDC_EDIT_SCALE);
	NewIDConfigIndex(IDC_CB_MOTION);

	void LoadCFG();
	void BuildCFG();
	void SaveCFG();
	int SpawnDialog();

	RevilMax();
	virtual ~RevilMax() {}
};

void ShowAboutDLG(HWND hWnd);
