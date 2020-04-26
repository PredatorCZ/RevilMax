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
#include "datas/masterprinter.hpp"
#include "datas/tchar.hpp"
#include "re_asset.hpp"
#include "uni/motion.hpp"
#include "uni/skeleton.hpp"
#include <memory>
#include <unordered_map>

#define REEngineImport_CLASS_ID Class_ID(0x373d264a, 0x90c37b7)
static const TCHAR _className[] = _T("REEngineImport");

class REEngineImport : public SceneImport, RevilMax {
public:
  // Constructor/Destructor
  REEngineImport();
  virtual ~REEngineImport() {}

  virtual int ExtCount();          // Number of extensions supported
  virtual const TCHAR *Ext(int n); // Extension #n (i.e. "3DS")
  virtual const TCHAR *
  LongDesc(); // Long ASCII description (i.e. "Autodesk 3D Studio File")
  virtual const TCHAR *
  ShortDesc(); // Short ASCII description (i.e. "3D Studio")
  virtual const TCHAR *AuthorName();       // ASCII Author name
  virtual const TCHAR *CopyrightMessage(); // ASCII Copyright message
  virtual const TCHAR *OtherMessage1();    // Other message #1
  virtual const TCHAR *OtherMessage2();    // Other message #2
  virtual unsigned int Version();    // Version number * 100 (i.e. v3.01 = 301)
  virtual void ShowAbout(HWND hWnd); // Show DLL's "About..." box
  virtual int DoImport(const TCHAR *name, ImpInterface *i, Interface *gi,
                       BOOL suppressPrompts = FALSE); // Import file

  std::unordered_map<uint32, INode *> nodes;

  void LoadSkeleton(uni::Skeleton *skel, TimeValue startTime = 0);
  TimeValue LoadMotion(uni::Motion *mot, TimeValue startTime = 0);
};

class : public ClassDesc2 {
public:
  virtual int IsPublic() { return TRUE; }
  virtual void *Create(BOOL) { return new REEngineImport(); }
  virtual const TCHAR *ClassName() { return _className; }
  virtual SClass_ID SuperClassID() { return SCENE_IMPORT_CLASS_ID; }
  virtual Class_ID ClassID() { return REEngineImport_CLASS_ID; }
  virtual const TCHAR *Category() { return NULL; }
  virtual const TCHAR *InternalName() { return _className; }
  virtual HINSTANCE HInstance() { return hInstance; }
} REEngineImportDesc;

ClassDesc2 *GetREEngineImportDesc() { return &REEngineImportDesc; }

//--- HavokImp -------------------------------------------------------
REEngineImport::REEngineImport() {}

int REEngineImport::ExtCount() { return 6; }

const TCHAR *REEngineImport::Ext(int n) {
  switch (n) {
  case 0:
    return _T("motlist.85");
  case 1:
    return _T("motlist");
  case 2:
    return _T("mot.65");
  case 3:
    return _T("mot");
  case 4:
    return _T("motlist.99");
  case 5:
    return _T("mot.78");
  default:
    return nullptr;
  }

  return nullptr;
}

const TCHAR *REEngineImport::LongDesc() { return _T("RE Engine Import"); }

const TCHAR *REEngineImport::ShortDesc() { return _T("RE Engine Import"); }

const TCHAR *REEngineImport::AuthorName() { return _T(RevilMax_AUTHOR); }

const TCHAR *REEngineImport::CopyrightMessage() {
  return _T(RevilMax_COPYRIGHT);
}

const TCHAR *REEngineImport::OtherMessage1() { return _T(""); }

const TCHAR *REEngineImport::OtherMessage2() { return _T(""); }

unsigned int REEngineImport::Version() { return REVILMAX_VERSIONINT; }

void REEngineImport::ShowAbout(HWND hWnd) { ShowAboutDLG(hWnd); }

void REEngineImport::LoadSkeleton(uni::Skeleton *skel, TimeValue startTime) {
  for (auto &b : *skel) {
    TSTRING boneName = ToTSTRING(b.Name());
    INode *node = GetCOREInterface()->GetINodeByName(boneName.c_str());

    if (!node) {
      Object *obj = static_cast<Object *>(
          CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0)));
      node = GetCOREInterface()->CreateObjectNode(obj);
      node->ShowBone(2);
      node->SetWireColor(0x80ff);
      node->SetName(ToBoneName(boneName));
    }

    auto boneTM = b.Transform();
    boneTM.Transpose();

    Matrix3 nodeTM = {
        reinterpret_cast<const Point3 &>(boneTM.r1),
        reinterpret_cast<const Point3 &>(boneTM.r2),
        reinterpret_cast<const Point3 &>(boneTM.r3),
        reinterpret_cast<const Point3 &>(boneTM.r4 * IDC_EDIT_SCALE_value),
    };

    auto parentBone = b.Parent();

    if (parentBone) {
      INode *pNode = nodes[parentBone->Index()];

      pNode->AttachChild(node);
      nodeTM *= pNode->GetNodeTM(startTime);
    } else {
      nodeTM *= corMat;
    }

    Control *cnt = node->GetTMController();

    if (cnt->GetPositionController()->ClassID() !=
        Class_ID(LININTERP_POSITION_CLASS_ID, 0))
      cnt->SetPositionController((Control *)CreateInstance(
          CTRL_POSITION_CLASS_ID, Class_ID(LININTERP_POSITION_CLASS_ID, 0)));

    if (cnt->GetRotationController()->ClassID() !=
        Class_ID(LININTERP_ROTATION_CLASS_ID, 0))
      cnt->SetRotationController((Control *)CreateInstance(
          CTRL_ROTATION_CLASS_ID, Class_ID(LININTERP_ROTATION_CLASS_ID, 0)));

    if (cnt->GetScaleController()->ClassID() !=
        Class_ID(LININTERP_SCALE_CLASS_ID, 0))
      cnt->SetScaleController((Control *)CreateInstance(
          CTRL_SCALE_CLASS_ID, Class_ID(LININTERP_SCALE_CLASS_ID, 0)));
    AnimateOn();
    node->SetNodeTM(startTime, nodeTM);
    AnimateOff();
    node->SetUserPropString(_T("BoneHash"), ToTSTRING(b.Index()).c_str());
    nodes[b.Index()] = node;
  }
}

TimeValue REEngineImport::LoadMotion(uni::Motion *mot, TimeValue startTime) {
  if (!flags[IDC_CH_RESAMPLE_checked])
    SetFrameRate(mot->FrameRate());

  const float aDuration = mot->Duration();
  TimeValue numTicks = SecToTicks(aDuration);
  TimeValue ticksPerFrame = GetTicksPerFrame();
  TimeValue overlappingTicks = numTicks % ticksPerFrame;

  if (overlappingTicks > (ticksPerFrame / 2))
    numTicks += ticksPerFrame - overlappingTicks;
  else
    numTicks -= overlappingTicks;

  Interval aniRange(startTime, startTime + numTicks - ticksPerFrame);
  std::vector<float> frameTimes;
  std::vector<TimeValue> frameTimesTicks;

  for (TimeValue v = aniRange.Start(); v <= aniRange.End();
       v += GetTicksPerFrame()) {
    frameTimes.push_back(TicksToSec(v - startTime));
    frameTimesTicks.push_back(v);
  }

  if (aniRange.Start() == aniRange.End()) {
    aniRange.SetEnd(aniRange.End() + ticksPerFrame);
  }

  GetCOREInterface()->SetAnimRange(aniRange);

  size_t numFrames = frameTimes.size();

  for (auto &v : *mot) {
    if (!nodes.count(v.BoneIndex()))
      continue;

    INode *node = nodes[v.BoneIndex()];
    Control *cnt = node->GetTMController();

    switch (v.TrackType()) {
    case uni::MotionTrack::Position: {
      Control *posControl = cnt->GetPositionController();

      AnimateOn();

      for (int i = 0; i < numFrames; i++) {
        Vector4A16 cVal;
        v.GetValue(cVal, frameTimes[i]);
        cVal *= IDC_EDIT_SCALE_value;
        Point3 kVal = reinterpret_cast<Point3 &>(cVal);

        if (node->GetParentNode()->IsRootNode())
          kVal = corMat.PointTransform(kVal);

        posControl->SetValue(frameTimesTicks[i], &kVal);
      }

      AnimateOff();
      break;
    }

    case uni::MotionTrack::Rotation: {
      Control *rotControl = cnt->GetRotationController();

      AnimateOn();

      for (int i = 0; i < numFrames; i++) {
        Vector4A16 cVal;
        v.GetValue(cVal, frameTimes[i]);
        Quat kVal = reinterpret_cast<Quat &>(cVal).Conjugate();

        if (node->GetParentNode()->IsRootNode()) {
          Matrix3 cMat;
          cMat.SetRotate(kVal);
          kVal = cMat * corMat;
        }

        rotControl->SetValue(frameTimesTicks[i], &kVal);
      }

      AnimateOff();
      break;
    }

    case uni::MotionTrack::Scale: {
      Control *sclControl = cnt->GetScaleController();

      AnimateOn();

      for (int i = 0; i < numFrames; i++) {
        Vector4A16 cVal;
        v.GetValue(cVal, frameTimes[i]);
        Point3 kVal = reinterpret_cast<Point3 &>(cVal);

        if (node->GetParentNode()->IsRootNode())
          kVal = corMat.PointTransform(kVal);

        sclControl->SetValue(frameTimesTicks[i], &kVal);
      }

      AnimateOff();
      break;
    }

    default:
      break;
    }
  }

  return aniRange.End() + ticksPerFrame;
}

int REEngineImport::DoImport(const TCHAR *fileName,
                             ImpInterface * /*importerInt*/, Interface * /*ip*/,
                             BOOL suppressPrompts) {
  char *oldLocale = setlocale(LC_NUMERIC, NULL);
  setlocale(LC_NUMERIC, "en-US");

  std::unique_ptr<REAsset> mainAsset(
      REAsset::Load(std::to_string(fileName).c_str()));

  if (!mainAsset)
    return FALSE;

  uni::List<uni::Motion> *motionList =
      dynamic_cast<decltype(motionList)>(mainAsset.get());
  uni::List<uni::Skeleton> *skelList =
      dynamic_cast<decltype(skelList)>(mainAsset.get());
  uni::Motion *cMotion = nullptr;
  uni::Skeleton *skel = skelList->Size() == 1 ? skelList->At(0) : nullptr;

  if (motionList && motionList->Size()) {
    for (auto &m : *motionList) {
      motionNames.emplace_back(ToTSTRING(m.Name()));
    }

    if (!suppressPrompts)
      if (!SpawnDialog())
        return TRUE;

    if (flags[IDC_RD_ANISEL_checked]) {
      cMotion = motionList->At(IDC_CB_MOTION_index);

      if (!skel) {
        skel = skelList->At(IDC_CB_MOTION_index);
      }
    } else {
      TimeValue lastTime = 0;
      int i = 0;

      printline("Sequencer not found, dumping animation ranges:");

      for (auto &m : *motionList) {
        uni::Skeleton *_skel = skel ? skel : skelList->At(i);

        LoadSkeleton(_skel, lastTime);
        TimeValue nextTime = LoadMotion(&m, lastTime);
        printer << std::to_string(motionNames[i]) << ": " << lastTime << ", " << nextTime >> 1;
        lastTime = nextTime;
        i++;
      }

      setlocale(LC_NUMERIC, oldLocale);
      return TRUE;
    }
  }

  if (!cMotion) {
    cMotion = dynamic_cast<uni::Motion *>(mainAsset.get());
  }

  if (!cMotion) {
    MessageBox(GetActiveWindow(),
               _T("Could't find any defined classes within file."),
               _T("Undefined file."), MB_ICONSTOP);
    return TRUE;
  }

  LoadSkeleton(skel);
  LoadMotion(cMotion);

  setlocale(LC_NUMERIC, oldLocale);

  return TRUE;
}
