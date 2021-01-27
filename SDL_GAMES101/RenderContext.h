#pragma once

#include <cstdint>

class IShader;

struct RenderContext
{
	uint32_t vertexID = 0;
	uint32_t primitiveID = 0;
};

