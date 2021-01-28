#pragma once

#include <cstdint>
#include <eigen3/Eigen/Eigen>
#include "RenderContext.h"

class SoftwareRenderer;

class Rasterizer
{
public:
	static void bresenhamDrawLine(uint32_t* surface, int pitch, int w, int h, int x1, int y1, int x2, int y2, uint32_t color);
	static void setPixel(uint32_t* surface, int pitch, int w, int h, int x, int y, uint32_t color);
	static void drawTriangleSample(SoftwareRenderer *renderer, RenderContext *ctx, Eigen::VectorXf vertices[3]);
	static void drawTriangleWireframe(SoftwareRenderer *renderer, RenderContext *ctx, Eigen::VectorXf vertices[3]);
};

