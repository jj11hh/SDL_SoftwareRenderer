#pragma once

#include <eigen3/Eigen/Eigen>

class ShaderUtils
{
protected:
	template <typename T>
	static inline const T& extractParam(const void* ptr) {
		return *reinterpret_cast<const T*>(ptr);
	}
};

