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

#include "FireRenderWardMtl.h"
#include "FireRenderNormalMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(WardMtl)

FRMTLCLASSDESCNAME(WardMtl) FRMTLCLASSNAME(WardMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("WardMtlPbdesc"), 0, &FRMTLCLASSNAME(WardMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_WARDMTL, IDS_FR_MTL_WARD, 0, 0, NULL,

	FRWardMtl_COLOR, _T("Color"), TYPE_RGBA, P_ANIMATABLE, 0,
    p_default, Color(0.5f, 0.5f, 0.5f), p_ui, TYPE_COLORSWATCH, IDC_WARD_COLOR, PB_END,

	FRWardMtl_ROTATION, _T("Rotation"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.f, p_range, -999999.f, 999999.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_WARD_ROTATION, IDC_WARD_ROTATION_S, SPIN_AUTOSCALE, PB_END,

	FRWardMtl_ROTATION_TEXMAP, _T("rotationTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRWardMtl_TEXMAP_ROTATION, p_ui, TYPE_TEXMAPBUTTON, IDC_WARD_ROTATION_TEXMAP, PB_END,

	FRWardMtl_ROUGHNESSX, _T("Rougness X"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.5f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_WARD_ROUGHNESSX, IDC_WARD_ROUGHNESSX_S, SPIN_AUTOSCALE, PB_END,

	FRWardMtl_ROUGHNESSY, _T("Rougness Y"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 0.5f, p_range, 0.f, 1.f, p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_WARD_ROUGHNESSY, IDC_WARD_ROUGHNESSY_S, SPIN_AUTOSCALE, PB_END,

	FRWardMtl_COLOR_TEXMAP, _T("colorTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRWardMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_WARD_COLOR_TEXMAP, PB_END,

	FRWardMtl_ROUGHNESSX_TEXMAP, _T("rougnessXTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRWardMtl_TEXMAP_ROUGHNESSX, p_ui, TYPE_TEXMAPBUTTON, IDC_WARD_ROUGHNESSX_TEXMAP, PB_END,

	FRWardMtl_ROUGHNESSY_TEXMAP, _T("rougnessYTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRWardMtl_TEXMAP_ROUGHNESSY, p_ui, TYPE_TEXMAPBUTTON, IDC_WARD_ROUGHNESSY_TEXMAP, PB_END,

	FRWardMtl_NORMALMAP, _T("normalTexmap"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRWardMtl_TEXMAP_NORMAL, p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_TEXMAP, PB_END,

    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(WardMtl)::TEXMAP_MAPPING = {
	{ FRWardMtl_TEXMAP_COLOR, { FRWardMtl_COLOR_TEXMAP, _T("Color Map") } },
	{ FRWardMtl_TEXMAP_ROUGHNESSX, { FRWardMtl_ROUGHNESSX_TEXMAP, _T("Roughness X Map") } },
	{ FRWardMtl_TEXMAP_ROUGHNESSY, { FRWardMtl_ROUGHNESSY_TEXMAP, _T("Roughness Y Map") } },
	{ FRWardMtl_TEXMAP_NORMAL, { FRWardMtl_NORMALMAP, _T("Normal Map") } },
	{ FRWardMtl_TEXMAP_ROTATION, { FRWardMtl_ROTATION_TEXMAP, _T("Rotation Map") } }
};

FRMTLCLASSNAME(WardMtl)::~FRMTLCLASSNAME(WardMtl)()
{
}


frw::Shader FRMTLCLASSNAME(WardMtl)::getShader(const TimeValue t, MaterialParser& mtlParser, INode* node)
{
	auto ms = mtlParser.materialSystem;

	frw::Shader material(ms, frw::ShaderTypeWard);
		
	Texmap* normalTexmap = GetFromPb<Texmap*>(pblock, FRWardMtl_NORMALMAP);
	const Color diffuseColor = GetFromPb<Color>(pblock, FRWardMtl_COLOR);
	Texmap* diffuseTexmap = GetFromPb<Texmap*>(pblock, FRWardMtl_COLOR_TEXMAP);
	const float roughnessx = GetFromPb<float>(pblock, FRWardMtl_ROUGHNESSX);
	const float roughnessy = GetFromPb<float>(pblock, FRWardMtl_ROUGHNESSY);
	Texmap* roughnessXTexmap = GetFromPb<Texmap*>(pblock, FRWardMtl_ROUGHNESSX_TEXMAP);
	Texmap* roughnessYTexmap = GetFromPb<Texmap*>(pblock, FRWardMtl_ROUGHNESSY_TEXMAP);
	const float rotation = GetFromPb<float>(pblock, FRWardMtl_ROTATION);
	Texmap* rotationTexmap = GetFromPb<Texmap*>(pblock, FRWardMtl_ROTATION_TEXMAP);
	
	frw::Value color(diffuseColor.r, diffuseColor.g, diffuseColor.b);
	if (diffuseTexmap)
		color = mtlParser.createMap(diffuseTexmap, 0);
	material.SetValue(RPR_MATERIAL_INPUT_COLOR, color);

	frw::Value roughnessxv(roughnessx, roughnessx, roughnessx);
	if (roughnessXTexmap)
		roughnessxv = mtlParser.createMap(roughnessXTexmap, MAP_FLAG_NOGAMMA);

	frw::Value roughnessyv(roughnessy, roughnessy, roughnessy);
	if (roughnessYTexmap)
		roughnessyv = mtlParser.createMap(roughnessYTexmap, MAP_FLAG_NOGAMMA);

	frw::Value rotationv(rotation, rotation, rotation);
	if (rotationTexmap)
		rotationv = mtlParser.createMap(rotationTexmap, MAP_FLAG_NOGAMMA);

	material.SetValue(RPR_MATERIAL_INPUT_ROTATION, rotationv);
	material.SetValue(RPR_MATERIAL_INPUT_ROUGHNESS_X, roughnessxv);
	material.SetValue(RPR_MATERIAL_INPUT_ROUGHNESS_Y, roughnessyv);

	if (normalTexmap)
		material.SetValue(RPR_MATERIAL_INPUT_NORMAL, FRMTLCLASSNAME(NormalMtl)::translateGenericBump(t, normalTexmap, 1.f, mtlParser));
	
    return material;
}

FIRERENDER_NAMESPACE_END;
