#pragma once

#include <cstdint>
#include <functional>

class IShader;
class SoftwareRenderer;

struct RenderContext
{
	SoftwareRenderer *renderer;

	std::function<void()> discard;
	uint32_t vertexID = 0;
	uint32_t primitiveID = 0;
};

