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

#include "Synchronizer.h"
#include "plugin/ParamBlock.h"
#include "plugin/TmManager.h"
#include "plugin/CamManager.h"

FIRERENDER_NAMESPACE_BEGIN

void Synchronizer::UpdateToneMapper()
{
	BOOL res;

	auto context = mScope.GetContext();
		
	int _toneMapping_type = frw::ToneMappingTypeNone;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_OPERATOR, _toneMapping_type); FASSERT(res);
	float _tonemapping_reinhard02_prescale;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_REINHARD_PRESCALE, _tonemapping_reinhard02_prescale); FASSERT(res);
	float _tonemapping_reinhard02_postscale;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_REINHARD_POSTSCALE, _tonemapping_reinhard02_postscale); FASSERT(res);
	float _tonemapping_reinhard02_burn;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_REINHARD_BURN, _tonemapping_reinhard02_burn); FASSERT(res);
	float _tonemapping_photolinear_iso;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_PHOTOLINEAR_ISO, _tonemapping_photolinear_iso); FASSERT(res);
	float _tonemapping_photolinear_fstop;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_PHOTOLINEAR_FSTOP, _tonemapping_photolinear_fstop); FASSERT(res);
	float _tonemapping_photolinear_shutterspeed;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_PHOTOLINEAR_SHUTTERSPEED, _tonemapping_photolinear_shutterspeed); FASSERT(res);
	float _tonemapping_simplified_exposure;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_EXPOSURE, _tonemapping_simplified_exposure); FASSERT(res);
	float _tonemapping_simplified_contrast;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_CONTRAST, _tonemapping_simplified_contrast); FASSERT(res);
	int _tonemapping_simplified_whitebalance;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_WHITEBALANCE, _tonemapping_simplified_whitebalance); FASSERT(res);
	Color _tonemapping_simplified_tintcolor;
	res = TmManagerMax::TheManager.GetProperty(PARAM_TM_SIMPLIFIED_TINTCOLOR, _tonemapping_simplified_tintcolor); FASSERT(res);
	
	const int renderMode = GetFromPb<int>(mBridge->GetPBlock(), PARAM_RENDER_MODE);
	
	float _interactiveToneMapping_exposure;
	res = CamManagerMax::TheManager.GetProperty(PARAM_CAM_EXPOSURE, _interactiveToneMapping_exposure);  FASSERT(res);

	// reset the exposure to 1 if we are doing render passes or material editor rendering
	if (renderMode == RPR_RENDER_MODE_WIREFRAME || renderMode == RPR_RENDER_MODE_MATERIAL_INDEX || 
		renderMode == RPR_RENDER_MODE_POSITION || renderMode == RPR_RENDER_MODE_NORMAL ||
		renderMode == RPR_RENDER_MODE_TEXCOORD || renderMode == RPR_RENDER_MODE_AMBIENT_OCCLUSION)
	{
		_interactiveToneMapping_exposure = 1.f;
	}
	bool _interactiveToneMapping_isNormals = (renderMode == RPR_RENDER_MODE_NORMAL);

	// Enable the tone mapping only if we are in suitable render mode
	bool _interactiveToneMapping_shouldToneMap =
		(_toneMapping_type != frw::ToneMappingTypeNone) &&
		(renderMode == RPR_RENDER_MODE_GLOBAL_ILLUMINATION ||
		renderMode == RPR_RENDER_MODE_DIRECT_ILLUMINATION ||
		renderMode == RPR_RENDER_MODE_DIRECT_ILLUMINATION_NO_SHADOW);
	
	bool isNormals = _interactiveToneMapping_isNormals;
	bool shouldToneMap = _interactiveToneMapping_shouldToneMap;

	if (mWhiteBalance)
	{
		context.Detach(mWhiteBalance);
		mWhiteBalance.Reset();
	}

	if (mSimpleTonemap)
	{
		context.Detach(mSimpleTonemap);
		mSimpleTonemap.Reset();
	}

	if (mLinearTonemap)
	{
		context.Detach(mLinearTonemap);
		mLinearTonemap.Reset();
	}

	if (mGammaCorrection)
	{
		context.Detach(mGammaCorrection);
		mGammaCorrection.Reset();
	}
	
	switch (_toneMapping_type)
	{
	case frw::ToneMappingTypeNone:
		break;

	case frw::ToneMappingTypeSimplified:
	{
		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_TYPE, RPR_TONEMAPPING_OPERATOR_NONE);

		float exposure = _tonemapping_simplified_exposure;
		float contrast = _tonemapping_simplified_contrast;
		int whitebalance = _tonemapping_simplified_whitebalance;
		Color tintcolor = _tonemapping_simplified_tintcolor;

		if (!mSimpleTonemap)
		{
			mSimpleTonemap = frw::PostEffect(context, frw::PostEffectTypeSimpleTonemap);
			context.Attach(mSimpleTonemap);
		}

		mSimpleTonemap.SetParameter("exposure", exposure);
		mSimpleTonemap.SetParameter("contrast", contrast);

		if (!mWhiteBalance)
		{
			mWhiteBalance = frw::PostEffect(context, frw::PostEffectTypeWhiteBalance);
			context.Attach(mWhiteBalance);
		}

		mWhiteBalance.SetParameter("colorspace", frw::ColorSpaceTypeAdobeSRGB);
		mWhiteBalance.SetParameter("colortemp", (float)whitebalance);

		if (!mGammaCorrection)
		{
			mGammaCorrection = frw::PostEffect(context, frw::PostEffectTypeGammaCorrection);
			context.Attach(mGammaCorrection);
		}

	}
	break;
	case frw::ToneMappingTypeLinear:
		if (!mLinearTonemap)
		{
			mLinearTonemap = frw::PostEffect(context, frw::PostEffectTypeToneMap);
			context.Attach(mLinearTonemap);
		}

		if (!mGammaCorrection)
		{
			mGammaCorrection = frw::PostEffect(context, frw::PostEffectTypeGammaCorrection);
			context.Attach(mGammaCorrection);
		}

		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_TYPE, RPR_TONEMAPPING_OPERATOR_PHOTOLINEAR);
		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_SENSITIVITY, _tonemapping_photolinear_iso * 0.01f);
		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_FSTOP, _tonemapping_photolinear_fstop);
		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_EXPOSURE, 1.0f / _tonemapping_photolinear_shutterspeed);
		break;

	case frw::ToneMappingTypeNonLinear:
		if (!mLinearTonemap)
		{
			mLinearTonemap = frw::PostEffect(context, frw::PostEffectTypeToneMap);
			context.Attach(mLinearTonemap);
		}

		if (!mGammaCorrection)
		{
			mGammaCorrection = frw::PostEffect(context, frw::PostEffectTypeGammaCorrection);
			context.Attach(mGammaCorrection);
		}

		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_TYPE, RPR_TONEMAPPING_OPERATOR_REINHARD02);
		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_REINHARD02_PRE_SCALE, _tonemapping_reinhard02_prescale);
		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_REINHARD02_POST_SCALE, _tonemapping_reinhard02_postscale);
		context.SetParameter(RPR_CONTEXT_TONE_MAPPING_REINHARD02_BURN, _tonemapping_reinhard02_burn);
		break;

	default:
		break;
	}

	// Pass the exposure value to Production renderer and ActiveShade for image post-processing
	mBridge->SetToneMappingExposure(_interactiveToneMapping_exposure);
}

FIRERENDER_NAMESPACE_END
