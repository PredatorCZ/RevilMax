/*  Revil Tool for 3ds Max
    Copyright(C) 2019-2021 Lukas Cone

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
#include "datas/except.hpp"
#include "datas/master_printer.hpp"
#include "datas/tchar.hpp"
#include "revil/re_asset.hpp"
#include "uni/motion.hpp"
#include "uni/rts.hpp"
#include "uni/skeleton.hpp"
#include <memory>
#include <unordered_map>

#define REEngineImport_CLASS_ID Class_ID(0x373d264a, 0x90c37b7)
static const TCHAR _className[] = _T("REEngineImport");

class REEngineImport : public SceneImport, RevilMax {
public:
  // Constructor/Destructor
  REEngineImport();

  int ExtCount() override;                  // Number of extensions supported
  const TCHAR *Ext(int n) override;         // Extension #n (i.e. "3DS")
  const TCHAR *LongDesc() override;         // Long ASCII description
  const TCHAR *ShortDesc() override;        // Short ASCII description
  const TCHAR *AuthorName() override;       // ASCII Author name
  const TCHAR *CopyrightMessage() override; // ASCII Copyright message
  const TCHAR *OtherMessage1() override;    // Other message #1
  const TCHAR *OtherMessage2() override;    // Other message #2
  unsigned int Version() override;    // Version number * 100 (i.e. v3.01 = 301)
  void ShowAbout(HWND hWnd) override; // Show DLL's "About..." box
  int DoImport(const TCHAR *name, ImpInterface *i, Interface *gi,
               BOOL suppressPrompts = FALSE) override;
  void DoImport(const std::string &fileName, bool suppressPrompts);

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
  const TCHAR *NonLocalizedClassName() { return _className; }
} REEngineImportDesc;

ClassDesc2 *GetREEngineImportDesc() { return &REEngineImportDesc; }

//--- HavokImp -------------------------------------------------------
REEngineImport::REEngineImport() {}

int REEngineImport::ExtCount() { return 10; }

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
  case 8:
    return _T("motlist.486");
  case 9:
    return _T("mot.458");
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

static class : public ITreeEnumProc {
public:
  const MSTR boneNameHint = _T("BoneHash");

  std::vector<INode *> bones;

  void RescanBones() {
    bones.clear();
    GetCOREInterface7()->GetScene()->EnumTree(this);
  }

  void LockPose(TimeValue atTime) {
    for (auto &b : bones) {
      Matrix3 pMat = b->GetParentTM(atTime);
      pMat.Invert();
      Matrix3 mtx = b->GetNodeTM(atTime) * pMat;
      SetXFormPacket packet(mtx);
      AnimateOn();
      b->GetTMController()->SetValue(atTime, &packet);
      AnimateOff();
    }
  }

  void ResetScene() {
    SuspendAnimate();

    for (auto &n : bones) {
      Matrix3 pMat = n->GetParentTM(-1);
      pMat.Invert();
      Matrix3 mtx = n->GetNodeTM(-1) * pMat;
      SetXFormPacket packet(mtx);
      Control *cnt = n->GetTMController();
      cnt->GetScaleController()->DeleteKeys(TRACK_DOALL | TRACK_RIGHTTOLEFT);
      cnt->GetRotationController()->DeleteKeys(TRACK_DOALL | TRACK_RIGHTTOLEFT);
      cnt->GetPositionController()->DeleteKeys(TRACK_DOALL | TRACK_RIGHTTOLEFT);
      AnimateOn();
      n->GetTMController()->SetValue(-1, &packet);
      AnimateOff();
    }
  }

  int callback(INode *node) {
    if (node->UserPropExists(boneNameHint)) {
      bones.push_back(node);
    }

    return TREE_CONTINUE;
  }
} REBoneScanner;

void REEngineImport::LoadSkeleton(const uni::Skeleton *skel,
                                  TimeValue startTime) {
  for (auto &b : *skel) {
    TSTRING boneName = ToTSTRING(b->Name());
    INode *node = GetCOREInterface()->GetINodeByName(boneName.c_str());

    if (!node) {
      if (checked[Checked::CH_ADDITIVE]) {
        if (!checked[Checked::CH_NOLOGBONES]) {
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
    nodeTM.SetTrans(
        reinterpret_cast<const Point3 &>(boneTM.translation * objectScale));

    auto parentBone = b->Parent();

    if (parentBone && !checked[Checked::CH_ADDITIVE]) {
      INode *pNode = nodes[parentBone->Index()];

      pNode->AttachChild(node);
    } else if (checked[Checked::CH_ADDITIVE]) {
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

    node->SetUserPropString(REBoneScanner.boneNameHint,
                            ToTSTRING(b->Index()).data());
    nodes[b->Index()] = node;
  }
}

TimeValue REEngineImport::LoadMotion(const uni::Motion *mot,
                                     TimeValue startTime) {
  if (!checked[Checked::CH_RESAMPLE]) {
    SetFrameRate(mot->FrameRate());
  }

  const float aDuration = mot->Duration();
  TimeValue numTicks = SecToTicks(aDuration);
  const TimeValue ticksPerFrame = GetTicksPerFrame();
  TimeValue overlappingTicks = numTicks % ticksPerFrame;

  if (overlappingTicks > (ticksPerFrame / 2)) {
    numTicks += ticksPerFrame - overlappingTicks;
  } else {
    numTicks -= overlappingTicks;
  }

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
        cVal *= objectScale;
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
        Quat kVal = reinterpret_cast<Quat &>(cVal.QConjugate());

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
  static std::string oldLocale;

  if (!oldLocale.empty()) {
    setlocale(LC_NUMERIC, oldLocale.data());
    oldLocale.clear();
  } else {
    oldLocale = setlocale(LC_NUMERIC, nullptr);
    setlocale(LC_NUMERIC, "en-US");
  }
}

static struct {
  revil::REAsset asset;
  std::string filename;
} areCache;

void REEngineImport::DoImport(const std::string &fileName,
                              bool suppressPrompts) {
  if (areCache.filename != fileName) {
    areCache.asset.Load(fileName);
    areCache.filename = fileName;
  }

  auto motionList = areCache.asset.As<uni::MotionsConst>();
  auto skelList = areCache.asset.As<uni::SkeletonsConst>();
  uni::Element<const uni::Motion> cMotion;
  auto skel = motionList->Size() > skelList->Size() ? skelList->At(0) : nullptr;

  if (motionList && motionList->Size()) {
    for (auto &m : *motionList) {
      motionNames.emplace_back(ToTSTRING(m->Name()));
    }

    if (!suppressPrompts) {
      if (!SpawnDialog()) {
        return;
      }
    }

    if (checked[Checked::RD_ANISEL]) {
      cMotion = std::move(motionList->At(motionIndex));

      if (!skel && skelList->Size()) {
        skel = skelList->At(motionIndex);
      }
    } else {
      TimeValue lastTime = 0;
      int i = 0;

      printline(
          "Sequencer not found, dumping animation ranges (in tick units):");

      for (auto &m : *motionList) {
        if (skelList->Size()) {
          auto _skel =
              skel ? decltype(skel){skel.get(), false} : skelList->At(i);

          LoadSkeleton(_skel.get(), lastTime);
        }

        TimeValue nextTime = LoadMotion(m.get(), lastTime);
        printer << std::to_string(motionNames[i]) << ": " << lastTime << ", "
                << nextTime >>
            1;
        lastTime = nextTime;
        REBoneScanner.RescanBones();
        REBoneScanner.LockPose(lastTime - GetTicksPerFrame());
        i++;
      }

      return;
    }
  }

  if (!cMotion) {
    cMotion = areCache.asset.As<uni::Element<const uni::Motion>>();
  }

  if (!cMotion) {
    throw std::runtime_error("Could't find any defined classes within file.");
  }

  if (skel) {
    LoadSkeleton(skel.get());
  }

  REBoneScanner.RescanBones();
  REBoneScanner.ResetScene();

  LoadMotion(cMotion.get());
}

int REEngineImport::DoImport(const TCHAR *fileName,
                             ImpInterface * /*importerInt*/, Interface * /*ip*/,
                             BOOL suppressPrompts) {
  SwapLocale();
  TSTRING filename_ = fileName;

  try {
    DoImport(std::to_string(filename_), suppressPrompts);
  } catch (const es::InvalidHeaderError &) {
    areCache.filename.clear();
    SwapLocale();
    return FALSE;
  } catch (const std::exception &e) {
    areCache.filename.clear();

    if (suppressPrompts) {
      printerror(e.what());
    } else {
      auto msg = ToTSTRING(e.what());
      MessageBox(GetActiveWindow(), msg.data(), _T("Exception thrown!"),
                 MB_ICONERROR | MB_OK);
    }
  } catch (...) {
    areCache.filename.clear();

    if (suppressPrompts) {
      printerror("Unhandled exception has been thrown!");
    } else {
      MessageBox(GetActiveWindow(), _T("Unhandled exception has been thrown!"),
                 _T("Exception thrown!"), MB_ICONERROR | MB_OK);
    }
  }

  if (checked[Checked::CH_NO_CACHE]) {
    areCache.filename.clear();
  }

  SwapLocale();

  return TRUE;
}
