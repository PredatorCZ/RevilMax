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
#include "datas/master_printer.hpp"
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

  void LoadSkeleton(const uni::Skeleton *skel, TimeValue startTime = 0);
  TimeValue LoadMotion(const uni::Motion *mot, TimeValue startTime = 0);
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

int REEngineImport::ExtCount() { return 8; }

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
  case 6:
    return _T("motlist.60");
  case 7:
    return _T("mot.43");
  default:
    return nullptr;
  }

  return nullptr;
}

const TCHAR *REEngineImport::LongDesc() { return _T("RE Engine Import"); }

const TCHAR *REEngineImport::ShortDesc() { return _T("RE Engine Import"); }

const TCHAR *REEngineImport::AuthorName() { return _T("Lukas Cone"); }

const TCHAR *REEngineImport::CopyrightMessage() {
  return _T(RevilMax_COPYRIGHT "Lukas Cone");
}

const TCHAR *REEngineImport::OtherMessage1() { return _T(""); }

const TCHAR *REEngineImport::OtherMessage2() { return _T(""); }

unsigned int REEngineImport::Version() { return REVILMAX_VERSIONINT; }

void REEngineImport::ShowAbout(HWND hWnd) { ShowAboutDLG(hWnd); }

void REEngineImport::LoadSkeleton(const uni::Skeleton *skel,
                                  TimeValue startTime) {
  for (auto &b : *skel) {
    TSTRING boneName = ToTSTRING(b->Name());
    INode *node = GetCOREInterface()->GetINodeByName(boneName.c_str());

    if (!node) {
      if (flags[IDC_CH_ADDITIVE_checked]) {
        if (!flags[IDC_CH_NOLOGBONES_checked]) {
          printerror("Cannot find bone: " << b->Name());
        }
        continue;
      }
      Object *obj = static_cast<Object *>(
          CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0)));
      node = GetCOREInterface()->CreateObjectNode(obj);
      node->ShowBone(2);
      node->SetWireColor(0x80ff);
      node->SetName(ToBoneName(boneName));
    }

    uni::RTSValue boneTM;
    b->GetTM(boneTM);

    Matrix3 nodeTM;
    nodeTM.SetRotate(
        reinterpret_cast<const Quat &>(boneTM.rotation.QConjugate()));
    nodeTM.SetTranslate(reinterpret_cast<const Point3 &>(boneTM.translation *
                                                         IDC_EDIT_SCALE_value));

    auto parentBone = b->Parent();

    if (parentBone && !flags[IDC_CH_ADDITIVE_checked]) {
      INode *pNode = nodes[parentBone->Index()];

      pNode->AttachChild(node);
    } else if (flags[IDC_CH_ADDITIVE_checked]) {
      Matrix3 pMat = node->GetParentTM(-1);
      pMat.Invert();
      nodeTM = node->GetNodeTM(-1) * pMat;
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
    SetXFormPacket packet(nodeTM);
    node->GetTMController()->SetValue(startTime, &packet);
    node->GetTMController()->SetValue(-1, &packet);
    AnimateOff();

    node->SetUserPropString(_T("BoneHash"), ToTSTRING(b->Index()).c_str());
    nodes[b->Index()] = node;
  }
}

TimeValue REEngineImport::LoadMotion(const uni::Motion *mot,
                                     TimeValue startTime) {
  if (!flags[IDC_CH_RESAMPLE_checked])
    SetFrameRate(mot->FrameRate());

  const float aDuration = mot->Duration();
  TimeValue numTicks = SecToTicks(aDuration);
  const TimeValue ticksPerFrame = GetTicksPerFrame();
  TimeValue overlappingTicks = numTicks % ticksPerFrame;

  if (overlappingTicks > (ticksPerFrame / 2))
    numTicks += ticksPerFrame - overlappingTicks;
  else
    numTicks -= overlappingTicks;

  Interval aniRange(startTime, startTime + numTicks - ticksPerFrame);
  std::vector<float> frameTimes;
  std::vector<TimeValue> frameTimesTicks;

  for (TimeValue v = aniRange.Start(); v <= aniRange.End();
       v += ticksPerFrame) {
    frameTimes.push_back(TicksToSec(v - startTime));
    frameTimesTicks.push_back(v);
  }

  if (aniRange.Start() == aniRange.End()) {
    aniRange.SetEnd(aniRange.End() + ticksPerFrame);
  }

  GetCOREInterface()->SetAnimRange(aniRange);

  size_t numFrames = frameTimes.size();

  for (auto &v : *mot) {
    if (!nodes.count(v->BoneIndex()))
      continue;

    INode *node = nodes[v->BoneIndex()];
    Control *cnt = node->GetTMController();

    switch (v->TrackType()) {
    case uni::MotionTrack::Position: {
      Control *posControl = cnt->GetPositionController();

      AnimateOn();

      for (int i = 0; i < numFrames; i++) {
        Vector4A16 cVal;
        v->GetValue(cVal, frameTimes[i]);
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
        v->GetValue(cVal, frameTimes[i]);
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
        v->GetValue(cVal, frameTimes[i]);
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

void SwapLocale() {
  static char *oldLocale = nullptr;

  if (oldLocale) {
    setlocale(LC_NUMERIC, oldLocale);
    oldLocale = nullptr;
  } else {
    oldLocale = setlocale(LC_NUMERIC, nullptr);
    setlocale(LC_NUMERIC, "en-US");
  }
}

static struct {
  std::unique_ptr<REAsset> asset;
  TSTRING filename;
} areCache;

int REEngineImport::DoImport(const TCHAR *fileName,
                             ImpInterface * /*importerInt*/, Interface * /*ip*/,
                             BOOL suppressPrompts) try {
  SwapLocale();
  TSTRING _filename = fileName;
  std::unique_ptr<REAsset> mainAsset;

  auto UpdateCache = [&](std::unique_ptr<REAsset> &in) {
    if (!flags[IDC_CH_NO_CACHE_checked]) {
      areCache.asset = std::move(in);
    } else {
      areCache.filename.clear();
    }
  };

  if (areCache.filename == _filename) {
    mainAsset = std::move(areCache.asset);
  } else {
    mainAsset = std::unique_ptr<REAsset>(
        REAsset::Load(std::to_string(_filename).c_str(), true));

    if (!mainAsset) {
      SwapLocale();
      return FALSE;
    }

    auto destroy = std::move(areCache.asset);
    areCache.filename = _filename;
  }
  uni::List<uni::Motion> *motionList =
      dynamic_cast<decltype(motionList)>(mainAsset.get());
  uni::List<uni::Skeleton> *skelList =
      dynamic_cast<decltype(skelList)>(mainAsset.get());
  uni::Element<const uni::Motion> cMotion;
  auto skel = skelList->Size() == 1 ? skelList->At(0) : nullptr;

  if (motionList && motionList->Size()) {
    for (auto &m : *motionList) {
      motionNames.emplace_back(ToTSTRING(m->Name()));
    }

    if (!suppressPrompts) {
      if (!SpawnDialog()) {
        UpdateCache(mainAsset);
        SwapLocale();
        return TRUE;
      }
    }

    if (flags[IDC_RD_ANISEL_checked]) {
      cMotion = std::move(motionList->At(IDC_CB_MOTION_index));

      if (!skel && skelList->Size()) {
        skel = skelList->At(IDC_CB_MOTION_index);
      }
    } else {
      TimeValue lastTime = 0;
      int i = 0;

      printline(
          "Sequencer not found, dumping animation ranges (in tick units):");

      for (auto &m : *motionList) {
        if (skelList->Size()) {
          auto _skel = skel ? std::move(skel) : skelList->At(i);

          LoadSkeleton(_skel.get(), lastTime);
        }

        TimeValue nextTime = LoadMotion(m.get(), lastTime);
        printer << std::to_string(motionNames[i]) << ": " << lastTime << ", "
                << nextTime >>
            1;
        lastTime = nextTime;
        i++;
      }

      SwapLocale();
      UpdateCache(mainAsset);
      return TRUE;
    }
  }

  if (!cMotion) {
    cMotion = std::move(decltype(cMotion){
        dynamic_cast<const uni::Motion *>(mainAsset.get()), false});
  }

  if (!cMotion) {
    MessageBox(GetActiveWindow(),
               _T("Could't find any defined classes within file."),
               _T("Undefined file."), MB_ICONSTOP);
    SwapLocale();
    UpdateCache(mainAsset);
    return TRUE;
  }

  if (skel) {
    LoadSkeleton(skel.get());
  }

  LoadMotion(cMotion.get());

  SwapLocale();
  UpdateCache(mainAsset);

  return TRUE;
} catch (const std::exception &e) {
  areCache.filename.clear();
  SwapLocale();
  auto msg = ToTSTRING(e.what());
  MessageBox(GetActiveWindow(), msg.data(), _T("Exception thrown!"),
             MB_ICONERROR | MB_OK);
  return TRUE;
} catch (...) {
  areCache.filename.clear();
  SwapLocale();
  MessageBox(GetActiveWindow(), _T("Unhandled exception has been thrown!"),
             _T("Exception thrown!"), MB_ICONERROR | MB_OK);
  return TRUE;
}
