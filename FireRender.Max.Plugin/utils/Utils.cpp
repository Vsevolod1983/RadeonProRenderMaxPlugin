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


/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* A collection of utility functions
*********************************************************************************************************************************/

#include "utils/Utils.h"
#include "CoronaDeclarations.h"
#include "plugin/ScopeManager.h"

#include <iparamb2.h>
#include <shlobj.h>
#include <bitmap.h>
#include <toneop.h>

#include <iomanip>
#include <vector>
#include <codecvt>
#include <experimental/filesystem>

#pragma comment(lib,"Version.lib")

using namespace std;

FIRERENDER_NAMESPACE_BEGIN

/// Static initialization will be done in the main thread, so this is the simplest way to get the main thread ID
static const DWORD mainThreadId = GetCurrentThreadId();

bool IsMainThread() {
    return mainThreadId == GetCurrentThreadId();
}

std::wstring ToUnicode(const std::string& input) {
    const size_t len = MultiByteToWideChar(CP_ACP, 0, &input[0], int(input.size()), 0, 0);
    std::wstring output;
    output.resize(len);
    MultiByteToWideChar(CP_ACP, 0, input.data(), int_cast(input.size()), const_cast<wchar_t*>(output.data()), int_cast(len));
    return output;
}

std::string ToAscii(const std::wstring& input) {
    const size_t len = WideCharToMultiByte(CP_ACP, 0, input.c_str(), int(input.size()), 0, 0, 0, 0);
    std::string output;
    output.resize(len);
    WideCharToMultiByte(CP_ACP, 0, input.c_str(), int(input.size()), &output[0], int(len), 0, 0);
    return output;
}

void PrintPblock(IParamBlock2* pb, std::ostream& output) {
    const int num = pb->NumParams();
    output << "Number of parameters: " << num << std::endl;
    output << "index | ID | type | flags | value" << std::endl << std::endl;
    for (int i = 0; i < num; ++i) {
        const ParamID id = pb->IndextoID(i);
        ParamDef& def = pb->GetParamDef(id);
        output << std::dec << std::setw(4) << i << " ";
        output << std::setw(6) << id << " ";
        output << std::setw(6) << def.type << " ";
        const std::string name = ToAscii(def.int_name);

        output << std::setw(10) << std::hex << def.flags << " " << name << " ";
        switch (int(def.type)) {
        case TYPE_BOOL:
        case TYPE_INT:
            output << std::dec << pb->GetInt(id);
            break;
        case TYPE_FLOAT:
        case TYPE_WORLD:
            output << pb->GetFloat(id);
            break;
        case TYPE_RGBA:
            output << pb->GetColor(id);
            break;
        case TYPE_TEXMAP:
            output << pb->GetTexmap(id);
            break;
        case TYPE_INODE:
            output << pb->GetINode(id);
            break;
		case TYPE_STRING:
			output << pb->GetStr(id);
			break;
        default:
            STOP; // more types should be implemented as they are encountered in a IParamBlock2
        }
        output << std::endl;
    }
}

void debugPrint(const std::wstring& msg) {
    time_t t = time(NULL);
    tm now;
    localtime_s(&now, &t);
    std::wstringstream timestamp;
    timestamp << now.tm_hour << _T(":") << now.tm_min << _T(":") << now.tm_sec;

    const std::wstring wholeMsg = _T("#RPR (") + timestamp.str() + _T(")  ") + msg + _T("\n");
    OutputDebugString(wholeMsg.c_str());
}


std::wstring GetDataStoreFolder()
{
	// We use the default plugin configuration dir, and add our own sub-directory "RadeonProRender"
	return TheManager->GetDir(APP_PLUGCFG_DIR) + std::wstring(DATA_FOLDER_NAME);
}

std::wstring GetModuleFolder()
{
	std::wstring pluginPath;

	HMODULE hModule = NULL;

	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPTSTR) &GetModuleFolder, &hModule))
	{
		DWORD pathSize = 1024;
		const DWORD reasonablePathSize = 4096;

		while (pathSize <= reasonablePathSize)
		{
			std::vector<wchar_t> buffer(pathSize);

			GetModuleFileName(hModule, &buffer[0], pathSize);
			DWORD rc = GetLastError();

			if (ERROR_INSUFFICIENT_BUFFER == rc)
			{
				pathSize *= 2;
			}
			else if (ERROR_SUCCESS == rc)
			{
				pluginPath.assign(buffer.data());
				break;
			}
		}
	}

	// in case we deal with a link, a real name of the module must be obtained
	DWORD fa = GetFileAttributes(pluginPath.c_str());
	bool isLink = fa & FILE_ATTRIBUTE_REPARSE_POINT;

	if (isLink)
	{
		HANDLE file = (HANDLE)CreateFile(pluginPath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		
		if (file != INVALID_HANDLE_VALUE)
		{
			// return value includes the size of the terminating null character
			int neededSize = GetFinalPathNameByHandle(file, 0, 0, VOLUME_NAME_NT);
			std::vector<wchar_t> buffer(neededSize);
			// return value does not include the size of the terminating null character
			int actualSize = GetFinalPathNameByHandle(file, buffer.data(), neededSize, 0);

			if (actualSize > 0)
			{
				pluginPath.assign(buffer.data());
			}

			CloseHandle(file);
		}
	}

	return pluginPath;
}

float GetDeterminant(const Matrix3& input) {
    return input[0][0] * input[1][1] * input[2][2]
        + input[1][0] * input[2][1] * input[0][2]
        + input[2][0] * input[0][1] * input[1][2]
        - input[0][0] * input[2][1] * input[1][2]
        - input[1][0] * input[0][1] * input[2][2]
        - input[2][0] * input[1][1] * input[0][2];
}

void CompositeFrameBuffersToBitmap(std::vector<float>& fbData, std::vector<float>& fbAlphaData, Bitmap* output, const float exposure, const bool isNormals, bool toneOperatorPreviewExecuting)
{
	if (toneOperatorPreviewExecuting) {
		CopyDataToPreviewBitmap(fbData, output, isNormals);
	}
	else {
		CopyDataToBitmap(fbData, fbAlphaData, output, exposure, isNormals);
	}
}

void CopyDataToPreviewBitmap(const std::vector<float>& fbData, Bitmap* output, const bool isNormals) {
	if (!output)
		return;

	int height = output->Height();
	int width = output->Width();

	ULONG chan = output->ChannelsPresent();

	chan = output->ChannelsPresent();

	if (chan & BMM_CHAN_COVERAGE) {
		DebugPrint(_T("G-Buffer requests Pixel Coverage\n"));

		ulong type;
		UBYTE *pixels = (UBYTE*)output->GetChannel(BMM_CHAN_COVERAGE, type);
		memset(pixels, 255, height * width);
	}


	float r, g, b;

	if (chan & BMM_CHAN_REALPIX) {
		ulong type;
		RealPixel *pixels = (RealPixel*)output->GetChannel(BMM_CHAN_REALPIX, type);

		#pragma omp parallel for
		for (int y = 0; y < height; ++y)
		{

			RealPixel *row = pixels + y * width;
			const int basey = (y << 2) * width;
			for (int x = 0; x < width; ++x)
			{
				const int base = (x << 2) + basey;

				// RPR requires us to normalize the image using the fourth component. 

				r = fbData[base] / fbData[base + 3];
				g = fbData[base + 1] / fbData[base + 3];
				b = fbData[base + 2] / fbData[base + 3];

				if (isNormals)
				{
					// We were requested to map the -1..1 RPR normals to 0..1 3ds Max normals
					r *= 0.5f;
					g *= 0.5f;
					b *= 0.5f;
					r += 0.5f;
					g += 0.5f;
					b += 0.5f;
				}

				*row++ = MakeRealPixel(r,g,b);
			}
		}
	}
}


void CopyDataToBitmap(std::vector<float>& fbData, const std::vector<float>& alphaData, Bitmap* output, const float exposure, const bool isNormals)
{
	if (fbData.size() == 0)
		return;

	FASSERT(output);
	FASSERT( IsReal(exposure) );

	if (!output)
		return;

	int width = output->Width();
	int height = output->Height();

	// we have 4 components of float type for each pixel from Core
	bool sizeValid = fbData.size() == 4 * width * height;

	FASSERT(sizeValid);

	if (!sizeValid)
		return;

	// convert normals from RPR representation to Max's
	if (isNormals)
	{
		std::for_each(fbData.begin(), fbData.end(), [](float& element)
		{
			element = element * 0.5f + 0.5f;
		});
	}

	// add alpha
	bool hasAlpha = !alphaData.empty();

	if (hasAlpha)
	{
		output->SetFlag(MAP_HAS_ALPHA);

		// fbData values are r g b alpha quadruplets; alpha is 1 by default
		// If we have input alpha values, we should replace alpha elementa in fbData
		// array (each 4-th element) with valus from alphaData array.
		FASSERT( fbData.size() == alphaData.size() );

		for (size_t idx = 3; idx < fbData.size(); idx += 4)
			fbData[idx] = alphaData[idx - 3];
	}

	// write data to output
	// max sdk can recieve picture data only by lines, thus loop is needed
	for (int y = 0; y < height; ++y)
	{
		// the reason of using reinterpret_cast here is to avoide unnecessary copying
		// BMM_Color_fl is a structure with 4 float values (r,g,b,a)
		output->PutPixels(0, y, width, reinterpret_cast<BMM_Color_fl*>(fbData.data() + y*(width * 4)));
	}
}

// don't do unit conversion here... all 3dsmax classes should retain system units
ViewParams ViewExp2viewParams(ViewExp& viewExp, INode*& outCameraNode) {
    ViewParams result;
    viewExp.GetAffineTM(result.affineTM);
	result.affineTM.SetTrans(result.affineTM.GetTrans());
    result.prevAffineTM = result.affineTM;
    result.projType = viewExp.IsPerspView() ? PROJ_PERSPECTIVE : PROJ_PARALLEL;
    result.farRange = result.nearRange = result.hither = result.yon = NAN;
	result.distance = viewExp.GetFocalDist();
	result.zoom = result.fov = viewExp.GetFOV();
    outCameraNode = viewExp.GetViewCamera();
    return result;
}

ViewExp& GetRenderViewExp() {
    if (GetCOREInterface11()->GetRendUseActiveView()) {
        return GetCOREInterface()->GetActiveViewExp();
    } else { // the render viewport is locked
        return GetCOREInterface14()->GetViewExpByID(GetCOREInterface14()->GetRendViewID());
    }
}


bool IsBumpMap(Texmap* input) {
    if (!input) {
        return false;
    }
    const Class_ID cid = input->ClassID();
    // We recognize 2 types of normal maps - 3ds Max built-in, and Corona one.
    if (cid == GNORMAL_CLASS_ID || cid == Corona::NORMAL_TEX_CID) {
        return false;
    } else {
        return true;
    }
}

void AssertImpl(const MCHAR* expression, const MCHAR* file, const int line) {
    // If a debugger is present, we will place a breakpoint, otherwise we will pop up a warning MessageBox.
    if (IsDebuggerPresent()) {
        __debugbreak();
    } else {
        std::wstringstream stream;
        stream << _T("File: ") << file << _T("(") << line << _T("): ") << expression;
        MessageBox(GetCOREInterface()->GetMAXHWnd(), stream.str().c_str(), _T("AMD Radeon ProRender debug assert"), MB_OK);
    }
}

wstring GetFileNameFromPath(wstring filePath)
{
	const wchar_t kPathSeparator =
#ifdef _WIN32
		L'\\';
#else
		L'/';
#endif
	std::size_t pos = filePath.rfind(kPathSeparator);

	if (pos == wstring::npos)
	{
		return filePath;
	}

	return filePath.substr(pos + 1, filePath.length() - pos - 1);
}

// helper function
wstring LogMaxError(rpr_int code, const wstring& fileName, const int line)
{
	std::wstring errorMsg = GetRPRErrorString(code);

	std::wstringstream messageStream;
	messageStream << fileName.c_str() << _T(":") << line << _M("\nErrorCode:\n") << code << _M(", Message: \n") << errorMsg.c_str();
	LogErrorStringToMaxLog(messageStream.str());

	return messageStream.str();
}

void LogErrorStringToMaxLog(const wstring& str)
{
	GetCOREInterface()->Log()->LogEntry(SYSLOG_ERROR, NO_DIALOG, _M("Radeon ProRender - Error"), str.c_str());
}

void RPRResultCheckImpl(rpr_int code, const MCHAR* filePath, const int line, rpr_context context, const char* functionName)
{
	if (code != RPR_SUCCESS)
	{
		wstring fileName = GetFileNameFromPath(filePath);

#if defined(FIREMAX_DEBUG)
		AssertImpl(_T(""), fileName.c_str(), line);
#endif
		wstring resMessage = LogMaxError(code, fileName, line);

		RPRLogContextStatus(code, context, functionName);

#if !defined(FIREMAX_DEBUG)
		MessageBox(GetCOREInterface()->GetMAXHWnd(), resMessage.c_str(), _T("Radeon ProRender Error"), MB_OK);
#endif
	}
}

void RPRLogContextStatus(rpr_int status, rpr_context context, const char* functionName)
{
	if( (context==nullptr) || (functionName==nullptr) )
		return;

	size_t length = 0;
	rpr_int res = rprContextGetInfo(context, RPR_CONTEXT_LAST_ERROR_MESSAGE, sizeof(size_t), nullptr, &length);
	//FCHECK_CONTEXT(res, context, "rprContextGetInfo"); // This is already inside FCHECK_CONTEXT(), don't recurse
	if( res==RPR_SUCCESS )
	{
		std::vector<rpr_char> info(length);
		res = rprContextGetInfo(context, RPR_CONTEXT_LAST_ERROR_MESSAGE, info.size(), &info[0], nullptr);
		//FCHECK_CONTEXT(res, context, "rprContextGetInfo"); // This is already inside FCHECK_CONTEXT(), don't recurse
		if( res==RPR_SUCCESS )
		{
			std::string errorMessage(info.begin(), info.end());

			std::wstring outputMessage = s2ws(functionName) + L" : " + s2ws(errorMessage);

			GetCOREInterface()->Log()->LogEntry(SYSLOG_ERROR, DISPLAY_DIALOG, L"Radeon ProRender Context Status", outputMessage.c_str());
		}
	}
}

std::wstring GetRPRErrorString(rpr_int code)
{
	std::wstring errorMsg;

	switch (code)
	{
	case RPR_ERROR_OUT_OF_VIDEO_MEMORY: errorMsg = L"Out of video memory error.\nThis scene and its textures requires more memory than is available.\nTo render you will need to upgrade your GPU"; break;
	case RPR_ERROR_OUT_OF_SYSTEM_MEMORY: errorMsg = L"Out of system memory error.\nThis scene and its textures requires more memory than is available.\nTo render you will need to upgrade your hardware"; break;
	case RPR_ERROR_MATERIAL_STACK_OVERFLOW: errorMsg = L"Material Stack Overflow"; break;
	case RPR_ERROR_COMPUTE_API_NOT_SUPPORTED: errorMsg = L"Compute API Not Supported"; break;
	case RPR_ERROR_INVALID_LIGHTPATH_EXPR: errorMsg = L"Invalid LightPath Expression"; break;
	case RPR_ERROR_INVALID_IMAGE: errorMsg = L"Invalid Image"; break;
	case RPR_ERROR_INVALID_AA_METHOD: errorMsg = L"Invalid AA Method"; break;
	case RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT: errorMsg = L"Unsupported Image Format"; break;
	case RPR_ERROR_INVALID_GL_TEXTURE: errorMsg = L"Invalid GL Texture"; break;
	case RPR_ERROR_INVALID_CL_IMAGE: errorMsg = L"Invalid CL Image"; break;
	case RPR_ERROR_INVALID_OBJECT: errorMsg = L"Invalid Object"; break;
	case RPR_ERROR_INVALID_PARAMETER: errorMsg = L"Invalid Parameter"; break;
	case RPR_ERROR_INVALID_TAG: errorMsg = L"Invalid Tag"; break;
	case RPR_ERROR_INVALID_LIGHT: errorMsg = L"Invalid Light"; break;
	case RPR_ERROR_INVALID_CONTEXT: errorMsg = L"Invalid Context"; break;
	case RPR_ERROR_UNIMPLEMENTED: errorMsg = L"Unimplemented"; break;
	case RPR_ERROR_INVALID_API_VERSION: errorMsg = L"Invalid API Version"; break;
	case RPR_ERROR_INTERNAL_ERROR: errorMsg = L"Internal Error"; break;
	case RPR_ERROR_IO_ERROR: errorMsg = L"IO Error"; break;
	case RPR_ERROR_UNSUPPORTED_SHADER_PARAMETER_TYPE: errorMsg = L"Unsupported Shader Parameter Type"; break;
	case RPR_ERROR_INVALID_PARAMETER_TYPE: errorMsg = L"Invalid Parameter Type"; break;
	case RPR_ERROR_UNSUPPORTED: errorMsg = L"Unsupported or incompatible device"; break;
	case RPR_ERROR_OPENCL_OUT_OF_HOST_MEMORY: errorMsg = L"OpenCL Out of host memory"; break;
	case RPR_ERROR_OPENGL: errorMsg = L"OpenGL Unspecified Error"; break;
	case RPR_ERROR_OPENCL: errorMsg = L"OpenCL Unspecified Error"; break;
	case RPR_ERROR_NULLPTR: errorMsg = L"Null Pointer"; break;
	case RPR_ERROR_NODETYPE: errorMsg = L"Node Type Unknown Error"; break;
	case RPR_ERROR_ABORTED: errorMsg = L"Aborted"; break;
	default:
		errorMsg = L"Unspecified internal error";
		break;
	}

	return errorMsg;
}

float GetUnitScale()
{
	return float(GetMasterScale(UNITS_METERS));
}

void UpdateBitmapWithToneOperator(Bitmap *bitmap) {
	//change type of toneoperator if needed
	ToneOperatorInterface* toneOpInt = (ToneOperatorInterface*)(GetCOREInterface(TONE_OPERATOR_INTERFACE));
	ToneOperator *toneOp = toneOpInt->GetToneOperator();
	if (bitmap && toneOp && toneOp->Active(0)) {
		toneOp->Update(0, Interval());
		int height = bitmap->Height();
		#pragma omp parallel for
		for (int y = 0; y < height; y++) {
			int numberOfPixels = bitmap->Width();
			PixelBufFloat l64(numberOfPixels);
			BMM_Color_fl *pixelColors = l64.Ptr();

			bitmap->GetPixels(0, y, numberOfPixels, pixelColors);

			for (int i = 0; i < numberOfPixels; i++) {
				toneOp->ScaledToRGB(pixelColors[i]);
			}
			int result = bitmap->PutPixels(0, y, numberOfPixels, pixelColors);
		}
	}
}

void DumpFRContents(const rpr_material_node& node)
{
	size_t _num_params = 0;
	int status = rprMaterialNodeGetInfo(node, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &_num_params, NULL);
	assert(status == RPR_SUCCESS);
	int num_params = int_cast(_num_params);

	DebugPrint(L"Number of shader parameters: %d\n", num_params);

	// For each parameter
	for (int i = 0; i < num_params; ++i)
	{
		std::string name;
/*
		// Get name
		size_t name_length = 0;
		status = rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, 0, NULL, &name_length);
		assert(status == RPR_SUCCESS);

		name.resize(name_length + 1);
		status = rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, name_length, &name[0], NULL);
		assert(status == RPR_SUCCESS);
*/
		// Output parameter info
		std::wstring wname = ToUnicode(name);
		DebugPrint(L"Parameter: %s", wname.c_str());

		rpr_material_node_type type;
		size_t read_count;
		status = rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(type), &type, &read_count);
		switch (type)
		{

		case RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4:
		{
			rpr_float fvalue[4] = { 0.f, 0.f, 0.f, 0.f };
			status = rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(fvalue), fvalue, nullptr);
			DebugPrint(L"\t\tValue: %f, %f, %f, %f", fvalue[0], fvalue[1], fvalue[2], fvalue[3]);
		}
		break;

		case RPR_MATERIAL_NODE_INPUT_TYPE_UINT:
		{
			rpr_int uivalue = 0;
			status = rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(uivalue), &uivalue, nullptr);
			DebugPrint(L"\t\tValue: %d", uivalue);
		}
		break;

		case RPR_MATERIAL_NODE_INPUT_TYPE_NODE:
			DebugPrint(L"\t\tValue: (node)");
			break;

		case RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE:
			DebugPrint(L"\t\tValue: (image)");
			break;

		default:
			DebugPrint(L"\t\tValue: --");
			break;
		}

		DebugPrint(L"\n");


	}
}

void DumpFRParms(const rpr_material_node& node)
{
	// Get the number of shader parameters for the shader
	size_t _num_params = 0;
	int status = rprMaterialNodeGetInfo(node, RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &_num_params, NULL);
	assert(status == RPR_SUCCESS);
	int num_params = int_cast(_num_params);

	DebugPrint(L"Number of shader parameters: %d\n", num_params);

#if 0
	// For each parameter
	for (int i = 0; i < num_params; ++i)
	{
		// Get name
		size_t name_length = 0;
		status = rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, 0, NULL, &name_length);
		assert(status == RPR_SUCCESS);

		std::string name;
		name.resize(name_length + 1);
		status = rprMaterialNodeGetInputInfo(node, i, RPR_MATERIAL_NODE_INPUT_NAME_STRING, name_length, &name[0], NULL);
		assert(status == RPR_SUCCESS);

		// Output parameter info
		std::wstring wname = ToUnicode(name);
		DebugPrint(L"Parameter: %s\n", wname.c_str());
	}
#endif
}

Bitmap* RenderTextToBitmap(const MCHAR* text)
{
	// create font
	HFONT hf = CreateFont(
		/*fontSize*/ -12,
		0, 0, 0,
		/*isBbild*/ FW_NORMAL,
		/*isItalic*/ FALSE,
		/*isUnderline*/ FALSE,
		/*isStrike*/ FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		NONANTIALIASED_QUALITY, // disable any antialiasing
		DEFAULT_PITCH | FF_DONTCARE,
		L"MS Shell Dlg"
	);
	FASSERT(hf);

	// create DC
	HDC dc = CreateCompatibleDC(NULL);
	FASSERT(dc);

	// setup DC
	SetTextColor(dc, RGB(255, 255, 255));
	SetBkColor(dc, RGB(0, 0, 0));
	SelectObject(dc, hf);

	// compute text size
	SIZE textSize;
	GetTextExtentPoint32(dc, text, int_cast(wcslen(text)), &textSize);
	int width = textSize.cx + 6;	 // add some margins
	int height = textSize.cy + 6;

	// create DIB
    RGBQUAD* bits = NULL;
	BITMAPINFO bmi;
	BITMAPINFOHEADER &head = bmi.bmiHeader;
	memset(&bmi, 0, sizeof(bmi));
	head.biSize        = sizeof(BITMAPINFOHEADER);
	head.biWidth       = width;
	head.biHeight      = height;
	head.biPlanes      = 1;
	head.biBitCount    = 32;
	head.biCompression = BI_RGB;

	HBITMAP dib = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, (void**)&bits, NULL, 0);
	FASSERT(dib);

	HBITMAP oldbitmap = (HBITMAP)SelectObject(dc, dib);

	// render text
	RECT r;
	r.left = 0;
	r.top = 2; // offset text a little bit down
	r.right = r.left + width - 1;
	r.bottom = r.top + height - 1;
	DrawText(dc, text, -1, &r, DT_CENTER | DT_VCENTER | DT_NOPREFIX);

	// create Max bitmap
	BitmapInfo bi;
	bi.SetWidth(width);
	bi.SetHeight(height);
	bi.SetType(BMM_TRUE_32);
//	bi.SetFlags(MAP_HAS_ALPHA);
	bi.SetCustomFlag(0);
	Bitmap* bm = TheManager->Create(&bi);

	// copy DIB's bits to Max bitmap
	const RGBQUAD* pBits = bits;
	for (int i = 0 ; i < height; i++)
	{
		for (int j = 0; j < width; j++, pBits++)
		{
			BMM_Color_fl color;
			float value = (pBits->rgbRed + pBits->rgbGreen + pBits->rgbBlue) / (3 * 255.0f);
			color.r = color.g = color.b = value;
			color.a = 0;
			bm->PutPixels(j, (height - i), 1, &color);
		}
	}

	// cleanup
	SelectObject(dc, GetStockObject(DEFAULT_GUI_FONT));
	if (oldbitmap)
		SelectObject(dc, oldbitmap);

	DeleteObject(hf);
	DeleteObject(dib);
	DeleteDC(dc);

	return bm;
}

static float lerp(float A, float B, float Alpha)
{
	return A * (1 - Alpha) + B * Alpha;
}

void BlitBitmap(Bitmap* Dst, Bitmap* Src, int x, int y, float alpha)
{
	int dx = Dst->Width() - x;
	if (dx > Src->Width()) dx = Src->Width();
	int dy = Dst->Height() - y;
	if (dy > Src->Height()) dy = Src->Height();

	BMM_Color_fl* bufferSrc = new BMM_Color_fl[dx];
	BMM_Color_fl* bufferDst = new BMM_Color_fl[dx];

	for (int cy = y; cy < y + dy; cy++)
	{
		Src->GetPixels(0, cy - y, dx, bufferSrc);
		Dst->GetPixels(x, cy, dx, bufferDst);

		if (alpha == 1.0f)
		{
			memcpy(bufferDst, bufferSrc, sizeof(BMM_Color_fl) * dx);
		}
		else if (alpha > 0.0f)
		{
			for (int i = 0; i < dx; i++)
			{
				bufferDst[i].r = lerp(bufferDst[i].r, bufferSrc[i].r, alpha);
				bufferDst[i].g = lerp(bufferDst[i].g, bufferSrc[i].g, alpha);
				bufferDst[i].b = lerp(bufferDst[i].b, bufferSrc[i].b, alpha);
			}
		}
		// else - alpha == 0, keep unchanged 'bufferDst'

		Dst->PutPixels(x, cy, dx, bufferDst);
	}

	delete[] bufferSrc;
	delete[] bufferDst;
}

std::wstring trim(const std::wstring& in, const std::wstring& delimiters = L" \f\n\r\t\v")
{
	std::wstring out;

	auto first = in.find_first_not_of(delimiters);
	auto last = in.find_last_not_of(delimiters);

	if (first != std::string::npos && last != std::string::npos)
	{
		out = in.substr(first, last + 1);
	}

	return out;
}

std::wstring GetCPUName()
{
	HKEY hKey = NULL;

	UINT uiRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0"), 0, KEY_READ, &hKey);
	
	if (ERROR_SUCCESS == uiRetVal)
	{
		wchar_t CpuName[256];
		DWORD valueSize = 256;

		uiRetVal = RegQueryValueEx(hKey, TEXT("ProcessorNameString"), NULL, NULL, (LPBYTE)CpuName, &valueSize);
	
		if (ERROR_SUCCESS == uiRetVal)
		{
			RegCloseKey(hKey);
			return trim(CpuName);
		}

		RegCloseKey(hKey);
	}

	return L"Unknown CPU";
}

std::wstring ComputerName()
{
	std::vector<wchar_t> buffer(MAX_COMPUTERNAME_LENGTH + 1);
	DWORD size = DWORD_cast( buffer.size() );

	if (GetComputerName(buffer.data(), &size) != 0)
	{
		return trim(buffer.data());
	}

	return L"Unknown Computer";
}

bool GetProductAndVersion(std::wstring& strProductName, std::wstring& strProductVersion, std::wstring& strCoreVersion)
{
	static bool versionChecked = false;

	static std::wstring cachedProductName;
	static std::wstring cachedProductVersion;
	static std::wstring cachedCoreVersion;

	if (!versionChecked)
	{
		// do not request version resources every time funciton is called - do this only once

		versionChecked = true;

		// get the filename of the executable containing the version resource
		TCHAR szFilename[MAX_PATH + 1] = { 0 };
		if (GetModuleFileName((HINSTANCE)&__ImageBase, szFilename, MAX_PATH) == 0)
			return false;

		// allocate a block of memory for the version info
		DWORD dummy;
		DWORD dwSize = GetFileVersionInfoSize(szFilename, &dummy);
		if (dwSize == 0)
			return false;
		std::vector<BYTE> data(dwSize);

		// load the version info
		if (!GetFileVersionInfo(szFilename, NULL, dwSize, &data[0]))
			return false;

		// get the name and version strings
		LPVOID pvProductName = NULL;
		unsigned int iProductNameLen = 0;
		LPVOID pvProductVersion = NULL;
		unsigned int iProductVersionLen = 0;
	
		if (!VerQueryValue(&data[0], _T("\\StringFileInfo\\040904b0\\ProductName"), &pvProductName, &iProductNameLen) ||
			!VerQueryValue(&data[0], _T("\\StringFileInfo\\040904b0\\ProductVersion"), &pvProductVersion, &iProductVersionLen))
			return false;

		// cache information
		cachedProductName = (LPCTSTR)pvProductName;
		cachedProductVersion = (LPCTSTR)pvProductVersion;

		// get core information
#ifdef RPR_VERSION_MAJOR_MINOR_REVISION
		std::wostringstream oss;
		oss << RPR_VERSION_MAJOR << "." << RPR_VERSION_MINOR << RPR_VERSION_REVISION;
#else
		int mj = (RPR_API_VERSION & 0xFFFF00000) >> 28;
		int mn = (RPR_API_VERSION & 0xFFFFF) >> 8;

		std::wostringstream oss;

		oss << std::hex << mj << "." << mn;
#endif

		cachedCoreVersion = oss.str();
	}

	strProductName = cachedProductName;
	strProductVersion = cachedProductVersion;
	strCoreVersion = cachedCoreVersion;

	return true;
}

std::wstring GetFriendlyUsedGPUName()
{
	int numGPUs = 0;
	std::string gpuName;
	int numIdenticalGpus = 1;
	int numOtherGpus = 0;
	
	for (int i = 0; i < ScopeManagerMax::TheManager.gpuInfoArray.size(); i++)
	{
		const GpuInfo &gpuInfo = ScopeManagerMax::TheManager.gpuInfoArray[i];
		if (gpuInfo.isUsed)
		{
			if (numGPUs == 0)
			{
				gpuName = gpuInfo.name; // remember 1st GPU name
			}
			else if (gpuInfo.name == gpuName)
			{
				numIdenticalGpus++; // more than 1 GPUs, but with identical name
			}
			else
			{
				numOtherGpus++; // different GPU used
			}
			numGPUs++;
		}
	}

	// compose string
	std::wstring str;
	if (!numGPUs)
	{
		str += _T("not used");
	}
	else
	{
		str += ToUnicode(gpuName);
		if (numIdenticalGpus > 1)
		{
			wchar_t buffer[32];
			wsprintf(buffer, _T(" x %d"), numIdenticalGpus);
			str += buffer;
		}
		if (numOtherGpus)
		{
			wchar_t buffer[32];
			wsprintf(buffer, _T(" + %d other"), numOtherGpus);
			str += buffer;
		}
	}

	return str;
}


// Enable or disable all controls located inside GroupBox with id 'groupId'. If 'groupId'
// is null, all controls will be processed. You may exclude one control from this list
// with providing its id in 'ignoreId'.
void EnableGroupboxControls(HWND dialogWnd, int groupId, bool bEnabled, std::vector<int> ignoreIds)
{
	HWND groupWnd = NULL;
	RECT groupRect;

	if (groupId)
	{
		groupWnd = GetDlgItem(dialogWnd, groupId);
		if (!groupWnd) return;
		GetWindowRect(groupWnd, &groupRect);
	}

	std::set<HWND> ignore;
	for (auto &i : ignoreIds)
	{
		HWND h = GetDlgItem(dialogWnd, i);
		if (h)
			ignore.insert(h);
	}

	for (HWND childWnd = GetWindow(dialogWnd, GW_CHILD); childWnd; childWnd = GetWindow(childWnd, GW_HWNDNEXT))
	{
		if (ignore.find(childWnd) != ignore.end())
			continue;
		if (childWnd == groupWnd) continue;

		bool shouldChange = (groupWnd == NULL);

		if (!shouldChange)
		{
			RECT childRect;
			GetWindowRect(childWnd, &childRect);
			RECT tmp;
			shouldChange = IntersectRect(&tmp, &groupRect, &childRect) != 0;
		}

		if (shouldChange)
		{
			EnableWindow(childWnd, bEnabled);
		}
	}
}


bool FileExists(const TCHAR* name)
{
	DWORD dwAttrib = GetFileAttributes(name);

	return
		dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool FolderExists(const TCHAR* path)
{
	DWORD dwAttrib = GetFileAttributes(path);

	return
		dwAttrib != INVALID_FILE_ATTRIBUTES &&
		(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

FIRERENDER_NAMESPACE_END

std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;

	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;

	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}
