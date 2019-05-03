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

#include <map>
#include "RevilMax.h"
#include "datas/esstring.h"
#include "REAsset.h"
#include "datas/masterprinter.hpp"

#define REEngineImport_CLASS_ID	Class_ID(0x373d264a, 0x90c37b7)
static const TCHAR _className[] = _T("REEngineImport");

class REEngineImport : public SceneImport, RevilMax
{
public:
	//Constructor/Destructor
	REEngineImport();
	virtual ~REEngineImport() {}

	virtual int				ExtCount();					// Number of extensions supported
	virtual const TCHAR *	Ext(int n);					// Extension #n (i.e. "3DS")
	virtual const TCHAR *	LongDesc();					// Long ASCII description (i.e. "Autodesk 3D Studio File")
	virtual const TCHAR *	ShortDesc();				// Short ASCII description (i.e. "3D Studio")
	virtual const TCHAR *	AuthorName();				// ASCII Author name
	virtual const TCHAR *	CopyrightMessage();			// ASCII Copyright message
	virtual const TCHAR *	OtherMessage1();			// Other message #1
	virtual const TCHAR *	OtherMessage2();			// Other message #2
	virtual unsigned int	Version();					// Version number * 100 (i.e. v3.01 = 301)
	virtual void			ShowAbout(HWND hWnd);		// Show DLL's "About..." box
	virtual int				DoImport(const TCHAR *name,ImpInterface *i,Interface *gi, BOOL suppressPrompts=FALSE);	// Import file

	void LoadMotion(REMotion &mot, TimeValue startTime = 0);
};

class : public ClassDesc2 
{
public:
	virtual int				IsPublic() 		{ return TRUE; }
	virtual void *			Create(BOOL) 	{ return new REEngineImport(); }
	virtual const TCHAR *	ClassName() 	{ return _className; }
	virtual SClass_ID		SuperClassID() 	{ return SCENE_IMPORT_CLASS_ID; }
	virtual Class_ID		ClassID() 		{ return REEngineImport_CLASS_ID; }
	virtual const TCHAR *	Category() 		{ return NULL; }
	virtual const TCHAR *	InternalName() 	{ return _className; }
	virtual HINSTANCE		HInstance() 	{ return hInstance; }
}REEngineImportDesc;

ClassDesc2* GetREEngineImportDesc() { return &REEngineImportDesc; }

//--- HavokImp -------------------------------------------------------
REEngineImport::REEngineImport()
{
}

int REEngineImport::ExtCount()
{
	return 4;
}

const TCHAR *REEngineImport::Ext(int n)
{
	switch (n)
	{
	case 0:
		return _T("motlist.85");
	case 1:
		return _T("motlist");
	case 2:
		return _T("mot.65");
	case 3:
		return _T("mot");
	default:
		return nullptr;
	}

	return nullptr;
}

const TCHAR *REEngineImport::LongDesc()
{
	return _T("RE Engine Import");
}

const TCHAR *REEngineImport::ShortDesc()
{
	return _T("RE Engine Import");
}

const TCHAR *REEngineImport::AuthorName()
{
	return _T("Lukas Cone");
}

const TCHAR *REEngineImport::CopyrightMessage()
{
	return _T("Copyright (C) 2019 Lukas Cone");
}

const TCHAR *REEngineImport::OtherMessage1()
{		
	return _T("");
}

const TCHAR *REEngineImport::OtherMessage2()
{		
	return _T("");
}

unsigned int REEngineImport::Version()
{				
	return 100;
}

void REEngineImport::ShowAbout(HWND hWnd)
{
	ShowAboutDLG(hWnd);
}

void REEngineImport::LoadMotion(REMotion &mot, TimeValue startTime)
{
	const int numBones = mot.numBones;
	const int numTracks = mot.numTracks;

	std::map<int, INode *> hashedNodes;

	for (int b = 0; b < numBones; b++)
	{
		REMotionBone &cBone = mot.bones->ptr[b];
		TSTRING boneName = esString(reinterpret_cast<wchar_t*>(cBone.boneName.ptr));
		INode *node = GetCOREInterface()->GetINodeByName(boneName.c_str());

		if (!node)
		{
			Object *obj = static_cast<Object *>(CreateInstance(HELPER_CLASS_ID, Class_ID(DUMMY_CLASS_ID, 0)));
			node = GetCOREInterface()->CreateObjectNode(obj);
			node->ShowBone(2);
			node->SetWireColor(0x80ff);
			node->SetName(ToBoneName(boneName));
		}

		Matrix3 nodeTM = {};
		nodeTM.SetRotate(reinterpret_cast<const Quat &>(cBone.rotation).Conjugate());
		nodeTM.SetTrans(reinterpret_cast<const Point3 &>(cBone.position) * IDC_EDIT_SCALE_value);

		if (cBone.parentBoneNamePtr.ptr)
		{
			INode *pNode = GetCOREInterface()->GetINodeByName(esStringConvert<TCHAR>(reinterpret_cast<wchar_t *>(*cBone.parentBoneNamePtr.ptr)).c_str());

			if (pNode)
			{
				pNode->AttachChild(node);
				nodeTM *= pNode->GetNodeTM(0);
			}
		}
		else
			nodeTM *= corMat;

		Control *cnt = node->GetTMController();

		if (cnt->GetPositionController()->ClassID() != Class_ID(LININTERP_POSITION_CLASS_ID, 0))
			cnt->SetPositionController((Control *)CreateInstance(CTRL_POSITION_CLASS_ID, Class_ID(LININTERP_POSITION_CLASS_ID, 0)));

		if (cnt->GetRotationController()->ClassID() != Class_ID(LININTERP_ROTATION_CLASS_ID, 0))
			cnt->SetRotationController((Control *)CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(LININTERP_ROTATION_CLASS_ID, 0)));

		if (cnt->GetScaleController()->ClassID() != Class_ID(LININTERP_SCALE_CLASS_ID, 0))
			cnt->SetScaleController((Control *)CreateInstance(CTRL_SCALE_CLASS_ID, Class_ID(LININTERP_SCALE_CLASS_ID, 0)));

		node->SetNodeTM(startTime, nodeTM);
		node->SetUserPropString(_T("BoneHash"), ToTSTRING(cBone.boneHash).c_str());
		hashedNodes[cBone.boneHash] = node;
	}

	GetCOREInterface()->SetAnimRange(Interval(startTime, (mot.intervals[0] * mot.framesPerSecond) + startTime));
	SetFrameRate(mot.framesPerSecond);

	for (int t = 0; t < numTracks; t++)
	{
		REMotionTrack &cTrack = mot.tracks[t];

		if (!hashedNodes.count(cTrack.boneHash))
			continue;

		INode *cNode = hashedNodes.at(cTrack.boneHash);
		Control *cnt = cNode->GetTMController();

		int currentCurve = 0;

		if (cTrack.usedCurves[REMotionTrack::TrackType_Position])
		{
			RETrackCurve &curve = cTrack.curves[currentCurve++];
			RETrackController *tController = curve.GetController();
			Control *posControl = cnt->GetPositionController();
			IKeyControl *kCon = GetKeyControlInterface(posControl);

			if (tController)
			{
				kCon->SetNumKeys(curve.numFrames);

				for (int f = 0; f < curve.numFrames; f++)
				{
					TimeValue atTime = startTime + tController->GetFrame(f) * curve.framesPerSecond;

					Vector4 cVal;
					tController->Evaluate(f, cVal);
					
					ILinPoint3Key cKey;
					cKey.time = atTime;
					cKey.val = reinterpret_cast<Point3 &>(cVal) * IDC_EDIT_SCALE_value;

					if (cNode->GetParentNode()->IsRootNode())
						cKey.val = corMat.PointTransform(cKey.val);

					kCon->SetKey(f, &cKey);
				}

				delete tController;
			}
		}

		if (cTrack.usedCurves[REMotionTrack::TrackType_Rotation])
		{
			RETrackCurve &curve = cTrack.curves[currentCurve++];
			RETrackController *tController = curve.GetController();
			Control *rotateControl = (Control *)CreateInstance(CTRL_ROTATION_CLASS_ID, Class_ID(HYBRIDINTERP_ROTATION_CLASS_ID, 0));
			IKeyControl *kCon = GetKeyControlInterface(rotateControl);

			if (tController)
			{
				kCon->SetNumKeys(curve.numFrames);
				for (int f = 0; f < curve.numFrames; f++)
				{
					TimeValue atTime = startTime + tController->GetFrame(f) * curve.framesPerSecond;

					Vector4 cVal;
					tController->Evaluate(f, cVal);

					ILinRotKey cKey;
					cKey.time = atTime;
					cKey.val = reinterpret_cast<Quat &>(cVal).Conjugate();

					if (cNode->GetParentNode()->IsRootNode())
					{
						Matrix3 cMat;
						cMat.SetRotate(cKey.val);
						cKey.val = cMat * corMat;
					}					

					kCon->SetKey(f, &cKey);
				}

				delete tController;
			}

			cnt->GetRotationController()->Copy(rotateControl); //Gimbal fix
		}

		if (cTrack.usedCurves[REMotionTrack::TrackType_Scale])
		{
			RETrackCurve &curve = cTrack.curves[currentCurve++];
			RETrackController *tController = curve.GetController();
			Control *scaleControl = cnt->GetScaleController();
			IKeyControl *kCon = GetKeyControlInterface(scaleControl);

			if (tController)
			{
				kCon->SetNumKeys(curve.numFrames);

				for (int f = 0; f < curve.numFrames; f++)
				{
					TimeValue atTime = startTime + tController->GetFrame(f) * curve.framesPerSecond;

					Vector4 cVal;
					tController->Evaluate(f, cVal);

					ILinPoint3Key cKey;
					cKey.time = atTime;
					cKey.val = reinterpret_cast<Point3 &>(cVal);

					if (cNode->GetParentNode()->IsRootNode())
						cKey.val = corMat.PointTransform(cKey.val);

					kCon->SetKey(f, &cKey);
				}

				delete tController;
			}
		}
	}
}

int REEngineImport::DoImport(const TCHAR*fileName, ImpInterface* /*importerInt*/, Interface* /*ip*/, BOOL suppressPrompts)
{
	char *oldLocale = setlocale(LC_NUMERIC, NULL);
	setlocale(LC_NUMERIC, "en-US");

	TSTRING _filename = fileName;
	REAsset mainAsset;

	if (mainAsset.Load(fileName))
		return FALSE;

	if (mainAsset.IsObject<REMotlist>())
	{
		REMotlist *mlst = mainAsset.Object<REMotlist>();

		if (mlst->numMotions > 1)
			for (int m = 0; m < mlst->numMotions; m++)
			{
				REMotion *cMot = static_cast<REMotion *>(mlst->motions[m].ptr);
				if (cMot)
					motionNames.push_back(esStringConvert<TCHAR>(reinterpret_cast<wchar_t *>(cMot->animationName.ptr)));
				else
					motionNames.push_back(_T("--[Error Type]--"));
			}

		if (!suppressPrompts)
			if (!SpawnDialog())
				return TRUE;

		REMotion *mot = static_cast<REMotion *>(mlst->motions[IDC_CB_MOTION_index].ptr);

		if (!mot)
			return FALSE;

		LoadMotion(*mot);

	}
	else if (mainAsset.IsObject<REMotion>())
	{
		if (!suppressPrompts)
			if (!SpawnDialog())
				return TRUE;

		LoadMotion(*mainAsset.Object<REMotion>());
	}
	else
	{
		MessageBox(GetActiveWindow(), _T("Could't find any defined classes within file."), _T("Undefined file."), MB_ICONSTOP);
		return TRUE;
	}

	setlocale(LC_NUMERIC, oldLocale);

	return TRUE;
}
	
