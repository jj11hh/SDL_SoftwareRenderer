#pragma once

#include <eigen3/Eigen/Eigen>
#include <memory>

struct RenderContext;

class IShader
{
public:

	struct ShaderDescriptor {
		std::size_t inputVertexSize;
		std::size_t positionPlacement;

		Eigen::Vector4f extractPosition(const Eigen::VectorXf& vertShaderOut) const noexcept {
			return vertShaderOut.segment<4>(positionPlacement);
		};
	};

	virtual const ShaderDescriptor& getDesc() noexcept = 0;
	virtual void vertexShader(const RenderContext *pCtx, const void *inputDatas, Eigen::VectorXf &vertexOut) noexcept = 0;
	virtual void fragmentShader(const RenderContext *pCtx, const Eigen::VectorXf &inputData, Eigen::Vector4f &colorOut) noexcept = 0;
};

