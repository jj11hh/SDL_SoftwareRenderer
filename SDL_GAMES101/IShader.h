#pragma once

#include <eigen3/Eigen/Eigen>

class IShader
{
public:

	class IVertexOutputDesc
	{
	public:
		IVertexOutputDesc() = delete;

		virtual Eigen::Vector4f getPosition(const void *vertData) noexcept = 0;

		virtual std::size_t instanceSize() noexcept = 0;
		virtual void initializeData(void* data) noexcept = 0;
		virtual void doInterpolation(const Eigen::Vector3f &weights, const void *vertDatas, void *outputData) noexcept = 0;
	};

	virtual void getVertexOutputDesc(IVertexOutputDesc& descToFill) noexcept = 0;
	virtual std::size_t outputSizeOfVertices() noexcept = 0;
	virtual void vertexShader(int vertexIndex, void *inputDatas) noexcept = 0;
	virtual void fragmentShader(void *inputDatas, Eigen::Vector4f &colorOutput) noexcept = 0;
};

