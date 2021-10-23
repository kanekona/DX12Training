#pragma once

#include <vector>
#include <string>
#include "Engine\System\System.h"

class Texture
{
public:
	Texture(const std::string& inFileName);

	std::vector<char>* GetData();

	unsigned int GetWidth() const;
	unsigned int GetHeight() const;
	unsigned int GetRowPitch() const;
	DXGI_FORMAT GetFormat() const;

private:
	std::vector<char> data;

	unsigned int width;
	unsigned int height;
	DXGI_FORMAT format;
};