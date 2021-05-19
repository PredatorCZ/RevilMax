/*    Revil Tool for 3ds Max
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
#include "datas/except.hpp"
#include "datas/master_printer.hpp"
#include "datas/reflector.hpp"
#include "datas/vectors_simd.hpp"
#include "revil/lmt.hpp"
#include <algorithm>
#include <iiksys.h>
#include <iksolver.h>
#include <map>
#include <memory>

#define MTFImport_CLASS_ID Class_ID(0x46f85524, 0xd4337f2)
static const TCHAR _className[] = _T("MTFImport");

class MTFImport : public SceneImport, RevilMax {
public:
  // Constructor/Destructor
  MTFImport();

  int ExtCount() override;                  // Number of extensions supported
  const TCHAR *Ext(int n) override;         // Extension #n (i.e. "3DS")
  const TCHAR *LongDesc() override;         // Long ASCII description
  const TCHAR *ShortDesc() override;        // Short ASCII description
  const TCHAR *AuthorName() override;       // ASCII Author name
  const TCHAR *CopyrightMessage() override; // ASCII Copyright message
  const TCHAR *OtherMessage1() override;    // Other message #1
  const TCHAR *OtherMessage2() override;    // Other message #2
  void ShowAbout(HWND hWnd) override;       // Show DLL's "About..." box
  unsigned int Version() override; // Version number * 100 (i.e. v3.01 = 301)
  int DoImport(const TCHAR *name, ImpInterface *i, Interface *gi,
               BOOL suppressPrompts = FALSE) override;

  void DoImport(const std::string &fileName, bool suppressPrompts);

  TimeValue LoadMotion(const uni::Motion &mot, TimeValue startTime = 0);
};

class : public ClassDesc2 {
public:
  virtual int IsPublic() { return TRUE; }
  virtual void *Create(BOOL) { return new MTFImport(); }
  virtual const TCHAR *ClassName() { return _className; }
  virtual SClass_ID SuperClassID() { return SCENE_IMPORT_CLASS_ID; }
  virtual Class_ID ClassID() { return MTFImport_CLASS_ID; }
  virtual const TCHAR *Category() { return NULL; }
  virtual const TCHAR *InternalName() { return _className; }
  virtual HINSTANCE HInstance() { return hInstance; }
  const TCHAR *NonLocalizedClassName() { return _className; }
} MTFImportDesc;

ClassDesc2 *GetMTFImportDesc() { return &MTFImportDesc; }

MTFImport::MTFImport() {}

int MTFImport::ExtCount() { return 5; }

const TCHAR *MTFImport::Ext(int n) {
  switch (n) {
  case 0:
    return _T("lmt");
  case 1:
    return _T("tml");
  case 2:
    return _T("mlx");
  case 3:
    return _T("mtx");
  case 4:
    return _T("mti");
  default:
    return nullptr;
  }

  return nullptr;
}

const TCHAR *MTFImport::LongDesc() { return _T("MT Framework Import"); }

const TCHAR *MTFImport::ShortDesc() { return _T("MT Framework Import"); }

const TCHAR *MTFImport::AuthorName() { return _T("Lukas Cone"); }

const TCHAR *MTFImport::CopyrightMessage() {
  return _T(RevilMax_COPYRIGHT "Lukas Cone");
}

const TCHAR *MTFImport::OtherMessage1() { return _T(""); }

const TCHAR *MTFImport::OtherMessage2() { return _T(""); }

unsigned int MTFImport::Version() { return REVILMAX_VERSIONINT; }

void MTFImport::ShowAbout(HWND hWnd) { ShowAboutDLG(hWnd); }

struct LMTNode : ReflectorInterface<LMTNode> {
  union {
    struct {
      Vector r1, r2, r3, r4;
    };
    Matrix3 mtx;
  };
  INode *nde;
  std::unique_ptr<LMTNode> ikTarget;
  int32 LMTBone = -3;

  INode *GetNode() { return ikTarget ? ikTarget->nde : nde; }

  LMTNode(INode *input) : nde(input) {
    ReflectorWrap<LMTNode> refl(this);
    const size_t numRefl = refl.GetNumReflectedValues();
    bool corrupted = false;

    for (size_t r = 0; r < numRefl; r++) {
      Reflector::KVPair reflPair = refl.GetReflectedPair(r);
      TSTRING usName = ToTSTRING(reflPair.name);

      if (!nde->UserPropExists(usName.c_str())) {
        corrupted = true;
        break;
      }

      MSTR value;
      nde->GetUserPropString(usName.c_str(), value);

      if (!value.length()) {
        corrupted = true;
        continue;
      }

      refl.SetReflectedValue(reflPair.name, std::to_string(value.data()));
    }

    if (LMTBone > 0) {
      BOOL isNub = 0;

      if (nde->GetUserPropBool(_T("isnub"), isNub)) {
        TSTRING bneName = nde->GetName();
        const size_t bneNameLen = bneName.size();

        if (bneName[bneNameLen - 1] == 'p' && bneName[bneNameLen - 2] == 's' &&
            bneName[bneNameLen - 3] == '_')
          bneName.resize(bneNameLen - 3);

        INode *ikNode = GetCOREInterface()->GetINodeByName(
            (bneName + _T("_IKTarget")).c_str());

        if (ikNode)
          ikTarget = std::unique_ptr<LMTNode>(new LMTNode(ikNode));
      }
    }

    if (!corrupted)
      return;

    Matrix3 pMat = nde->GetParentTM(0);
    pMat.Invert();

    mtx = nde->GetNodeTM(0) * pMat;

    for (int r = 0; r < numRefl; r++) {
      Reflector::KVPair reflPair = refl.GetReflectedPair(r);
      TSTRING usName = ToTSTRING(reflPair.name);
      TSTRING usVal = ToTSTRING(reflPair.value);

      nde->SetUserPropString(usName.c_str(), usVal.c_str());
    }
  }
};

REFLECTOR_CREATE(LMTNode, 1, VARNAMES, LMTBone, r1, r2, r3, r4);

static class : public ITreeEnumProc {
public:
  const MSTR boneNameHint = _T("LMTBone");

  std::vector<LMTNode> bones;

  void RescanBones() {
    bones.clear();
    GetCOREInterface7()->GetScene()->EnumTree(this);

    for (auto &b : bones) {
      if (b.LMTBone == -1)
        return;
    }

    for (auto &b : bones) {
      if (b.LMTBone == 255) {
        b.nde->SetUserPropInt(boneNameHint, -1);
        b.LMTBone = -1;
      }
    }
  }

  void RestoreBasePose(TimeValue atTime) {
    AnimateOn();

    for (auto &n : bones) {
      SetXFormPacket packet(n.mtx);
      n.nde->GetTMController()->SetValue(atTime, &packet);
    }

    AnimateOff();
  }

  void LockPose(TimeValue atTime) {
    for (auto &b : bones) {
      Matrix3 pMat = b.nde->GetParentTM(atTime);
      pMat.Invert();
      Matrix3 mtx = b.nde->GetNodeTM(atTime) * pMat;
      SetXFormPacket packet(mtx);
      AnimateOn();
      b.nde->GetTMController()->SetValue(atTime, &packet);
      AnimateOff();
    }
  }

  void ResetScene() {
    SuspendAnimate();

    for (auto &n : bones) {
      Control *cnt = n.nde->GetTMController();
      cnt->GetScaleController()->DeleteKeys(TRACK_DOALL | TRACK_RIGHTTOLEFT);
      cnt->GetRotationController()->DeleteKeys(TRACK_DOALL | TRACK_RIGHTTOLEFT);
      cnt->GetPositionController()->DeleteKeys(TRACK_DOALL | TRACK_RIGHTTOLEFT);
    }
  }

  void SetIKState(bool enabled) {
    for (auto &n : bones) {
      if (n.LMTBone != -3)
        continue;

      Control *ikCnt = n.nde->GetTMController();

      if (ikCnt->ClassID() == IKCHAINCONTROL_CLASS_ID) {
        IParamBlock2 *bck = ikCnt->GetParamBlock(IIKChainControl::kParamBlock);
        bck->SetValue(IIKChainControl::kAutoSnap, 0, 0);
        Control *enableCnt =
            (Control *)ikCnt->GetReference(IIKChainControl::kEnableRef);

        BOOL iEnabled = enabled;

        enableCnt->SetValue(0, &iEnabled);
      }
    }
  }

  void RestoreIKChains() {
    for (auto &n : bones) {
      if (n.LMTBone != -3)
        continue;

      Control *ikCnt = n.nde->GetTMController();

      if (ikCnt->ClassID() == IKCHAINCONTROL_CLASS_ID) {
        IParamBlock2 *bck = ikCnt->GetParamBlock(IIKChainControl::kParamBlock);
        INode *startJoint = bck->GetINode(IIKChainControl::kStartJoint);
        INode *endJoint = bck->GetINode(IIKChainControl::kEndJoint);
        int LMTID;
        bool updateIKSolver = false;

        if (startJoint->UserPropExists(boneNameHint)) {
          startJoint->GetUserPropInt(boneNameHint, LMTID);

          if (LMTID == -2) {
            ikCnt->ReplaceReference(IIKChainControl::kStartJointRef,
                                    startJoint->GetParentNode(), 0);
            updateIKSolver = true;
          }
        }

        if (endJoint->UserPropExists(boneNameHint)) {
          endJoint->GetUserPropInt(boneNameHint, LMTID);

          if (LMTID == -2) {
            ikCnt->ReplaceReference(IIKChainControl::kEndJointRef,
                                    endJoint->GetParentNode(), 0);
            updateIKSolver = true;
          }
        }

        if (updateIKSolver) {
          bck->SetValue(IIKChainControl::kSolverName, 0, _T("IKSolver"));
          bck->SetValue(IIKChainControl::kSolverName, 0, _T("IKHISolver"));
        }
      }
    }
  }

  LMTNode *LookupNode(int ID) {
    for (auto &b : bones) {
      if (b.LMTBone == ID)
        return &b;
    }

    return nullptr;
  }

  int callback(INode *node) {
    if (node->UserPropExists(boneNameHint)) {
      bones.push_back(node);
    }

    return TREE_CONTINUE;
  }
} iBoneScanner;

struct MTFTrackPair {
  INode *nde;
  const uni::MotionTrack *track;
  INode *scaleNode;
  MTFTrackPair *parent;
  std::vector<Vector4A16> frames;
  std::vector<MTFTrackPair *> children;

  MTFTrackPair(INode *inde, const uni::MotionTrack *tck,
               MTFTrackPair *prent = nullptr)
      : nde(inde), track(tck), scaleNode(nullptr), parent(prent) {}
};

typedef std::vector<MTFTrackPair> MTFTrackPairCnt;
typedef std::vector<TimeValue> Times;
typedef std::vector<float> Secs;

static bool IsRoot(MTFTrackPairCnt &collection, INode *item) {
  if (item->IsRootNode())
    return true;

  auto fnd = std::find_if(collection.begin(), collection.end(),
                          [item](const MTFTrackPair &i0) {
                            return i0.nde == item->GetParentNode();
                          });

  if (fnd != collection.end())
    return false;

  return IsRoot(collection, item->GetParentNode());
}

static INode *BuildScaleHandles(MTFTrackPairCnt &pairs, MTFTrackPair &item,
                                const Times &times) {
  INode *fNode = item.nde;
  int numChildren = fNode->NumberOfChildren();

  for (int c = 0; c < numChildren; c++) {
    int LMTIndex;
    INode *childNode = fNode->GetChildNode(c);

    if (childNode->GetUserPropInt(iBoneScanner.boneNameHint, LMTIndex) &&
        LMTIndex == -2) {
      item.scaleNode = childNode;
      break;
    }
  }

  if (!item.scaleNode) {
    INodeTab baseBone;
    INodeTab clonedBone;
    Point3 dummy;
    baseBone.AppendNode(fNode);

    GetCOREInterface()->CloneNodes(baseBone, dummy, false, NODE_COPY, nullptr,
                                   &clonedBone);
    item.scaleNode = fNode;
    item.nde = clonedBone[0];

    TSTRING bName = fNode->GetName();
    bName.append(_T("_sp"));

    item.nde->SetName(ToBoneName(bName));
    fNode->SetUserPropInt(iBoneScanner.boneNameHint, -2);
    fNode->GetParentNode()->AttachChild(item.nde);

    for (int c = 0; c < numChildren; c++)
      item.nde->AttachChild(fNode->GetChildNode(0));

    item.nde->AttachChild(fNode);
    fNode->SetUserPropString(_T("r1"), _T(""));
    fNode->SetUserPropString(_T("r2"), _T(""));
    fNode->SetUserPropString(_T("r3"), _T(""));
    fNode->SetUserPropString(_T("r4"), _T(""));
  }

  item.frames.resize(times.size(), Vector4A16(1.f));

  fNode = item.nde;
  numChildren = fNode->NumberOfChildren();

  std::vector<INode *> childNodes;

  for (int c = 0; c < numChildren; c++) {
    INode *childNode = fNode->GetChildNode(c);

    if (childNode == item.scaleNode)
      continue;

    childNodes.push_back(childNode);
  }

  for (auto c : childNodes) {
    const uni::MotionTrack *foundTrack = nullptr;

    for (auto &t : pairs)
      if (t.nde == c || t.scaleNode == c) {
        foundTrack = t.track;
        break;
      }

    MTFTrackPair *nChild = new MTFTrackPair(c, foundTrack, &item);
    item.children.push_back(nChild);
    BuildScaleHandles(pairs, **std::prev(item.children.end()), times);
  }

  return fNode;
}

static void PopulateScaleData(MTFTrackPair &item, const Times &times,
                              const Secs &secs) {
  if (!item.scaleNode)
    return;

  const size_t numKeys = times.size();
  Control *cnt = item.scaleNode->GetTMController();

  AnimateOn();

  if (item.track) {
    for (int t = 0; t < numKeys; t++) {
      Vector4A16 cVal;
      item.track->GetValue(cVal, secs[t]);
      item.frames[t] *= cVal;

      if (item.parent)
        item.frames[t] *= item.parent->frames[t];

      Matrix3 cMat(1);
      cMat.SetScale(
          Point3(item.frames[t].X, item.frames[t].Y, item.frames[t].Z));

      SetXFormPacket packet(cMat);

      cnt->SetValue(times[t], &packet);
    }
  }

  AnimateOff();

  for (auto &c : item.children)
    PopulateScaleData(*c, times, secs);
}

static void
FixupHierarchialTranslations(INode *nde, const Times &times,
                             const std::vector<Vector4A16> &scaleValues,
                             const std::vector<Matrix3> *parentTransforms) {
  const size_t numKeys = times.size();
  const int numChildren = nde->NumberOfChildren();
  std::vector<Point3> values;
  values.resize(numKeys);

  for (int c = 0; c < numChildren; c++) {
    INode *childNode = nde->GetChildNode(c);
    int LMTIndex;

    if (childNode->GetUserPropInt(iBoneScanner.boneNameHint, LMTIndex) &&
        LMTIndex == -2)
      continue;

    for (int t = 0; t < numKeys; t++) {
      Matrix3 rVal = childNode->GetNodeTM(times[t]);
      Matrix3 pVal = nde->GetNodeTM(times[t]);
      pVal.Invert();
      rVal *= pVal;
      values[t] = rVal.GetTrans() *
                  Point3(scaleValues[t].X, scaleValues[t].Y, scaleValues[t].Z);
    }

    Control *cnt = childNode->GetTMController()->GetPositionController();

    AnimateOn();

    for (int t = 0; t < numKeys; t++) {
      cnt->SetValue(times[t], &values[t]);
    }

    AnimateOff();
  }
}

static void ScaleTranslations(MTFTrackPair &item, const Times &times) {
  if (!item.scaleNode)
    return;

  std::vector<Matrix3> absValues;
  absValues.reserve(times.size());

  for (auto t : times)
    absValues.push_back(item.nde->GetNodeTM(t));

  FixupHierarchialTranslations(item.nde, times, item.frames, &absValues);

  for (auto &c : item.children) {
    ScaleTranslations(*c, times);
  }
}

TimeValue MTFImport::LoadMotion(const uni::Motion &mot, TimeValue startTime) {
  const float aDuration = mot.Duration();

  TimeValue numTicks = SecToTicks(aDuration);
  const TimeValue ticksPerFrame = GetTicksPerFrame();
  TimeValue overlappingTicks = numTicks % ticksPerFrame;

  if (overlappingTicks > (ticksPerFrame / 2))
    numTicks += ticksPerFrame - overlappingTicks;
  else
    numTicks -= overlappingTicks;

  Interval aniRange(startTime, startTime + numTicks - ticksPerFrame);
  std::vector<float> frameTimes;
  Times frameTimesTicks;

  for (TimeValue v = aniRange.Start(); v <= aniRange.End();
       v += ticksPerFrame) {
    frameTimes.push_back(TicksToSec(v - startTime));
    frameTimesTicks.push_back(v);
  }

  const size_t numFrames = frameTimes.size();
  std::vector<MTFTrackPair> scaleTracks;

  for (auto &t : mot) {
    const size_t boneID = t->BoneIndex();
    LMTNode *lNode = iBoneScanner.LookupNode(boneID);

    if (!lNode)
      continue;

    if (t->TrackType() == uni::MotionTrack::TrackType_e::Scale)
      scaleTracks.emplace_back(lNode->nde, t.get());

    /*if (t.BoneType())
      printline("Bone: " << es::ToUTF8(lNode->nde->GetName())
                          << ", RuleID: " << tck->BoneType());*/
  }

  std::vector<MTFTrackPair *> rootsOnly;

  for (auto &s : scaleTracks)
    if (IsRoot(scaleTracks, s.nde))
      rootsOnly.push_back(&s);

  for (auto &s : rootsOnly)
    BuildScaleHandles(scaleTracks, *s, frameTimesTicks);

  iBoneScanner.RescanBones();
  iBoneScanner.RestoreBasePose(startTime);
  const bool additive = checked[Checked::CH_ADDITIVE];

  for (auto &t : mot) {
    const int32 boneID = t->BoneIndex();
    LMTNode *lNode = iBoneScanner.LookupNode(boneID);

    if (!lNode) {
      if (!checked[Checked::CH_NOLOGBONES]) {
        printwarning("[MTF] Couldn't find LMTBone: " << boneID);
      }
      continue;
    }

    INode *fNode =
        !checked[Checked::CH_DISABLEIK] ? lNode->GetNode() : lNode->nde;
    Control *cnt = fNode->GetTMController();

    switch (t->TrackType()) {
    case uni::MotionTrack::TrackType_e::Position: {
      Control *posCnt = cnt->GetPositionController();
      Point3 additivum;

      if (additive) {
        posCnt->GetValue(-1, &additivum, FOREVER);
      }

      AnimateOn();

      for (int i = 0; i < numFrames; i++) {
        Vector4A16 cVal;
        t->GetValue(cVal, frameTimes[i]);
        cVal *= objectScale;
        Point3 kVal = reinterpret_cast<Point3 &>(cVal);

        if (fNode->GetParentNode()->IsRootNode() && !additive)
          kVal = corMat.PointTransform(kVal);

        kVal += additivum;
        posCnt->SetValue(frameTimesTicks[i], &kVal);
      }

      AnimateOff();
      break;
    }
    case uni::MotionTrack::TrackType_e::Rotation: {
      Control *rotCnt = cnt->GetRotationController();
      Quat additivum;

      if (additive) {
        rotCnt->GetValue(-1, &additivum, FOREVER);
      }
      AnimateOn();

      for (int i = 0; i < numFrames; i++) {
        Vector4A16 cVal;
        t->GetValue(cVal, frameTimes[i]);
        Quat kVal = reinterpret_cast<Quat &>(cVal).Conjugate();

        if (fNode->GetParentNode()->IsRootNode() && !additive) {
          Matrix3 cMat;
          cMat.SetRotate(kVal);
          kVal = cMat * corMat;
        }

        kVal += additivum;
        rotCnt->SetValue(frameTimesTicks[i], &kVal);
      }

      AnimateOff();
      break;
    }
    default:
      break;
    }
  }

  for (auto &s : rootsOnly)
    PopulateScaleData(*s, frameTimesTicks, frameTimes);

  for (auto &s : rootsOnly)
    ScaleTranslations(*s, frameTimesTicks);

  if (aniRange.Start() == aniRange.End()) {
    aniRange.SetEnd(aniRange.End() + ticksPerFrame);
  }

  GetCOREInterface()->SetAnimRange(aniRange);
  return aniRange.End() + ticksPerFrame;
}

void SwapLocale();
static struct {
  revil::LMT asset;
  std::string filename;
} lmtCache;

void MTFImport::DoImport(const std::string &fileName, bool suppressPrompts) {
  if (lmtCache.filename != fileName) {
    es::Dispose(lmtCache.asset);
    lmtCache.asset.Load(fileName);
    lmtCache.filename = fileName;
  }

  size_t curMotionID = 0;
  uni::MotionsConst motions = lmtCache.asset;

  for (auto &m : *motions) {
    if (m) {
      motionNames.push_back(ToTSTRING(curMotionID));
    } else {
      motionNames.push_back(_T("--[Empty]--"));
    }

    curMotionID++;
  }

  instanceDialogType = DLGTYPE_LMT;

  if (!suppressPrompts) {
    if (!SpawnDialog()) {
      return;
    }
  }

  GetCOREInterface()->ClearNodeSelection();

  iBoneScanner.RescanBones();
  iBoneScanner.ResetScene();
  iBoneScanner.SetIKState(!checked[Checked::CH_DISABLEIK]);
  const int32 frameRate = 30 * (frameRateIndex + 1);

  if (!checked[Checked::CH_RESAMPLE]) {
    SetFrameRate(frameRate);
  }

  if (checked[Checked::RD_ANISEL]) {
    auto mot = motions->At(motionIndex);

    if (!mot) {
      return;
    }

    mot->FrameRate(frameRate);
    LoadMotion(*mot, 0);
  } else {
    TimeValue lastTime = 0;
    size_t i = 0;

    printline("Sequencer not found, dumping animation ranges (in tick units):");

    for (auto &a : *motions) {

      if (!a) {
        i++;
        continue;
      }

      a->FrameRate(frameRate);

      TimeValue nextTime = LoadMotion(*a.get(), lastTime);
      printer << std::to_string(motionNames[i]) << ": " << lastTime << ", "
              << nextTime;
      const auto &_a = static_cast<const revil::LMTAnimation &>(*a.get());

      if (_a.LoopFrame() > 0)
        printer << ", loopFrame: "
                << SecToTicks(_a.LoopFrame() / float(frameRate));

      printer >> 1;
      lastTime = nextTime;
      iBoneScanner.LockPose(nextTime - GetTicksPerFrame());

      i++;
    }
  }

  iBoneScanner.RescanBones();
  iBoneScanner.RestoreBasePose(-1);
  iBoneScanner.RestoreIKChains();
}

int MTFImport::DoImport(const TCHAR *fileName, ImpInterface * /*importerInt*/,
                        Interface * /*ip*/, BOOL suppressPrompts) {
  SwapLocale();

  TSTRING filename_ = fileName;

  try {
    DoImport(std::to_string(filename_), suppressPrompts);
  } catch (const es::InvalidHeaderError &) {
    lmtCache.filename.clear();
    SwapLocale();
    return FALSE;
  } catch (const std::exception &e) {
    lmtCache.filename.clear();

    if (suppressPrompts) {
      printerror(e.what());
    } else {
      auto msg = ToTSTRING(e.what());
      MessageBox(GetActiveWindow(), msg.data(), _T("Exception thrown!"),
                 MB_ICONERROR | MB_OK);
    }
  } catch (...) {
    lmtCache.filename.clear();

    if (suppressPrompts) {
      printerror("Unhandled exception has been thrown!");
    } else {
      MessageBox(GetActiveWindow(), _T("Unhandled exception has been thrown!"),
                 _T("Exception thrown!"), MB_ICONERROR | MB_OK);
    }
  }

  if (checked[Checked::CH_NO_CACHE]) {
    lmtCache.filename.clear();
  }

  SwapLocale();

  return TRUE;
}
