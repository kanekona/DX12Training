
#include "Engine\Core\Texture\Texture.h"
#include <fstream>
#include <wincodec.h>

Texture::Texture(const std::string& inFileName)
{
	IWICImagingFactory* factory = nullptr;
	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* framedecode = nullptr;

	HRESULT result;

	result = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&factory);

	if (result != S_OK)
	{
		return;
	}

	LPCWSTR fileName = L"resources/texture/a1zunko102.png";

	result = factory->CreateDecoderFromFilename(fileName, NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &decoder);

	if (result != S_OK)
	{
		return;
	}

	result = decoder->GetFrame(0, &framedecode);

	if (result != S_OK)
	{
		return;
	}

	WICPixelFormatGUID pixelFormatguid;

	result = framedecode->GetPixelFormat(&pixelFormatguid);

	if (result != S_OK)
	{
		return;
	}

	result = framedecode->GetSize(&width, &height);

	if (result != S_OK)
	{
		return;
	}

	// ‘½•ªRGBA‚È‚ñ‚Å4‚Å‚¢‚¢‚Í‚¸
	data.resize(width*height * 4);

	result = framedecode->CopyPixels(NULL, width * 4, width*height * 4, (BYTE*)data.data());

	if (pixelFormatguid == GUID_WICPixelFormat32bppRGBA)
	{
		format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
	}
	else if (pixelFormatguid == GUID_WICPixelFormat32bppBGRA)
	{
		format = DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM;
	}

	if (result != S_OK)
	{
		return;
	}

	factory->Release();
	decoder->Release();
	framedecode->Release();
}

std::vector<char>* Texture::GetData()
{
	return &data;
}

unsigned int Texture::GetWidth() const
{
	return width;
}

unsigned int Texture::GetHeight() const
{
	return height;
}

unsigned int Texture::GetRowPitch() const
{
	return GetWidth() * 4;
}

DXGI_FORMAT Texture::GetFormat() const
{
	return format;
}