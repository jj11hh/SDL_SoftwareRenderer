#pragma once

#include <cstdint>
#include <eigen3/Eigen/Eigen>
#include "IShader.h"

class Rasterizer
{
	static void bresenhamDrawLine(uint32_t* surface, int pitch, int w, int h, int x1, int y1, int x2, int y2, uint32_t color);
	static void setPixel(uint32_t* surface, int pitch, int w, int h, int x, int y);
	static void drawTriangleSample(uint32_t* surface, int pitch, int w, int h, IShader& shader, const void* vertices);
};

