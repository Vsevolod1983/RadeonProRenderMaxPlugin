/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

#include "FireRenderPhysicalLight.h"
#include <iparamb2.h>
#include <iparamm2.h>
#include <maxscript\ui\rollouts.h>
#include <unordered_map>
#include "Common.h"
#include "utils/KelvinToColor.h"
#include "Resource.h"

FIRERENDER_NAMESPACE_BEGIN

namespace
{
	// General
	static const float AreaLengthsMin = 0.0f;
	static const float AreaLengthsDefault = 25.0f;
	static const float AreaLengthsMax = FLT_MAX;
	static const float AreaWidthsMin = 0.0f;
	static const float AreaWidthsDefault = 25.0f;
	static const float AreaWidthsMax = FLT_MAX;
	static const float DistToTargetMin = 0.0f;
	static const float DistToTargetDefault = 1.0f;
	static const float DistToTargetMax = FLT_MAX;

	// Intensity
	static const float LightIntensityMin = 0.0f;
	static const float LightIntensityMax = FLT_MAX;
	static const float LightIntensityDefault = 1000.0f;
	static const float LuminousEfficacyMin = 0.0f;
	static const float LuminousEfficacyDefault = 17.0f;
	static const float LuminousEfficacyMax = FLT_MAX;
	static const Color IntensityColour(0.5f, 0.5f, 0.5f);
	static const float ColourTemperatureMin = 0.0f;
	static const float ColourTemperatureDefault = 6500.0f;
	static const float ColourTemperatureMax = FLT_MAX;
	static const Color TemperatureColour(1.0f, 1.0f, 1.0f);

	// Area Light


	// Spot Light
	static const float InnerConeAngleMin = 0.0f;
	static const float InnerConeAngleDefault = 10.0f;
	static const float InnerConeAngleMax = 360.0f;
	static const float OuterConeFalloffMin = 0.0f;
	static const float OuterConeFalloffDefault = 30.0f;
	static const float OuterConeFalloffMax = 360.0f;

	// Light Decay
	static const float DecayFalloffStartMin = 0.0f;
	static const float DecayFalloffStartDefault = 1.0f;
	static const float DecayFalloffStartMax = FLT_MAX;
	static const float DecayFalloffEndMin = 0.0f;
	static const float DecayFalloffEndDefault = 1.0f;
	static const float DecayFalloffEndMax = FLT_MAX;

	// Shadows
	static const float ShadowsSoftnessMin = 0.0f;
	static const float ShadowsSoftnessDefault = 1.0f;
	static const float ShadowsSoftnessMax = FLT_MAX;
	static const float ShadowsTransparencyMin = 0.0f;
	static const float ShadowsTransparencyDefault = 1.0f;
	static const float ShadowsTransparencyMax = FLT_MAX;

	// Volume
	static const float VolumeScaleMin = 0.0f;
	static const float VolumeScaleDefault = 1.0f;
	static const float VolumeScaleMax = FLT_MAX;
}

static std::unordered_map<FRPhysicalLight_RolloutID, TSTR> rolloutWindowsTable =
{
	{ ROLLOUT_GENERAL,    _T("General") },
	{ ROLLOUT_INTENSITY,  _T("Intensity") },
	{ ROLLOUT_AREALIGHT,  _T("Light Area") },
	{ ROLLOUT_SPOTLIGHT,  _T("Spot Light") },
	//{ROLLOUT_LIGHTDECAY, _T("Light Decay") },
	//{ ROLLOUT_SHADOWS,    _T("Light Shadows") },
	//{ ROLLOUT_VOLUME,     _T("Light Volume") },
	{ ROLLOUT_MAX,        _T("Dummy") },
};

void UpdatePhysLightUI(IParamBlock2* pBlock);

// commented out because GetPanelTitle(int) function doesn't exist in MAX 2016 SDK
// will be restored when we will eventually drop support of MAX 2016
/*int GetRolloutWindowActualIndex(PhysLightRolloutWindow rollWindowID, IRollupWindow* pRollWindow)
{
	TSTR rollWindowName = rolloutWindowsTable[rollWindowID];

	for (int idx = 0; idx < pRollWindow->GetNumPanels(); ++idx) // ROLLOUT_MAX + shift?
		if (pRollWindow->GetPanelTitle(idx) == rollWindowName)
			return idx;
	
	FASSERT(false); // failed

	return -1; // dummy return for compiler
}*/

// dummy function that will be used until we drop support of MAX 2016
int GetRolloutWindowActualIndex(FRPhysicalLight_RolloutID rollWindowID, IRollupWindow* pRollWindow)
{
	return rollWindowID;
}

const int ROLLOUT_MAX_GENERAL_SHIFT = 2;
const int ROLLOUTS_PHYS_LIGHT_COUNT = 7;
const float DEFAULT_DIST_TO_TARGET = 5.0f;
static const int VERSION = 2;

PhysLightGeneralParamsDlgProc dlgProcPhysLightGeneralParams;
PhysLightsAreaLightsDlgProc dlgProcPhysLightsAreaLights;
PhysLightsSpotLightsDlgProc dlgProcPhysLightsSpotLights;
PhysLightIntensityDlgProc dlgProcPhysLightIntensity;
PhysLightsVolumeDlgProc dlgProcPhysLightsVolume;

const TCHAR* FIRERENDER_PHYSICALLIGHT_CATEGORY = _T("Radeon ProRender");
const TCHAR* FIRERENDER_PHYSICALLIGHT_INTERNAL_NAME = _T("Physical Light");
const TCHAR* FIRERENDER_PHYSICALLIGHT_CLASS_NAME = _T("Physical Light");

class FireRenderPhysicalLightClassDesc : public ClassDesc2
{
public:
	int IsPublic(void) override
	{
		return 1; // DEACTIVATED! SET TO 1 TO REACTIVATE.
	}

	HINSTANCE HInstance(void) override
	{
		FASSERT(fireRenderHInstance != NULL);
		return fireRenderHInstance;
	}

	//used in MaxScript to create objects of this class
	const TCHAR* InternalName(void) override
	{
		return FIRERENDER_PHYSICALLIGHT_INTERNAL_NAME;
	}

	//used as lable in creation UI and in MaxScript to create objects of this class
	const TCHAR* ClassName(void) override
	{
		return FIRERENDER_PHYSICALLIGHT_CLASS_NAME;
	}

	Class_ID ClassID(void) override
	{
		return FIRERENDER_PHYSICALLIGHT_CLASS_ID;
	}

	SClass_ID SuperClassID(void) override
	{
		return LIGHT_CLASS_ID;
	}

	const TCHAR* Category(void) override
	{
		return FIRERENDER_PHYSICALLIGHT_CATEGORY;
	}

	void* Create(BOOL loading) override;
};

static FireRenderPhysicalLightClassDesc desc;

class PickNodeAreaLightMeshValidator : public PBValidator
{
	BOOL Validate(PB2Value& v)
	{
		INode* pNode = static_cast<INode*>(v.r);
		if (pNode == nullptr)
		{
			return FALSE;
		}

		Object* pObj = pNode->GetObjectRef();
		if (pObj == nullptr)
		{
			return FALSE;
		}

		int numFaces = 0;
		int numVerts = 0;
		TimeValue time = GetCOREInterface()->GetTime();
		BOOL havePolys = pObj->PolygonCount(time, numFaces, numVerts);
		if (!havePolys || (numFaces == 0))
		{
			return FALSE;
		}

		return TRUE;
	}
};

static PickNodeAreaLightMeshValidator physlightMeshValidator;

class IntensityColourPickerAccessor : public PBAccessor
{
	// read-only
	void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		return;
	}
};

static IntensityColourPickerAccessor physlightTColourAccessor;

class PickNodeAreaLightMeshAccessor : public PBAccessor
{
	virtual void Set(PB2Value& v, ReferenceMaker* owner, ParamID id, int tabIndex, TimeValue t)
	{
		// process selested node (make it disabled)
		ReferenceTarget* pRef = v.r;
		FASSERT(pRef);
		INode* pMeshNode = static_cast<INode*>(pRef);
		FASSERT(pMeshNode);
		pMeshNode->Hide(true);

		// move light node root to position of selected scene object
		if (INode* pOwnerNode = GetNodeRef(owner))
		{
			Matrix3 meshTM = pMeshNode->GetNodeTM(t);
			TimeValue time = GetCOREInterface()->GetTime();
			pOwnerNode->SetNodeTM(t, meshTM);
			pOwnerNode->InvalidateTM();
		}
	}
};

static PickNodeAreaLightMeshAccessor physlightMeshAccessor;

ClassDesc2* GetFireRenderPhysicalLightDesc(void)
{
	return &desc;
}

ClassDesc2* FireRenderPhysicalLight::GetClassDesc()
{
	return &desc;
}

void *FireRenderPhysicalLightClassDesc::Create(BOOL loading)
{
	FireRenderPhysicalLight* pLight = new FireRenderPhysicalLight();

	if (loading)
	{
		pLight->FinishPreview();
	}

	return pLight;
}

static ParamBlockDesc2 pbDesc(
	0, _T("PhysicalLightPbdesc"), 0, &desc, P_AUTO_CONSTRUCT | P_AUTO_UI | P_MULTIMAP | P_VERSION,
	VERSION,  // for P_VERSION
	0, //for P_AUTO_CONSTRUCT

	// rollouts (P_AUTO_UI)
	ROLLOUT_MAX,
	ROLLOUT_GENERAL, IDD_FIRERENDER_PHYS_LIGHT_GENERAL, IDS_PHYS_LIGHT_GENERAL, 0, 0, &dlgProcPhysLightGeneralParams,
	ROLLOUT_INTENSITY, IDD_FIRERENDER_PHYS_LIGHT_INTENSITY, IDS_PHYS_LIGHT_INTENSITY, /*ROC_INVISIBLE*/ 0, APPENDROLL_CLOSED, &dlgProcPhysLightIntensity,
	ROLLOUT_AREALIGHT, IDD_FIRERENDER_PHYS_LIGHT_AREA, IDS_PHYS_LIGHT_AREA, 0, DONTAUTOCLOSE, &dlgProcPhysLightsAreaLights,
	ROLLOUT_SPOTLIGHT, IDD_FIRERENDER_PHYS_LIGHT_SPOT, IDS_PHYS_LIGHT_SPOT, 0, APPENDROLL_CLOSED, &dlgProcPhysLightsSpotLights,
	//ROLLOUT_LIGHTDECAY, IDD_FIRERENDER_PHYS_LIGHT_DECAY, IDS_PHYS_LIGHT_DECAY, 0, APPENDROLL_CLOSED, NULL,
	//ROLLOUT_SHADOWS, IDD_FIRERENDER_PHYS_LIGHT_SHADOWS, IDS_PHYS_LIGHT_SHADOWS, 0, APPENDROLL_CLOSED, NULL,
	//ROLLOUT_VOLUME, IDD_FIRERENDER_PHYS_LIGHT_VOLUME, IDS_PHYS_LIGHT_VOLUME, 0, APPENDROLL_CLOSED, &dlgProcPhysLightsVolume,

	// General
	//IDC_FIRERENDER_PHYS_LIGHT_ENABLED
	FRPhysicalLight_ISENABLED, _T("IsEnabled"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, TRUE,
	p_ui, ROLLOUT_GENERAL,
	TYPE_SINGLECHEKBOX, IDC_FIRERENDER_PHYS_LIGHT_ENABLED,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_TYPE
	FRPhysicalLight_LIGHT_TYPE, _T("LightType"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, FRPhysicalLight_AREA,
	p_ui, ROLLOUT_GENERAL,
	TYPE_INT_COMBOBOX, IDC_FIRERENDER_PHYS_LIGHT_TYPE,
	4, IDS_PHYS_LIGHT_TYPE_AREA, IDS_PHYS_LIGHT_TYPE_SPOT, IDS_PHYS_LIGHT_TYPE_POINT, IDS_PHYS_LIGHT_TYPE_DIRECTIONAL,
	p_vals, FRPhysicalLight_AREA, FRPhysicalLight_SPOT, FRPhysicalLight_POINT, FRPhysicalLight_DIRECTIONAL,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_AREA_LENGTHS, IDC_FIRERENDER_PHYS_LIGHT_AREA_LENGTHS_S
	FRPhysicalLight_AREA_LENGTHS, _T("AreaLength"), TYPE_FLOAT, P_ANIMATABLE, 0,  //change to "Length" to match UI?
	p_default, AreaLengthsDefault,
	p_range, AreaLengthsMin, AreaLengthsMax,
	p_ui, ROLLOUT_AREALIGHT,
	TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_AREA_LENGTHS, IDC_FIRERENDER_PHYS_LIGHT_AREA_LENGTHS_S, SPIN_AUTOSCALE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH, IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH_S
	FRPhysicalLight_AREA_WIDTHS, _T("AreaWidths"), TYPE_FLOAT, P_ANIMATABLE, 0,   //change to "Width" to match UI?
	p_default, AreaWidthsDefault,
	p_range, AreaWidthsMin, AreaWidthsMax,
	p_ui, ROLLOUT_AREALIGHT,
	TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH, IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH_S, SPIN_AUTOSCALE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_TARGETED
	FRPhysicalLight_IS_TARGETED, _T("IsTargeted"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, ROLLOUT_GENERAL,
	TYPE_SINGLECHEKBOX, IDC_FIRERENDER_PHYS_LIGHT_TARGETED,
	p_enable_ctrls, 1, FRPhysicalLight_TARGET_DIST,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_TARGET_DISTANCE, IDC_FIRERENDER_PHYS_LIGHT_TARGET_DISTANCE_S
	FRPhysicalLight_TARGET_DIST, _T("DistanceToTarget"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, DistToTargetDefault,
	p_range, DistToTargetMin, DistToTargetMax,
	p_ui, ROLLOUT_GENERAL,
	TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_TARGET_DISTANCE, IDC_FIRERENDER_PHYS_LIGHT_TARGET_DISTANCE_S, SPIN_AUTOSCALE,
	p_enabled, FALSE,
	PB_END,

	// - invisible params (click points)
	FRPhysicalLight_LIGHT_POINT, _T("LightPoint"), TYPE_POINT3, 0, 0,
	p_default, Point3(0.0f, 0.0f, 0.0f),
	PB_END,

	FRPhysicalLight_SECOND_POINT, _T("SecondPoint"), TYPE_POINT3, 0, 0,
	p_default, Point3(0.0f, 0.0f, 0.0f),
	PB_END,

	FRPhysicalLight_THIRD_POINT, _T("ThirdPoint"), TYPE_POINT3, 0, 0,
	p_default, Point3(0.0f, 0.0f, 0.0f),
	PB_END,

	FRPhysicalLight_TARGET_POINT, _T("TargetPoint"), TYPE_POINT3,
	P_RESET_DEFAULT, 0, // reset default so "upright" mode always resets when created,
	// avoids unpredictable behaior when toggling the Targeted setting after creation
	p_default, Point3(0.0f, 0.0f, 0.0f),
	PB_END,

	// Intensity
	//IDC_FIRERENDER_PHYS_LIGHT_INTENSITY, IDC_FIRERENDER_PHYS_LIGHT_INTENSITY_S
	FRPhysicalLight_LIGHT_INTENSITY, _T("LightIntensity"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, LightIntensityDefault,
	p_range, LightIntensityMin, LightIntensityMax,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_INTENSITY, IDC_FIRERENDER_PHYS_LIGHT_INTENSITY_S, SPIN_AUTOSCALE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_INTENS_UNITS
	FRPhysicalLight_INTENSITY_UNITS, _T("UnitsType"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, FRPhysicalLight_WATTS,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_INT_COMBOBOX, IDC_FIRERENDER_PHYS_LIGHT_INTENS_UNITS,
	4, IDS_PHYS_LIGHT_UNITS_LUMINANCE, IDS_PHYS_LIGHT_UNITS_WATTS, IDS_PHYS_LIGHT_UNITS_RADIANCE, IDS_PHYS_LIGHT_UNITS_LUMENS,
	p_vals, FRPhysicalLight_LUMINANCE, FRPhysicalLight_WATTS, FRPhysicalLight_RADIANCE, FRPhysicalLight_LUMEN,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_LUM_EFF, IDC_FIRERENDER_PHYS_LIGHT_LUM_EFF_S
	FRPhysicalLight_LUMINOUS_EFFICACY, _T("LuminousEfficacy"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, LuminousEfficacyDefault,
	p_range, LuminousEfficacyMin, LuminousEfficacyMax,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_LUM_EFF, IDC_FIRERENDER_PHYS_LIGHT_LUM_EFF_S, SPIN_AUTOSCALE,
	//p_enabled, FALSE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_COLOUR_MODE
	FRPhysicalLight_COLOUR_MODE, _T("ColourMode"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, FRPhysicalLight_COLOUR,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_INT_COMBOBOX, IDC_FIRERENDER_PHYS_LIGHT_COLOUR_MODE,
	2, IDS_PHYS_LIGHT_MODE_COLOUR, IDS_PHYS_LIGHT_MODE_TEMPERAT,
	p_vals, FRPhysicalLight_COLOUR, FRPhysicalLight_TEMPERATURE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_COLOR
	FRPhysicalLight_INTENSITY_COLOUR, _T("ColourPicker"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, IntensityColour,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_COLORSWATCH, IDC_FIRERENDER_PHYS_LIGHT_COLOR,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_COLOR_T
	FRPhysicalLight_INTENSITY_COLOUR_MAP, _T("ColourMap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRPhysicalLight_MAP_INTENSITY_COLOUR,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_TEXMAPBUTTON, IDC_FIRERENDER_PHYS_LIGHT_COLOR_T,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_COLOUR_MAP_ENABLED
	FRPhysicalLight_INTENSITY_COLOUR_USEMAP, _T("EnableMap"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_SINGLECHEKBOX, IDC_FIRERENDER_PHYS_LIGHT_COLOUR_MAP_ENABLED,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE_S
	FRPhysicalLight_INTENSITY_TEMPERATURE, _T("Temperature"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, ColourTemperatureDefault,
	p_range, ColourTemperatureMin, ColourTemperatureMax,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE_S, SPIN_AUTOSCALE,
	//p_enabled, FALSE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE_C
	FRPhysicalLight_INTENSITY_TEMPERATURE_COLOUR, _T("TemperatureColour"), TYPE_RGBA, P_ANIMATABLE, 0,
	p_default, TemperatureColour,
	p_accessor, &physlightTColourAccessor,
	p_ui, ROLLOUT_INTENSITY,
	TYPE_COLORSWATCH, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE_C,
	//p_enabled, FALSE,
	PB_END,

	// Area Light
	//IDC_FIRERENDER_PHYS_LIGHT_AREA_VISIBLE
	FRPhysicalLight_AREALIGHT_ISVISIBLE, _T("IsVisible"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, ROLLOUT_AREALIGHT,
	TYPE_SINGLECHEKBOX, IDC_FIRERENDER_PHYS_LIGHT_AREA_VISIBLE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_BIDIRECT
	FRPhysicalLight_AREALIGHT_ISBIDIRECTIONAL, _T("IsBiDirectional"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, ROLLOUT_AREALIGHT,
	TYPE_SINGLECHEKBOX, IDC_FIRERENDER_PHYS_LIGHT_BIDIRECT,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_SHAPE
	FRPhysicalLight_AREALIGHT_LIGHTSHAPE, _T("AreaShape"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, FRPhysicalLight_DISC,
	p_ui, ROLLOUT_AREALIGHT,
	TYPE_INT_COMBOBOX, IDC_FIRERENDER_PHYS_LIGHT_SHAPE,
	5, IDS_PHYS_LIGHT_SHAPE_DISC, IDS_PHYS_LIGHT_SHAPE_CYLINDER, IDS_PHYS_LIGHT_SHAPE_SPHERE, IDS_PHYS_LIGHT_SHAPE_RECT, IDS_PHYS_LIGHT_SHAPE_MESH,
	p_vals, FRPhysicalLight_DISC, FRPhysicalLight_CYLINDER, FRPhysicalLight_SPHERE, FRPhysicalLight_RECTANGLE, FRPhysicalLight_MESH,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_MESH_T
	FRPhysicalLight_AREALIGHT_LIGHTMESH, _T("AreaMesh"), TYPE_INODE, P_NO_INIT, 0,
	p_validator, &physlightMeshValidator,
	p_accessor, &physlightMeshAccessor,
	p_ui, ROLLOUT_AREALIGHT,
	TYPE_PICKNODEBUTTON, IDC_FIRERENDER_PHYS_LIGHT_MESH_T,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_INTENSITYNORM
	FRPhysicalLight_AREALIGHT_ISINTENSITYNORMALIZATION, _T("IsIntensityNormalized"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, ROLLOUT_AREALIGHT,
	TYPE_SINGLECHEKBOX, IDC_FIRERENDER_PHYS_LIGHT_INTENSITYNORM,
	PB_END,

	// Spot Light
	//IDC_FIRERENDER_PHYS_LIGHT_SPOT_VISIBLE
	FRPhysicalLight_SPOTLIGHT_ISVISIBLE, _T("IsSpotlightVisible"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE,
	p_ui, ROLLOUT_SPOTLIGHT,
	TYPE_SINGLECHEKBOX, IDC_FIRERENDER_PHYS_LIGHT_SPOT_VISIBLE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_INNER_CONE, IDC_FIRERENDER_PHYS_LIGHT_INNER_CONE_S
	FRPhysicalLight_SPOTLIGHT_INNERCONE, _T("InnerConeAngle"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, InnerConeAngleDefault,
	p_range, InnerConeAngleMin, InnerConeAngleMax,
	p_ui, ROLLOUT_SPOTLIGHT,
	TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_INNER_CONE, IDC_FIRERENDER_PHYS_LIGHT_INNER_CONE_S, SPIN_AUTOSCALE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_OUTER_CONE, IDC_FIRERENDER_PHYS_LIGHT_OUTER_CONE_S
	FRPhysicalLight_SPOTLIGHT_OUTERCONE, _T("OuterConeFalloff"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, OuterConeFalloffDefault,
	p_range, OuterConeFalloffMin, OuterConeFalloffMax,
	p_ui, ROLLOUT_SPOTLIGHT,
	TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_OUTER_CONE, IDC_FIRERENDER_PHYS_LIGHT_OUTER_CONE_S, SPIN_AUTOSCALE,
	PB_END,

	// Light Decay
	//IDC_FIRERENDER_PHYS_LIGHT_DECAY_TYPE
	FRPhysicalLight_LIGHTDECAY_TYPE, _T("LightDecayType"), TYPE_INT, P_ANIMATABLE, 0,
	p_default, FRPhysicalLight_NONE,
	//p_ui, ROLLOUT_LIGHTDECAY,
	//TYPE_INT_COMBOBOX, IDC_FIRERENDER_PHYS_LIGHT_DECAY_TYPE,
	//3, IDS_PHYS_LIGHT_DECAY_TYPE_NONE, IDS_PHYS_LIGHT_DECAY_TYPE_INV, IDS_PHYS_LIGHT_DECAY_TYPE_LIN,
	//p_vals, FRPhysicalLight_NONE, FRPhysicalLight_INVERSESQUARE, FRPhysicalLight_LINEAR,
	p_enabled, FALSE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_FALLOFF_START, IDC_FIRERENDER_PHYS_LIGHT_FALLOFF_START_S
	FRPhysicalLight_LIGHTDECAY_FALLOFF_START, _T("FalloffStart"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, DecayFalloffStartDefault,
	p_range, DecayFalloffStartMin, DecayFalloffStartMax,
	//p_ui, ROLLOUT_LIGHTDECAY,
	//TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_FALLOFF_START, IDC_FIRERENDER_PHYS_LIGHT_FALLOFF_START_S, SPIN_AUTOSCALE,
	p_enabled, FALSE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_FALLOFF_END, IDC_FIRERENDER_PHYS_LIGHT_FALLOFF_END_S
	FRPhysicalLight_LIGHTDECAY_FALLOFF_END, _T("FalloffEnd"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, DecayFalloffEndDefault,
	p_range, DecayFalloffEndMin, DecayFalloffEndMax,
	//p_ui, ROLLOUT_LIGHTDECAY,
	//TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_FALLOFF_END, IDC_FIRERENDER_PHYS_LIGHT_FALLOFF_END_S, SPIN_AUTOSCALE,
	p_enabled, FALSE,
	PB_END,

	// Shadows
	//IDC_FIRERENDER_PHYS_LIGHT_SHADOW_ENABLED
	FRPhysicalLight_SHADOWS_ISENABLED, _T("AreShadowsEnabled"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE,
	//p_ui, ROLLOUT_SHADOWS,
	//TYPE_SINGLECHEKBOX, IDC_FIRERENDER_PHYS_LIGHT_SHADOW_ENABLED,
	p_enable_ctrls, 2, FRPhysicalLight_SHADOWS_SOFTNESS, FRPhysicalLight_SHADOWS_TRANSPARENCY,
	p_enabled, FALSE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_SHADOW_SOFTNESS, IDC_FIRERENDER_PHYS_LIGHT_SHADOW_SOFTNESS_S
	FRPhysicalLight_SHADOWS_SOFTNESS, _T("Softness"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, ShadowsSoftnessDefault,
	p_range, ShadowsSoftnessMin, ShadowsSoftnessMax,
	//p_ui, ROLLOUT_SHADOWS,
	//TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_SHADOW_SOFTNESS, IDC_FIRERENDER_PHYS_LIGHT_SHADOW_SOFTNESS_S, SPIN_AUTOSCALE,
	p_enabled, FALSE,
	PB_END,

	//IDC_FIRERENDER_PHYS_LIGHT_SHADOW_TRANS, IDC_FIRERENDER_PHYS_LIGHT_SHADOW_TRANS_S
	FRPhysicalLight_SHADOWS_TRANSPARENCY, _T("Transparency"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, ShadowsTransparencyDefault,
	p_range, ShadowsTransparencyMin, ShadowsTransparencyMax,
	//p_ui, ROLLOUT_SHADOWS,
	//TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_SHADOW_TRANS, IDC_FIRERENDER_PHYS_LIGHT_SHADOW_TRANS_S, SPIN_AUTOSCALE,
	p_enabled, FALSE,
	PB_END,

	// Volume
	//IDC_FIRERENDER_PHYS_LIGHT_VOLUME_SCALE, IDC_FIRERENDER_PHYS_LIGHT_VOLUME_SCALE_S
	FRPhysicalLight_VOLUME_SCALE, _T("Volume Scale"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, VolumeScaleDefault,
	p_range, VolumeScaleMin, VolumeScaleMax,
	//p_ui, ROLLOUT_VOLUME,
	//TYPE_SPINNER, EDITTYPE_FLOAT, IDC_FIRERENDER_PHYS_LIGHT_VOLUME_SCALE, IDC_FIRERENDER_PHYS_LIGHT_VOLUME_SCALE_S, SPIN_AUTOSCALE,
	p_enabled, FALSE,
	PB_END,

	PB_END
);

ParamBlockDesc2* GetFireRenderPhysicalLightPBDesc()
{
	return &pbDesc;
}

std::map<int, std::pair<ParamID, MCHAR*>> FireRenderPhysicalLight::TEXMAP_MAPPING = {
	{ FRPhysicalLight_MAP_INTENSITY_COLOUR,{ FRPhysicalLight_INTENSITY_COLOUR_MAP, _T("Intensity Color") } },
};

// class LoadCallback, used by Load() method
FireRenderPhysicalLight::LoadCallback::LoadCallback(FireRenderPhysicalLight *light)
	: m_light(light)
{}

void FireRenderPhysicalLight::LoadCallback::proc(ILoad *iload)
{
	FASSERT(m_light);
	IParamBlock2* pblock = (m_light==nullptr? nullptr : m_light->GetParamBlock(0));

	// Update parameters from older paramblock versions
	if( pblock!=nullptr )
	{
		IParamBlock2PostLoadInfo* postLoadInfo =
			(IParamBlock2PostLoadInfo*)pblock->GetInterface(IPARAMBLOCK2POSTLOADINFO_ID);

		// Check version number from loaded file
		if( (postLoadInfo!=NULL) && (postLoadInfo->GetVersion()<=1) )
		{
			TimeValue t(0);
			FRPhysicalLight_LightType lightType = m_light->GetLightType(t);
			FRPhysicalLight_AreaLight_LightShape lightShape = m_light->GetLightShapeMode(t);

			// Cylinder lights now use formal parameter instead of mouse points for height
			if( (lightType==FRPhysicalLight_AREA) && (lightShape==FRPhysicalLight_CYLINDER) )
			{
				float length = (m_light->GetThirdPoint(t).z-m_light->GetSecondPoint(t).z);
				m_light->SetLength(length,t); // Apply newer-style value
			}
		}
	}
	delete this; // object is expected to delete itself when proc() is finished
}

IOResult FireRenderPhysicalLight::Save( ISave *isave )
{	// nothing to do, call superclass method
	return __super::Save(isave); 
}
IOResult FireRenderPhysicalLight::Load( ILoad *iload )
{	// register our post-load callback, then call superclass method
	iload->RegisterPostLoadCallback(new LoadCallback(this));
	return __super::Load(iload);
}


RefResult FireRenderPhysicalLight::NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS)
//define NOTIFY_REF_CHANGED_PARAMETERS const Interval& interval, RefTargetHandle hTarget, PartID& partId, RefMessage msg, BOOL propagate
{
	// back-offs
	if (REFMSG_CHANGE != msg)
		return REF_SUCCEED;

	if (m_isPreview)
		return REF_SUCCEED;

	TimeValue time = GetCOREInterface()->GetTime();
	// update light based on changes
	if (hTarget == m_pblock)
	{
		ParamID p = m_pblock->LastNotifyParamID();

		switch (p)
		{
			// target added / removed
			case FRPhysicalLight_IS_TARGETED:
			{
				if (!theHold.RestoreOrRedoing())
				{
					if (IsTargeted())
					{
						// changed to is targeted => create target
						AddTarget(time, false);
					}
					else
					{
						// changed to not is targeted => remove target
						RemoveTarget(time);
					}
					GetCOREInterface()->RedrawViews(time);
				}

				break;
			}

			// intensity temperature value changed
			// => update temperature colour
			case FRPhysicalLight_INTENSITY_TEMPERATURE:
			// intensity temperature colour value changed
			// => change temperature colour value back to corresponding temperature value
			case FRPhysicalLight_INTENSITY_TEMPERATURE_COLOUR:
			{
				// enable colour window
				// - get map
				IParamMap2* pMap = m_pblock->GetMap(ROLLOUT_INTENSITY);
				FASSERT(pMap);
				HWND hDlg = pMap->GetHWnd();
				HWND hColorBox = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE_C);

				// change colour
				float kelvin = 0.0f;
				Interval valid(interval);
				m_pblock->GetValue(FRPhysicalLight_INTENSITY_TEMPERATURE, time, kelvin, valid);
				Color newColor = KelvinToColor(kelvin);
				m_pblock->SetValue(FRPhysicalLight_INTENSITY_TEMPERATURE_COLOUR, time, newColor);

				// disable colour window
				pMap->Enable(FRPhysicalLight_INTENSITY_TEMPERATURE_COLOUR, FALSE);
				EnableWindow(hColorBox, FALSE);
				pMap->Invalidate();

				break;
			}

			// distance to target
			case FRPhysicalLight_TARGET_DIST:
			{
				// Don't move the target if...
				// - User is moving the light or the target, and notify processing is underway
				// - Performing an Undo operation, as the system sets the target position separately
				if( (!m_isNotifyMoveLight) && (!m_isNotifyMoveTarget) && (!theHold.RestoreOrRedoing()) )
					SetTargetDistance(GetDist(time), time);

				break;
			}
		}
	}
	else if (hTarget == m_targNodeMonitor)
	{
		m_isNotifyMoveTarget = true;
		// update UI
		float distToTarg = GetTargetDistance(time);
		SetDist(distToTarg, time);
		m_isNotifyMoveTarget = false;
	}
	else if (hTarget == m_thisNodeMonitor)
	{
		// update ui
		if (HasTargetNode())
		{
			m_isNotifyMoveLight = true;
			float distToTarg = GetTargetDistance(time);
			SetDist(distToTarg, time);
			m_isNotifyMoveLight = false;
		}
	}

	return REF_SUCCEED;
}

void FireRenderPhysicalLight::UpdateUI()
{
	 UpdatePhysLightUI(m_pblock);
}

INT_PTR PhysLightGeneralParamsDlgProc::DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR processed = FALSE;

	static std::unordered_map<UINT, MsgProc> msgProc =
	{
		{ WM_INITDIALOG, MsgProcInitDialog },
		{ WM_CLOSE, MsgProcClose },
		{ WM_COMMAND, MsgProcCommand },
	};

	auto msgProcIt = msgProc.find(msg);

	if (msgProcIt != msgProc.end())
		processed = msgProcIt->second(t, map, hWnd, wParam, lParam);

	return processed;
}

void PhysLightGeneralParamsDlgProc::DeleteThis()
{}

INT_PTR PhysLightGeneralParamsDlgProc::MsgProcInitDialog(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	// update all rollouts when general rollout is initialised (more work than neccessary?)
	UpdatePhysLightUI(map->GetParamBlock()); 
	return 1;
}

INT_PTR PhysLightGeneralParamsDlgProc::MsgProcClose(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	return 1;
}

static const std::unordered_map<FRPhysicalLight_LightType, std::vector<FRPhysicalLight_ParamID>> uiPanelsToHide =
{
	{ 
		FRPhysicalLight_AREA, 
		{
			FRPhysicalLight_SPOTLIGHT_ISVISIBLE , 
			FRPhysicalLight_SPOTLIGHT_INNERCONE , 
			FRPhysicalLight_SPOTLIGHT_OUTERCONE 
		} 
	},
	{ 
		FRPhysicalLight_SPOT, 
		{
			FRPhysicalLight_AREALIGHT_ISVISIBLE , 
			FRPhysicalLight_AREALIGHT_ISBIDIRECTIONAL ,
			FRPhysicalLight_AREALIGHT_LIGHTSHAPE ,
			FRPhysicalLight_AREALIGHT_LIGHTMESH ,
			FRPhysicalLight_AREALIGHT_ISINTENSITYNORMALIZATION 
		} 
	},
	{ 
		FRPhysicalLight_POINT, 
		{
			FRPhysicalLight_AREALIGHT_ISVISIBLE ,
			FRPhysicalLight_AREALIGHT_ISBIDIRECTIONAL ,
			FRPhysicalLight_AREALIGHT_LIGHTSHAPE ,
			FRPhysicalLight_AREALIGHT_LIGHTMESH ,
			FRPhysicalLight_AREALIGHT_ISINTENSITYNORMALIZATION,
			FRPhysicalLight_SPOTLIGHT_ISVISIBLE , 
			FRPhysicalLight_SPOTLIGHT_INNERCONE , 
			FRPhysicalLight_SPOTLIGHT_OUTERCONE 
		}
	},
	{ 
		FRPhysicalLight_DIRECTIONAL, 
		{
			FRPhysicalLight_AREALIGHT_ISVISIBLE ,
			FRPhysicalLight_AREALIGHT_ISBIDIRECTIONAL ,
			FRPhysicalLight_AREALIGHT_LIGHTSHAPE ,
			FRPhysicalLight_AREALIGHT_LIGHTMESH ,
			FRPhysicalLight_AREALIGHT_ISINTENSITYNORMALIZATION,
			FRPhysicalLight_SPOTLIGHT_ISVISIBLE , 
			FRPhysicalLight_SPOTLIGHT_INNERCONE , 
			FRPhysicalLight_SPOTLIGHT_OUTERCONE 
		}
	}
};

#define FRPhysicalLight_AllLightPanels FRPhysicalLight_POINT 

void UpdatePhysLightUI(IParamBlock2* pBlock)
{
	// get light type
	int lightType = GetFromPb<int>(pBlock, FRPhysicalLight_LIGHT_TYPE);
	int lightShape = GetFromPb<int>(pBlock, FRPhysicalLight_AREALIGHT_LIGHTSHAPE);

	// grey out sections not needed for selected light type
	IParamMap2* pAreaMap = pBlock->GetMap(ROLLOUT_AREALIGHT);
	IParamMap2* pSpotLMap = pBlock->GetMap(ROLLOUT_SPOTLIGHT);
	if (pAreaMap && pSpotLMap) // ensure that both rollouts exist
	{
		std::unordered_map<FRPhysicalLight_ParamID, IParamMap2*> id2map =
		{
			{ FRPhysicalLight_AREALIGHT_ISVISIBLE ,				   pAreaMap },
			{ FRPhysicalLight_AREALIGHT_ISBIDIRECTIONAL ,		   pAreaMap },
			{ FRPhysicalLight_AREALIGHT_LIGHTSHAPE ,			   pAreaMap },
			{ FRPhysicalLight_AREALIGHT_LIGHTMESH ,				   pAreaMap },
			{ FRPhysicalLight_AREALIGHT_ISINTENSITYNORMALIZATION , pAreaMap },

			{ FRPhysicalLight_SPOTLIGHT_ISVISIBLE , pSpotLMap },
			{ FRPhysicalLight_SPOTLIGHT_INNERCONE , pSpotLMap },
			{ FRPhysicalLight_SPOTLIGHT_OUTERCONE , pSpotLMap },
		};

		// enable all
		auto it = uiPanelsToHide.find(FRPhysicalLight_AllLightPanels);
		for (FRPhysicalLight_ParamID id : it->second)
			id2map[id]->Enable(id, TRUE);

		// disable panels that should not be visible with selected light type
		it = uiPanelsToHide.find((FRPhysicalLight_LightType)lightType);
		for (FRPhysicalLight_ParamID id : it->second)
			id2map[id]->Enable(id, FALSE);

		//pRolloutWIndow->Show(ROLLOUT_AREALIGHT + shift); // I'm not removing commented out code because we want it back after 2016 support is discontinued
		//pRolloutWIndow->Hide(ROLLOUT_SPOTLIGHT + shift);

		IRollupWindow* pSpotlightRolloutWindow = pSpotLMap->GetIRollup();
		HWND hAreaIndex = pAreaMap->GetHWnd();
		HWND hSpotIndex = pSpotLMap->GetHWnd();
		int areaIndex = pSpotlightRolloutWindow->GetPanelIndex(hAreaIndex);
		int spotIndex = pSpotlightRolloutWindow->GetPanelIndex(hSpotIndex);

		// if spotlight then show rollout spot light and hide rollout area light.
		if (lightType == FRPhysicalLight_SPOT)
		{
			pSpotlightRolloutWindow->Show(spotIndex);
			pSpotlightRolloutWindow->Hide(areaIndex);
		}
		else  if ( lightType == FRPhysicalLight_AREA)
		{
			pSpotlightRolloutWindow->Show(areaIndex);
			pSpotlightRolloutWindow->Hide(spotIndex);
		}
		else
		{
			pSpotlightRolloutWindow->Hide(spotIndex);
			pSpotlightRolloutWindow->Hide(areaIndex);
		}

		IParamMap2* pGeneralMap = pBlock->GetMap(ROLLOUT_GENERAL);
		HWND hDlg = pAreaMap->GetHWnd(); 
		bool isMesh = (lightShape == FRPhysicalLight_MESH);
		bool hasWidth = (lightShape != FRPhysicalLight_MESH);
		bool hasLength = ((lightShape == FRPhysicalLight_RECTANGLE) || (lightShape == FRPhysicalLight_CYLINDER));
		ShowWindow(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_MESH_T), (isMesh ? SW_SHOW : SW_HIDE) );
		ShowWindow(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH), (hasWidth ? SW_SHOW : SW_HIDE) );
		ShowWindow(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH_L), (hasWidth ? SW_SHOW : SW_HIDE) );
		ShowWindow(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH_S), (hasWidth ? SW_SHOW : SW_HIDE) );
		ShowWindow(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_AREA_LENGTHS), (hasLength ? SW_SHOW : SW_HIDE) );
		ShowWindow(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_AREA_LENGTHS_L), (hasLength ? SW_SHOW : SW_HIDE) );
		ShowWindow(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_AREA_LENGTHS_S), (hasLength ? SW_SHOW : SW_HIDE) );
		TSTR diameterTitle(GetString(IDS_PHYS_LIGHT_DIAMETER));
		TSTR widthTitle(GetString(IDS_PHYS_LIGHT_WIDTH));
		if( lightShape == FRPhysicalLight_DISC || lightShape == FRPhysicalLight_SPHERE || lightShape == FRPhysicalLight_CYLINDER)
		{
			SetWindowText(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH_L), diameterTitle);
		}
		if( lightShape == FRPhysicalLight_RECTANGLE)
		{
			SetWindowText(GetDlgItem( hDlg, IDC_FIRERENDER_PHYS_LIGHT_AREA_WIDTH_L), widthTitle);
		}

		// trigger ui update in editor
		pAreaMap->Invalidate(); // ui won't update without these calls
		pSpotLMap->Invalidate();
	}
}

INT_PTR PhysLightGeneralParamsDlgProc::MsgProcCommand(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	// don't need to change ui display if message comes not from light type combo box
	int paramId = LOWORD(wParam); // IDC_FIRERENDER_PHYS_LIGHT_TYPE  1594
	if (IDC_FIRERENDER_PHYS_LIGHT_TYPE != paramId)
		return 1; // back-off

	// update all rollouts because value in general rollout changed ( is this more work than neccessary? )
	UpdatePhysLightUI(map->GetParamBlock()); 

	return 1; // processed;
}

INT_PTR PhysLightIntensityDlgProc::DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR processed = FALSE;

	static std::unordered_map<UINT, MsgProc> msgProc =
	{
		{ WM_INITDIALOG, MsgProcInitDialog },
		{ WM_CLOSE, MsgProcClose },
		{ WM_COMMAND, MsgProcCommand },
	};

	auto msgProcIt = msgProc.find(msg);

	if (msgProcIt != msgProc.end())
		processed = msgProcIt->second(t, map, hWnd, wParam, lParam);

	return processed;
}

void PhysLightIntensityDlgProc::DeleteThis()
{}

INT_PTR PhysLightIntensityDlgProc::MsgProcInitDialog(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	// setup controls

	return 1;
}

INT_PTR PhysLightIntensityDlgProc::MsgProcClose(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	return 1;
}

void UpdatePhysLightIntensityTemperatureControls(IParamMap2* map, HWND hDlg)
{
	IParamBlock2* pBlock = map->GetParamBlock();
	int intensUnitsType = GetFromPb<int>(pBlock, FRPhysicalLight_INTENSITY_UNITS);
	if (FRPhysicalLight_LUMINANCE == intensUnitsType)
	{
		HWND hEditBox = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_LUM_EFF);
		EnableWindow(hEditBox, FALSE);
		HWND hSpinner = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_LUM_EFF_S);
		EnableWindow(hSpinner, FALSE);
	}
	else
	{
		HWND hEditBox = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_LUM_EFF);
		EnableWindow(hEditBox, TRUE);
		HWND hSpinner = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_LUM_EFF_S);
		EnableWindow(hSpinner, TRUE);
	}

	int intensColourMode = GetFromPb<int>(pBlock, FRPhysicalLight_COLOUR_MODE);
	if (FRPhysicalLight_COLOUR == intensColourMode)
	{
		HWND hEditBox = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE);
		EnableWindow(hEditBox, FALSE);
		HWND hSpinner = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE_S);
		EnableWindow(hSpinner, FALSE);
		HWND hPicker = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE_C);
		EnableWindow(hPicker, FALSE);

		HWND hColourPicker = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_COLOR);
		EnableWindow(hColourPicker, TRUE);
	}
	else
	{
		HWND hEditBox = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE);
		EnableWindow(hEditBox, TRUE);
		HWND hSpinner = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_TEMPERATURE_S);
		EnableWindow(hSpinner, TRUE);

		HWND hColourPicker = GetDlgItem(hDlg, IDC_FIRERENDER_PHYS_LIGHT_COLOR);
		EnableWindow(hColourPicker, FALSE);
	}
}

INT_PTR PhysLightIntensityDlgProc::MsgProcCommand(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	// don't need to change ui display if message comes not from light type combo box
	int paramId = LOWORD(wParam);
	if ((IDC_FIRERENDER_PHYS_LIGHT_INTENS_UNITS != paramId) && (IDC_FIRERENDER_PHYS_LIGHT_COLOUR_MODE != paramId))
		return 1; // back-off

	UpdatePhysLightIntensityTemperatureControls(map, hDlg);

	return 1; // processed;
}

INT_PTR PhysLightsAreaLightsDlgProc::DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR processed = FALSE;

	static std::unordered_map<UINT, MsgProc> msgProc =
	{
		{ WM_INITDIALOG, MsgProcInitDialog },
		{ WM_CLOSE, MsgProcClose },
		{ WM_COMMAND, MsgProcCommand },
		{ WM_UPDATEUISTATE, MsgProcCommand },
		{ WM_SHOWWINDOW, MsgProcCommand },
	};

	auto msgProcIt = msgProc.find(msg);

	if (msgProcIt != msgProc.end())
		processed = msgProcIt->second(t, map, hWnd, wParam, lParam);

	return processed;
}

void PhysLightsAreaLightsDlgProc::DeleteThis()
{}

INT_PTR PhysLightsAreaLightsDlgProc::MsgProcInitDialog(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	return 1;
}

INT_PTR PhysLightsAreaLightsDlgProc::MsgProcClose(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	return 1;
}

INT_PTR PhysLightsAreaLightsDlgProc::MsgProcCommand(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR processed = FALSE;

	int paramId = LOWORD(wParam); 
	if (IDC_FIRERENDER_PHYS_LIGHT_SHAPE == paramId)
		UpdatePhysLightUI(map->GetParamBlock());

	IRollupWindow* pRolloutWIndow = map->GetIRollup();

	IParamBlock2* pBlock = map->GetParamBlock();
	int lightType = GetFromPb<int>(pBlock, FRPhysicalLight_LIGHT_TYPE);

	// shift is different when light is created and when it is edited
	int shift = 0;
	int count = pRolloutWIndow->GetNumPanels();
	const int physLightRolloutWindowsCount = ROLLOUTS_PHYS_LIGHT_COUNT + ROLLOUT_MAX_GENERAL_SHIFT;
	if (count == physLightRolloutWindowsCount)
		shift = ROLLOUT_MAX_GENERAL_SHIFT;
	
	return 1; // processed;
}

INT_PTR PhysLightsSpotLightsDlgProc::DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR processed = FALSE;

	static std::unordered_map<UINT, MsgProc> msgProc =
	{
		{ WM_INITDIALOG, MsgProcInitDialog },
		{ WM_CLOSE, MsgProcClose },
		{ WM_COMMAND, MsgProcCommand },
	};

	auto msgProcIt = msgProc.find(msg);

	if (msgProcIt != msgProc.end())
		processed = msgProcIt->second(t, map, hWnd, wParam, lParam);

	return processed;
}

void PhysLightsSpotLightsDlgProc::DeleteThis()
{}

INT_PTR PhysLightsSpotLightsDlgProc::MsgProcInitDialog(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	return 1;
}

INT_PTR PhysLightsSpotLightsDlgProc::MsgProcClose(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	return 1;
}

INT_PTR PhysLightsSpotLightsDlgProc::MsgProcCommand(TimeValue t, IParamMap2* map, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
	return 1; // processed;
}

INT_PTR PhysLightsVolumeDlgProc::DlgProc(TimeValue t, IParamMap2* map, HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR processed = FALSE;

	if (WM_INITDIALOG == msg)
	{
		IRollupWindow* pRolloutWIndow = map->GetIRollup();

		// update all rollouts when value in general rollout changed (more work than neccessary? )
		UpdatePhysLightUI(map->GetParamBlock()); 

		// shift is different when light is created and when it is edited
		int shift = 0;
		int count = pRolloutWIndow->GetNumPanels();
		const int physLightRolloutWindowsCount = ROLLOUTS_PHYS_LIGHT_COUNT + ROLLOUT_MAX_GENERAL_SHIFT;
		if (count == physLightRolloutWindowsCount)
			shift = ROLLOUT_MAX_GENERAL_SHIFT;

		HWND intensityDlg = pRolloutWIndow->GetPanelDlg(GetRolloutWindowActualIndex(ROLLOUT_INTENSITY, pRolloutWIndow) + shift);

		UpdatePhysLightIntensityTemperatureControls(map, intensityDlg);

		processed = TRUE;
	}

	return processed;
}

FIRERENDER_NAMESPACE_END
