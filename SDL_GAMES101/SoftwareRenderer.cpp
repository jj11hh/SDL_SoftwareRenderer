#include "SoftwareRenderer.h"
#include "Rasterizer.h"
#include "RenderContext.h"
#include <memory>

SoftwareRenderer::SoftwareRenderer(uint32_t* frameBuffer, int w, int h, int pitch): frameBuffer(frameBuffer), w(w), h(h), pitch(pitch)
{
	zBuffer.resize(std::size_t(w) * h);
	clearZBuffer();
}

void SoftwareRenderer::bindShader(IShader* pShader)
{
	this->pShader = pShader;
}

void SoftwareRenderer::setVertexArray(const void* vertexArray, std::size_t size)
{
	this->pVertexArray = vertexArray;
	this->vertexArrayLength = size;
}

void SoftwareRenderer::setDrawStyle(DrawStyle drawStyle)
{
	this->drawStyle = drawStyle;
}

void SoftwareRenderer::setBackfaceCull(bool enable)
{
	backfaceCull = enable;
}

void SoftwareRenderer::setSampleDensity(uint8_t density)
{
	sampleDensity = density;
}

void SoftwareRenderer::setZBufferEnabled(bool enable)
{
	zBufferEnabled = enable;
}

void SoftwareRenderer::setPerspectiveCorrect(bool enable)
{
	perspectiveCorrectEnabled = enable;
}

void SoftwareRenderer::clearZBuffer()
{
	std::fill(zBuffer.begin(), zBuffer.end(), -1);
}

std::vector<float>& SoftwareRenderer::getZbuffer()
{
	return zBuffer;
}

void SoftwareRenderer::draw()
{
	assert(this->pShader != nullptr && "No valid shader is bond!");

	std::size_t inputElemSize = pShader->getDesc().inputVertexSize;

	const uint8_t* inputElems = reinterpret_cast<const uint8_t *>(pVertexArray);

	Eigen::VectorXf outputElems[3];

	RenderContext ctx;
	ctx.renderer = this;

	for (std::size_t i = 0; i < vertexArrayLength; i += 3) {
		for (std::size_t j = 0; j < 3; ++j) {
			auto inputVertexData = reinterpret_cast<const void*>(inputElems + (i + j) * inputElemSize);

			ctx.vertexID = i + j;
			pShader->vertexShader(ctx, inputVertexData, outputElems[j]);
		}

		if (drawStyle == DrawStyle::TRIANGLES)
			Rasterizer::drawTriangleSample(this, &ctx, outputElems);
		else if (drawStyle == DrawStyle::TRIANGLES_WIREFRAME)
			Rasterizer::drawTriangleWireframe(this, &ctx, outputElems);

		ctx.primitiveID = i / 3;
	}
}

void SoftwareRenderer::drawIndexed(const uint32_t* indices, std::size_t size)
{
	assert(this->pShader != nullptr && "No valid shader is bond!");

	std::size_t inputElemSize = pShader->getDesc().inputVertexSize;

	const uint8_t* inputElems = reinterpret_cast<const uint8_t *>(pVertexArray);

	Eigen::VectorXf outputElems[3];

	RenderContext ctx;
	ctx.renderer = this;


	for (std::size_t i = 0; i < size; i += 3) {
		for (std::size_t j = 0; j < 3; ++j) {
			std::size_t index = indices[i + j];
			assert(index < vertexArrayLength && "Vertex array out of index!");

			auto inputVertexData = reinterpret_cast<const void*>(inputElems + index * inputElemSize);
			ctx.vertexID = i + j;
			pShader->vertexShader(ctx, inputVertexData, outputElems[j]);
		}

		if (drawStyle == DrawStyle::TRIANGLES)
			Rasterizer::drawTriangleSample(this, &ctx, outputElems);
		else if (drawStyle == DrawStyle::TRIANGLES_WIREFRAME)
			Rasterizer::drawTriangleWireframe(this, &ctx, outputElems);

		ctx.primitiveID++;
	}

}
