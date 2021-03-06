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

#include "frWrap.h"

#include "Common.h"
#include "utils/Utils.h"

#include "Resource.h"

#include "max.h"
#include "iparamm2.h"
#include "renderElements.h"
#include "iparamb2.h"
#include "plugin.h"



// pblock parameter ids
enum
{
	enableOn,
	filterOn,
	atmosphereOn,
	shadowOn,
	eleName,
	pbBitmap
};



class AOVRenderElement : public IRenderElement
{
	IParamBlock2* mPblock = nullptr;

public:
	ClassDesc2* mClassDesc;
	const wchar_t* mName;

public:
	AOVRenderElement(ClassDesc2* classDesc, const wchar_t* name) :
		mClassDesc(classDesc),
		mName(name)
	{
		assert(classDesc != nullptr);
		assert(name != nullptr);

		classDesc->MakeAutoParamBlocks(this);
		FASSERT(mPblock);
		SetElementName( GetName() );
	}

	virtual ~AOVRenderElement()
	{
	}

	RefTargetHandle Clone(RemapDir& remap) override
	{
		AOVRenderElement* newElement = new AOVRenderElement(mClassDesc, mName);
		newElement->ReplaceReference(0, remap.CloneRef(mPblock));
		BaseClone(this, newElement, remap);

		return static_cast<RefTargetHandle>(newElement);
	}

	int NumSubs() override
	{
		return 1;
	}

	Animatable* SubAnim(int i) override
	{
		return 0 == i ? mPblock : NULL;
	}

	TSTR SubAnimName(int i) override
	{
		return 0 == i ? L"pblock" : L"";
	}

	int NumRefs() override
	{
		return 1;
	};

	RefTargetHandle GetReference(int i) override
	{
		return 0 == i ? mPblock : NULL;
	}

private:
	virtual void SetReference(int i, RefTargetHandle rtarg) override
	{ 
		if ( i == 0 ) 
			mPblock = (IParamBlock2*) rtarg;
	}

public:
	int	NumParamBlocks() override
	{
		return 1;
	}

	IParamBlock2* GetParamBlock(int i) override
	{
		return mPblock;
	}
	
	IParamBlock2* GetParamBlockByID(BlockID id) override
	{
		return (mPblock->ID() == id) ? mPblock : NULL;
	}
    
	Class_ID ClassID() override
	{
		return mClassDesc->ClassID();
	}

    TSTR GetName() override
	{
		return mName;
	}

    void GetClassName(TSTR& s) override
	{
		static const wchar_t* classNameStr = L"RPR AOV";

		s = classNameStr;
	}

	IRenderElementParamDlg* CreateParamDialog(IRendParams* ip) override
	{
		return 0;
	}

	virtual void Update(TimeValue t, Interval& valid) override
	{
	}

	virtual int RenderBegin(TimeValue t, ULONG flags) override
	{
		return true;
	}
    
	BOOL AtmosphereApplied() const override
	{
		return false;
	}

	void* GetInterface(ULONG id) override
	{ 
		if( id==FireRender::FIRERENDER_RENDERELEMENT_INTERFACEID )
			return this;
		return SpecialFX::GetInterface(id);
	}

	void ReleaseInterface(ULONG, void *) override
	{
	}

	void SetEnabled(BOOL on) override
	{
		mPblock->SetValue(enableOn, 0, on);
	}

	BOOL IsEnabled(void) const override
	{
		int on;
		mPblock->GetValue(enableOn, 0, on, FOREVER);

		return on;
	}

	void SetFilterEnabled(BOOL on) override
	{
		mPblock->SetValue(filterOn, 0, on);
	}

	BOOL IsFilterEnabled(void) const override
	{
		int on;
		mPblock->GetValue(filterOn, 0, on, FOREVER);

		return on;
	}

	BOOL BlendOnMultipass(void) const override
	{
		return false;
	}

	BOOL ShadowsApplied(void) const override
	{
		return false;
	}

	void SetElementName(const wchar_t* value) override
	{
		// Strip semicolon, and any space characters before a semicolon
		int ibuf=0, ival=0, ichk=0;
		const int buflen=1024;
		wchar_t buf[buflen], prev=1;
		// Scan until end of buffer or terminator char in value string
		for( ; (ibuf<(buflen-1)) && (prev!=_T('\0')); ibuf++, ival++ )
		{
			// Scan for a series of spaces and semicolons
			for( ichk=ival; (value[ichk]==_T(' ')) || (value[ichk]==_T(':')); ichk++ )
				if( value[ichk]==_T(':') ) ival = (ichk+1); // if semicolon, skip past it

			prev=buf[ibuf]=value[ival];
		}
		buf[buflen-1] = _T('\0'); // in case we reached the end of the buffer

		mPblock->SetValue(eleName, 0, buf);
	}

	const wchar_t* ElementName(void) const override
	{
		const TCHAR* s = NULL;
		mPblock->GetValue(eleName, 0, s, FOREVER);

		return s;
	}

	void SetPBBitmap(PBBitmap *&b) const override
	{
		mPblock->SetValue(pbBitmap, 0, b);
	}

	void GetPBBitmap(PBBitmap *&b) const override
	{
		mPblock->GetValue(pbBitmap, 0, b, FOREVER);
	}
};



class AOVRenderElementClassDesc : public ClassDesc2
{
	Class_ID mClassId;

	const wchar_t* mClassName;
	const wchar_t* mName;

public:
 	AOVRenderElementClassDesc(Class_ID classId, const wchar_t* className, const wchar_t* name) :
		mClassId(classId),
		mClassName(className),
		mName(name)
	{
		assert(className);
		assert(name);
	}

    int IsPublic() override
	{
		return TRUE;
	}

    void* Create(BOOL loading) override
	{
		return new AOVRenderElement(this, mName);
	}

    const TCHAR* ClassName() override
	{
		return mName;
	}

    SClass_ID SuperClassID() override
	{
		return RENDER_ELEMENT_CLASS_ID;
	}

    Class_ID ClassID() override
	{
		return mClassId;
	}

    const TCHAR* Category() override
	{
		return _T("IRenderElement");
	}

    const TCHAR* InternalName() override
	{
		return mClassName;
	}
  
	HINSTANCE HInstance() override
	{ 
		using namespace FireRender;

		FASSERT(fireRenderHInstance != NULL);

		return fireRenderHInstance;
	}
};



class AOVElementXXX
{
public:
	std::shared_ptr<AOVRenderElementClassDesc> mDesc;
	std::shared_ptr<ParamBlockDesc2> mPblock;
	rpr_aov mAov;

public:
	AOVElementXXX() :
		mAov(RPR_AOV_MAX)
	{
	}

	~AOVElementXXX()
	{
	}

	AOVElementXXX(rpr_aov frAov, const wchar_t* className, const wchar_t* name, Class_ID classId) :
		mAov(frAov)
	{
		assert(frAov > RPR_AOV_COLOR && frAov < RPR_AOV_MAX);
		assert(className != nullptr);
		assert(name != nullptr);

		mDesc = std::make_shared<AOVRenderElementClassDesc>(classId, className, name);

		mPblock = std::make_shared<ParamBlockDesc2>(0, _T(""), 0, mDesc.get(), P_AUTO_CONSTRUCT, 0,
			enableOn, _T("enabled"), TYPE_BOOL, 0, IDS_ENABLED, p_default, TRUE, p_end,
			filterOn, _T("filterOn"), TYPE_BOOL, 0, IDS_FILTER_ON, p_default, TRUE, p_end,
			eleName, _T("elementName"), TYPE_STRING, 0, IDS_ELEMENT_NAME, p_default, _T(""), p_end,
			pbBitmap, _T("bitmap"), TYPE_BITMAP, 0, IDS_BITMAP, p_end,
			p_end
		);
	}

	AOVElementXXX(const AOVElementXXX& rhs)
	{
		mDesc = rhs.mDesc;
		mPblock = rhs.mPblock;
		mAov = rhs.mAov;
	}

	AOVElementXXX(AOVElementXXX&& rhs)
	{
		mDesc = rhs.mDesc;
		mPblock = rhs.mPblock;
		mAov = rhs.mAov;

		rhs.mDesc.reset();
		rhs.mPblock.reset();
	}

	AOVElementXXX& operator=(const AOVElementXXX& rhs)
	{
		mDesc = rhs.mDesc;
		mPblock = rhs.mPblock;
		mAov = rhs.mAov;
	}
};



static std::vector<AOVElementXXX> AOVRenderElements =
{
	{ RPR_AOV_OPACITY, L"RprOpacity", L"RPR : Opacity", Class_ID(0x209e157d, 0x652b22f3) },
	{ RPR_AOV_WORLD_COORDINATE, L"RprWorldCoordinate", L"RPR : World Coordinate", Class_ID(0x2b206111, 0x214071d8) },
	{ RPR_AOV_UV, L"RprUV", L"RPR : UV", Class_ID(0x4f084770, 0x5a6e19c1) },
	{ RPR_AOV_MATERIAL_IDX, L"RprMaterialIdx", L"RPR : Material Index", Class_ID(0x5fa228b7, 0x52b85c68) },
	{ RPR_AOV_GEOMETRIC_NORMAL, L"RprGeometricNormal", L"RPR : Geometric Normal", Class_ID(0x61d978d9, 0x5b02258) },
	{ RPR_AOV_SHADING_NORMAL, L"RprShadingNormal", L"RPR : Shading Normal", Class_ID(0x1f557d30, 0x628a6d6c) },
	{ RPR_AOV_DEPTH, L"RprDepth", L"RPR : Depth", Class_ID(0x73f37173, 0x17b71f13) },
	{ RPR_AOV_OBJECT_ID, L"RprObjectId", L"RPR : Object Id", Class_ID(0x6e556ec6, 0x4774515b) },
	{ RPR_AOV_OBJECT_GROUP_ID, L"RprObjectGroupId", L"RPR : Object Group Id", Class_ID(0x1d4e3072, 0x1da02872) },
	{ RPR_AOV_BACKGROUND, L"RprBackground", L"RPR : Background", Class_ID(0x106a41f0, 0x12bd4e2a) },
	{ RPR_AOV_EMISSION, L"RprEmission", L"RPR : Emission", Class_ID(0x58373b7f, 0x3a14077a) },
	{ RPR_AOV_VELOCITY, L"RprVelocity", L"RPR : Velocity", Class_ID(0x579077b0, 0x51732d75) },
	{ RPR_AOV_DIRECT_ILLUMINATION, L"RprDirectIllumination", L"RPR : Direct Illumination", Class_ID(0x48a9465e, 0x5f8f347f) },
	{ RPR_AOV_INDIRECT_ILLUMINATION, L"RprIndirectIllumination", L"RPR : Indirect Illumination", Class_ID(0x51bf04a7, 0x79a202c9) },
	{ RPR_AOV_AO, L"RprAO", L"RPR : Ambient Occlusion", Class_ID(0x6d69675a, 0x56672c13) },
	{ RPR_AOV_DIRECT_DIFFUSE, L"RprDirectDiffuse", L"RPR : Direct Diffuse", Class_ID(0x65ff44a8, 0x16e318bc) },
	{ RPR_AOV_DIRECT_REFLECT, L"RprDirectReflect", L"RPR : Direct Reflect", Class_ID(0x5cfc21b7, 0x44a31fba) },
	{ RPR_AOV_INDIRECT_DIFFUSE, L"RprIndirectDiffuse", L"RPR : Indirect Diffuse", Class_ID(0x53c808fb, 0x5ba824c3) },
	{ RPR_AOV_INDIRECT_REFLECT, L"RprIndirectReflect", L"RPR : Indirect Reflect", Class_ID(0x53bd322c, 0x2a184bdf) },
	{ RPR_AOV_REFRACT, L"RprRefract", L"RPR : Refract", Class_ID(0x359b767a, 0x70fb474e) },
	{ RPR_AOV_VOLUME, L"RprVolume", L"RPR : Volume", Class_ID(0x19aa705d, 0x1a5c6a47) },
	{ RPR_AOV_VARIANCE, L"RprVariance", L"RPR : Variance", Class_ID(0x59b14343, 0x121b359d) }
};



int GetAOVElementClassDescCount()
{ 
	return int_cast( AOVRenderElements.size() );
}

ClassDesc2& GetAOVElementClassDesc(int i)
{ 
	return *AOVRenderElements[i].mDesc; 
}

struct RenderElementTag
{
	Class_ID    mClassId;
	const char* mName;
	rpr_aov     mAOV;
};

// these just in case we want to support some of the others renderelements
// it's a list of all render elements I had enumerated on my system
RenderElementTag compatibleRenderElements[] =
{
//{Class_ID(1778464685, 1758472605),"3dsmax Hair"},
//{Class_ID(610800728, 1405106252),"Paint"},
//{Class_ID(450513551, 1836194615),"Ink"},
//{Class_ID(2009139533, 233458819),"Light: Area"},
//{Class_ID(1318062206, 483423356),"Light: Point"},
//{Class_ID(1735665060, 1570650691),"iray: Normal"},
//{Class_ID(2008367787, 1950689204),"iray: Alpha"},
//{Class_ID(1003705968, 1680683943),"iray: Irradiance"},
//{Class_ID(1603234833, 1368149639),"Light: Environment"},
//{Class_ID(580994625, 316828782),"Mtl: Diffuse"},
//{Class_ID(1345461277, 1072069339),"Mtl: Caustics"},
//{Class_ID(2073323103, 1345732541),"Mtl: Reflections"},
//{Class_ID(1017512132, 1170749456),"Mtl: Self Illumination"},
//{Class_ID(1675198369, 1839140307),"Mtl: Translucency"},
//{Class_ID(734068718, 3234512922),"Mtl: Transparency"},
//{Class_ID(733032594, 560299706),"Custom LPE"},
//{Class_ID(1751194458, 1234512735),"mr Labeled Element"},
//{Class_ID(584922219, 1020284513),"mr Shader Element"},
//{Class_ID(1430875960, 440614919),"mr A&D Output: Reflections"},
//{Class_ID(1430875960, 440614920),"mr A&D Raw: Reflections"},
//{Class_ID(1430875960, 440614921),"mr A&D Level: Reflections"},
//{Class_ID(1430875960, 440614928),"mr A&D Output: Transparency"},
//{Class_ID(1430875960, 440614929),"mr A&D Raw: Transparency"},
//{Class_ID(1430875960, 440614930),"mr A&D Level: Transparency"},
//{Class_ID(1430875960, 440614931),"mr A&D Output: Translucency"},
//{Class_ID(1430875960, 440614932),"mr A&D Raw: Translucency"},
//{Class_ID(1430875960, 440614933),"mr A&D Level: Translucency"},
//{Class_ID(1430875960, 440614934),"mr A&D Output: Diffuse Indirect Illumination"},
//{Class_ID(13792239, 1961785759),"mr Ambient Occlusion"},
//{Class_ID(1430875960, 440614935),"mr A&D Raw: Diffuse Indirect Illumination"},
//{Class_ID(1430875960, 440614936),"mr A&D Xtra: Diffuse Indirect Illumination with AO"},
//{Class_ID(1430875960, 440614937),"mr A&D Raw: Ambient Occlusion"},
//{Class_ID(1430875960, 440614944),"mr A&D Output: Self Illumination"},
//{Class_ID(1430875960, 440614945),"mr A&D Output: Opacity Background"},
//{Class_ID(1430875960, 440614946),"mr A&D Raw: Opacity Background"},
//{Class_ID(1430875960, 440614947),"mr A&D Level: Opacity"},
//{Class_ID(1430875960, 440614912),"mr A&D Output: Beauty"},
//{Class_ID(1430875960, 440614913),"mr A&D Output: Diffuse Direct Illumination"},
//{Class_ID(1430875960, 440614914),"mr A&D Raw: Diffuse Direct Illumination"},
//{Class_ID(1430875960, 440614915),"mr A&D Level: Diffuse"},
//{Class_ID(1430875960, 440614916),"mr A&D Output: Specular"},
//{Class_ID(1430875960, 440614917),"mr A&D Raw: Specular"},
//{Class_ID(1430875960, 440614918),"mr A&D Level: Specular"},
//{Class_ID(2, 0),"Specular"},
//{Class_ID(13, 0),"Alpha", RPR_AOV_OPACITY},
//{Class_ID(15, 0),"Background"},
//{Class_ID(17, 0),"Lighting"},
//{Class_ID(18, 0),"Matte"},
//{Class_ID(19, 0),"Velocity"},
{Class_ID(20, 0),"Material ID", RPR_AOV_MATERIAL_IDX},
{Class_ID(21, 0),"Object ID", RPR_AOV_OBJECT_ID},
//{Class_ID(22, 0),"Luminance HDR Data"},
//{Class_ID(23, 0),"Illuminance HDR Data"},
//{Class_ID(4, 0),"Diffuse"},
//{Class_ID(5, 0),"Self-Illumination"},
//{Class_ID(6, 0),"Reflection"},
//{Class_ID(7, 0),"Refraction"},
//{Class_ID(8, 0),"Shadow"},
//{Class_ID(9, 0),"Atmosphere"},
//{Class_ID(11, 0),"Blend"},
{Class_ID(12, 0),"Z Depth", RPR_AOV_DEPTH},
//{Class_ID(1707893372, 1745002193),"CESSENTIAL_Direct"},
//{Class_ID(1707893628, 1745510097),"CESSENTIAL_Indirect"},
//{Class_ID(1707894140, 1760489169),"CESSENTIAL_Reflect"},
//{Class_ID(1707894652, 1745027025),"CESSENTIAL_Refract"},
//{Class_ID(1707893372, 1755471569),"CESSENTIAL_Translucency"},
//{Class_ID(1700635260, 1745706705),"CESSENTIAL_Emission"},
//{Class_ID(1707893372, 1744985809),"CMasking_WireColor"},
//{Class_ID(106749660, 2270322401),"CMasking_ID"},
//{Class_ID(1705861756, 1753560145),"CMasking_Mask"},
//{Class_ID(2238081148, 305215425),"CShading_Alpha"},
//{Class_ID(106444598, 1827466977),"CShading_Components"},
//{Class_ID(1707893372, 1744985761),"CShading_Shadows"},
//{Class_ID(123393602, 947230413),"CTexmap"},
//{Class_ID(1707893372, 1744985768),"CGeometry_NormalsShading"},
//{Class_ID(1707893373, 1744985769),"CGeometry_NormalsGeometry"},
//{Class_ID(1707893372, 1744985777),"CGeometry_NormalsDotProduct"},
//{Class_ID(2235766908, 305485761),"CGeometry_WorldPosition"},
//{Class_ID(1707893372, 2014469793),"CGeometry_ZDepth"},
//{Class_ID(1707893372, 1758617169),"CGeometry_UvwCoords"},
//{Class_ID(1707893372, 1744985825),"CShading_SourceColor"},
//{Class_ID(2235768284, 449693633),"CShading_Albedo"},
//{Class_ID(1646691420, 2900975329),"CShading_RawComponent"},
//{Class_ID(1707893372, 107026049),"CESSENTIAL_Volumetrics"},
//{Class_ID(1058218705, 34734223),"VRayNormals"},
//{Class_ID(909205137, 1357209021),"VRayDiffuseFilter"},
//{Class_ID(1487086871, 1892707416),"VRayAlpha"},
//{Class_ID(1690831072, 1616736838),"VRayLighting"},
//{Class_ID(300447288, 43922261),"VRayAtmosphere"},
//{Class_ID(758140482, 433600676),"VRayBackground"},
//{Class_ID(1431780069, 1261767434),"VRayReflection"},
//{Class_ID(94963057, 1405487279),"VRayRefraction"},
//{Class_ID(393767572, 567836278),"VRaySelfIllumination"},
//{Class_ID(2120841827, 921189039),"VRayShadows"},
//{Class_ID(679610457, 1225747885),"VRaySpecular"},
//{Class_ID(425990546, 1072105274),"VRayZDepth"},
//{Class_ID(1940458856, 1646548534),"VRayRawLighting"},
//{Class_ID(122257132, 1374313835),"VRayGlobalIllumination"},
//{Class_ID(1211962951, 25895046),"VRayRawGlobalIllumination"},
//{Class_ID(1024680222, 217711203),"VRayUnclampedColor"},
//{Class_ID(736244021, 2030769033),"VRayRawShadow"},
//{Class_ID(1028009481, 2134776785),"VRayCaustics"},
//{Class_ID(435553506, 9773135),"VRayRenderID"},
//{Class_ID(800855091, 660363747),"VRayMtlID"},
//{Class_ID(2123966780, 177214407),"VRayVelocity"},
//{Class_ID(605381254, 2029204317),"VRayObjectID"},
//{Class_ID(1680487547, 564862438),"VRayReflectionFilter"},
//{Class_ID(677000469, 695293605),"VRayRawReflection"},
//{Class_ID(608446197, 364409819),"VRayRefractionFilter"},
//{Class_ID(1996100592, 128989817),"VRayRawRefraction"},
//{Class_ID(391864451, 1412566053),"VRayWireColor"},
//{Class_ID(772107011, 830866611),"VRayMtlSelect"},
//{Class_ID(1241667874, 99384385),"VRayMatteShadow"},
//{Class_ID(981756222, 1301440191),"VRayTotalLighting"},
//{Class_ID(1509425606, 529603901),"VRayRawTotalLighting"},
//{Class_ID(472131563, 518873682),"VRayBumpNormals"},
//{Class_ID(1834776139, 139401469),"VRayExtraTex"},
//{Class_ID(1040721562, 215291331),"VRaySampleRate"},
//{Class_ID(1067988051, 462435019),"VRayIlluminance"},
//{Class_ID(682799745, 2552020145),"VRayDRBucket"},
//{Class_ID(1696212553, 842215169),"VRayLightSelect"},
//{Class_ID(979451415, 648287708),"VRayMtlReflectGlossiness"},
//{Class_ID(840916418, 598046913),"VRayMtlReflectHilightGlossiness"},
//{Class_ID(848825044, 1070231005),"VRayMtlRefractGlossiness"},
//{Class_ID(1151091624, 212489509),"VRayMtlReflectIOR"},
//{Class_ID(1584217762, 1922109065),"VRayNoiseLevel"},
//{Class_ID(989152159, 534982078),"VRayDenoiser"},
//{Class_ID(1652447975, 1824545255),"VRaySSS2"},
//{Class_ID(91629368, 598765248),"VRaySamplerInfo"},
//{Class_ID(2520421430, 819296842),"MultiMatteElement"},
//{Class_ID(1733179615, 2038135853),"VRayObjectSelect"},
//{Class_ID(1683101311, 310053866),"VRayOptionRE"},
{Class_ID(1325942640, 1517164993),"frAOV_GetClassName", RPR_AOV_UV},
};

void printRenderElement(IRenderElement* renderElement)
{
	assert(renderElement != nullptr);

	MSTR className;
	renderElement->GetClassName(className);

	Class_ID classId = renderElement->ClassID();

	FireRender::debugPrint(
		std::wstring(L"isAOVRenderElement: {")
		+ L"Class_ID("
		+ std::to_wstring(classId.PartA())
		+ L", "
		+ std::to_wstring(classId.PartB()) + L")"
		+ L","
		+ L"\"" + std::wstring(className) + L"\""
		+ L"},");
}

rpr_aov getAOVRenderElementFRId(IRenderElement* renderElement)
{
	assert(renderElement != nullptr);

	printRenderElement(renderElement);

	for (const AOVElementXXX& aov : AOVRenderElements)
	{
		if (renderElement->ClassID() == aov.mDesc->ClassID())
		{
			return aov.mAov;
		}
	}

	for (const RenderElementTag& compatibleAov : compatibleRenderElements)
	{
		if (renderElement->ClassID() == compatibleAov.mClassId)
		{
			return compatibleAov.mAOV;
		}
	}

	return RPR_AOV_MAX;
}

bool isAOVRenderElement(IRenderElement* renderElement)
{
	printRenderElement(renderElement);

	for (const AOVElementXXX& aov : AOVRenderElements)
	{
		if (renderElement->ClassID() == aov.mDesc->ClassID())
		{
			return true;
		}
	}

	for (const RenderElementTag& compatibleAov : compatibleRenderElements)
	{
		if (renderElement->ClassID() == compatibleAov.mClassId)
		{
			return true;
		}
	}

	return false;
}
