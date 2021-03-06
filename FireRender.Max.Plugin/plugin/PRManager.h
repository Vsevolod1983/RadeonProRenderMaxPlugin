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

#pragma once

#include <max.h>
#include <guplib.h>
#include <notify.h>
#include <iparamb2.h>

#include <thread>

#include "Common.h"
#include "ParamBlock.h"
#include "plugin/ManagerBase.h"
#include "parser/RenderParameters.h"
#include "parser/SceneCallbacks.h"
#include "plugin/FireRenderer.h"
#include "ImageFilter/ImageFilter.h"


FIRERENDER_NAMESPACE_BEGIN

#define MPMANAGER_VERSION 1


class FireRenderer;
class ProductionRenderCore;

// render elements
enum FramebufferTypeId
{
	FramebufferTypeId_Color,
	FramebufferTypeId_ColorResolve,
	FramebufferTypeId_Alpha,
	FramebufferTypeId_AlphaResolve,
	FramebufferTypeId_WorldCoordinate,
	FramebufferTypeId_WorldCoordinateResolve,
	FramebufferTypeId_UV,
	FramebufferTypeId_UVResolve,
	FramebufferTypeId_MaterialIdx,
	FramebufferTypeId_MaterialIdxResolve,
	FramebufferTypeId_GeometricNormal,
	FramebufferTypeId_GeometricNormalResolve,
	FramebufferTypeId_ShadingNormal,
	FramebufferTypeId_ShadingNormalResolve,
	FramebufferTypeId_Depth,
	FramebufferTypeId_DepthResolve,
	FramebufferTypeId_ObjectId,
	FramebufferTypeId_ObjectIdResolve,
	FrameBufferTypeId_GroupId,
	FrameBufferTypeId_GroupIdResolve,
	FrameBufferTypeId_Background,
	FrameBufferTypeId_BackgroundResolve,
	FrameBufferTypeId_Emission,
	FrameBufferTypeId_EmissionResolve,
	FrameBufferTypeId_Velocity,
	FrameBufferTypeId_VelocityResolve,
	FrameBufferTypeId_DirectIllumination,
	FrameBufferTypeId_DirectIlluminationResolve,
	FrameBufferTypeId_IndirectIllumination,
	FrameBufferTypeId_IndirectIlluminationResolve,
	FrameBufferTypeId_AO,
	FrameBufferTypeId_AOResolve,
	FrameBufferTypeId_DirectDiffuse,
	FrameBufferTypeId_DirectDiffuseResolve,
	FrameBufferTypeId_DirectReflect,
	FrameBufferTypeId_DirectReflectResolve,
	FrameBufferTypeId_IndirectDiffuse,
	FrameBufferTypeId_IndirectDiffuseResolve,
	FrameBufferTypeId_IndirectReflect,
	FrameBufferTypeId_IndirectReflectResolve,
	FrameBufferTypeId_DirectRefract,
	FrameBufferTypeId_DirectRefractResolve,
	FrameBufferTypeId_Volume,
	FrameBufferTypeId_VolumeResolve,
	FramebufferTypeId_DiffuseAlbedo,
	FramebufferTypeId_DiffuseAlbedoResolve,

	// special cases
	FrameBufferTypeId_ShadowCatcher,
	FrameBufferTypeId_Composite,
	FrameBufferTypeId_CompositeResolve,
	FrameBufferTypeId_Variance,
	FrameBufferTypeId_VarianceResolve,
};


class PRManagerMax : public GUP, public ReferenceMaker
{
public:
	static PRManagerMax TheManager;

public:
	PRManagerMax();
	~PRManagerMax();

	// GUP Methods
	DWORD Start() override;
	void Stop() override;

	void DeleteThis() override;

	static ClassDesc* GetClassDesc();

	int Open(FireRenderer *renderer, HWND hwnd, RendProgressCallback* prog);

	int Render(FireRenderer *pRenderer, TimeValue t, ::Bitmap* frontBuffer, FrameRendParams &frp, HWND hwnd, RendProgressCallback* prog, ViewParams* viewPar);

	void Close(FireRenderer *pRenderer, HWND hwnd, RendProgressCallback* prog);

	void Abort(FireRenderer *pRenderer);

	void EnableExportState(bool state);

	void EnableUseExternalFiles(bool state);

	bool GetIsExportCurrentFrame();

	void SetIsExportCurrentFrame(bool state);

	void SetExportName(const std::wstring & fileName);

	void SetExportCompressionFlags(std::uint32_t flags);

	static const MCHAR* GetStampHelp();

private:
	class Data
	{
	public:
		ScopeID scopeId;

		TerminationCriteria termCriteria;
		unsigned int passLimit;
		__time64_t timeLimit;
		bool isNormals;
		bool shouldToneMap;
		float toneMappingExposure;
		bool isToneOperatorPreviewRender;
		bool isAlphaEnabled;
		bool isDenoiserEnabled;
		bool isAdaptiveEnabled;
		float adaptiveThreshold;

		class ProductionRenderCore* renderThread;
		std::thread* helperThread;
		std::atomic<bool> bRenderCancelled;
		bool bRenderThreadDone;
		std::atomic<bool> bQuitHelperThread;
		std::atomic<bool> bitmapUpdated;

		std::unique_ptr<ImageFilter> mDenoiser;

		Data() :
			scopeId(-1),
			termCriteria(Termination_None),
			passLimit(1),
			timeLimit(10),
			isNormals(false),
			shouldToneMap(false),
			toneMappingExposure(1.0f),
			isToneOperatorPreviewRender(false),
			isAlphaEnabled(false),
			isDenoiserEnabled(false),
			isAdaptiveEnabled(true),
			adaptiveThreshold(0.0f),

			renderThread(nullptr),
			helperThread(nullptr),
			bRenderCancelled(false),
			bRenderThreadDone(false),
			bQuitHelperThread(false),
			bitmapUpdated(false)
		{}
	};

	std::map<FireRenderer*, Data*> mInstances;

	struct ExportState
	{
		bool IsEnable = false;
		bool IsUseExternalFiles = false;
		bool IsExportCurrentFrame = false;
		std::uint32_t compressionFlags = 0;
		std::wstring FileName;
	} exportState;
	
	void CleanUpRender(FireRenderer *pRenderer);

	static void NotifyProc(void *param, NotifyInfo *info);

	// reference maker
	RefTargetHandle refTarget;

	int NumRefs() override
	{
		return 1;
	}

	RefTargetHandle GetReference(int i) override
	{
		return refTarget;
	}

	void SetReference(int i, RefTargetHandle rtarg) override
	{
		refTarget = rtarg;
	}
	void MakeReference(RefTargetHandle rtarg)
	{
		ReplaceReference(0, rtarg);
	}

	RefResult NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS) override;

	void SetupCamera(const ParsedView& view, const int imageWidth, const int imageHeight, rpr_camera outCamera);

	FramebufferTypeId GetFramebufferTypeIdForAOV(rpr_aov aov) const;
	FramebufferTypeId GetFramebufferTypeIdForAOVResolve(rpr_aov aov) const;

	std::tuple<bool, DenoiserType> IsDenoiserEnabled(IParamBlock2* pb) const;
	void SetupDenoiser(frw::Context context, PRManagerMax::Data* data, int imageWidth, int imageHeight, DenoiserType type, IParamBlock2* pb);

	void PostProcessAOVs(const RenderParameters& parameters, frw::Scope) const;
	std::tuple<float, float> SearchMinMax(const std::vector<float>& data, int width, int height) const;
	void PostProcessDepth(std::vector<float>& data, int width, int height) const;

	const int FbComponentsNumber = 4;

	std::string mMlModelPath;
};

FIRERENDER_NAMESPACE_END
