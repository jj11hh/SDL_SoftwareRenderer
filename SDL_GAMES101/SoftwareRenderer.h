#pragma once

#include "IShader.h"

class SoftwareRenderer
{
	friend class Rasterizer;
public:
	enum class DrawStyle {
		TRIANGLES,
		TRIANGLES_WIREFRAME,
	};

private:
	uint32_t* frameBuffer;
	int w;
	int h;
	int pitch;

	IShader* pShader = nullptr;
	const void* pVertexArray = nullptr;
	std::size_t vertexArrayLength = 0;
	DrawStyle drawStyle = DrawStyle::TRIANGLES;
	bool backfaceCull = false;
	uint8_t sampleDensity = 0;
	std::vector<float> zBuffer;
	bool zBufferEnabled = false;
	bool perspectiveCorrectEnabled = false;

public:


	SoftwareRenderer(uint32_t* frameBuffer, int w, int h, int pitch);

	void bindShader(IShader *pShader);
	void setVertexArray(const void* vertexArray, std::size_t size);
	void setDrawStyle(DrawStyle drawStyle);
	void setBackfaceCull(bool enable);
	void setSampleDensity(uint8_t density);
	void setZBufferEnabled(bool enable);
	void setPerspectiveCorrect(bool enable);
	void clearZBuffer();
	std::vector<float>& getZbuffer();

	void draw();
	void drawIndexed(const uint32_t* indices, std::size_t size);
};

