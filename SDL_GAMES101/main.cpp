#include <SDL/SDL.h>
#include <eigen3/Eigen/Eigen>
#include "IShader.h"
#include "ShaderUtils.h"
#include "SoftwareRenderer.h"
#include "RenderContext.h"

#include <windows.h>
#include <cmath>
#include <sstream>
#include <iomanip>

#undef near
#undef far

using v3f = Eigen::Vector3f;
using v4f = Eigen::Vector4f;
using mat4f = Eigen::Matrix4f;
using mat3f = Eigen::Matrix3f;
using AAf = Eigen::AngleAxisf;

constexpr float PI = 3.1415926535897932384f;

mat4f make_view_matrix(v3f cameraAt, v3f lookAt, v3f upAt) {
	
	mat4f rotate, translate;

	translate <<
		1, 0, 0, -cameraAt.x(),
		0, 1, 0, -cameraAt.y(),
		0, 0, 1, -cameraAt.z(),
		0, 0, 0, 1;

	v3f newX = lookAt.cross(upAt);

	rotate << 
		newX.x(), newX.y(), newX.z(), 0,
		upAt.x(), upAt.y(), upAt.z(), 0,
		-lookAt.x(), -lookAt.y(), -lookAt.z(), 0,
		0,        0,        0,        1;

	return rotate * translate;
}

mat4f make_ortho_matrix(float left, float right, float top, float bottom, float near, float far) {
	mat4f S = v4f(2 / (right - left), 2 / (top - bottom), 2 / (near - far), 1).asDiagonal();
	mat4f T;
	T <<
		1, 0, 0, -(right + left) / 2,
		0, 1, 0, -(top + bottom) / 2,
		0, 0, 1, -(near + far) / 2,
		0, 0, 0, 1;

	return S * T;
}

mat4f make_prespective_matrix(float fovy, float aspect, float near, float far) {
	float t = fabsf(near) * tanf(fovy / 2);
	float r = t / aspect;

	mat4f mat;
	mat <<
		near / r, 0, 0, 0,
		0, near / t, 0, 0,
		0, 0, (far + near) / (near - far), 2 * far * near / (far - near),
		0, 0, 1, 0;

	return mat;
}

class BoxDrawer {
	class Shader : public IShader, private ShaderUtils {
		mat4f modelview;

		const ShaderDescriptor desc = {sizeof(v3f), 0};
		const Eigen::Vector4f colorOfFaces[6] = {
			{1.0f, 0.0f, 0.0f, 1.0f},
			{0.0f, 1.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 1.0f, 1.0f},
			{1.0f, 1.0f, 0.0f, 1.0f},
			{1.0f, 0.0f, 1.0f, 1.0f},
			{0.0f, 1.0f, 1.0f, 1.0f},
		};

	public:
		const ShaderDescriptor& getDesc() noexcept final {
			return desc;
		}
		void vertexShader(const RenderContext* pCtx, const void* inputDatas, Eigen::VectorXf &vertex_out) noexcept final {
			auto &vertex_in = extractParam<v3f>(inputDatas);

			vertex_out.resize(7);
			vertex_out.segment<4>(0) = modelview * v4f(vertex_in.x(), vertex_in.y(), vertex_in.z(), 1.0f);
			vertex_out.segment<3>(4) = vertex_in * 0.5 + v3f(0.2, 0.2, 0.2);
		};
		void fragmentShader(const RenderContext* pCtx, const Eigen::VectorXf &inputData, Eigen::Vector4f &color_out) noexcept final {
			int face = pCtx->primitiveID / 2;

			//color_out = colorOfFaces[face];
			color_out.segment<3>(0) = inputData.segment<3>(4);
			color_out(3) = 1.0;
		};

		void setModelView(mat4f modelview) {
			this->modelview = modelview;
		}
	};

	const v3f box_points[8] = {
		{0.0f, 0.0f, 0.0f}, // 0
		{0.0f, 0.0f, 1.0f}, // 1
		{0.0f, 1.0f, 0.0f}, // 2
		{0.0f, 1.0f, 1.0f}, // 3
		{1.0f, 0.0f, 0.0f}, // 4
		{1.0f, 0.0f, 1.0f}, // 5
		{1.0f, 1.0f, 0.0f}, // 6
		{1.0f, 1.0f, 1.0f}, // 7
	};

	const uint32_t box_indices[36] = {
		1,5,3,3,5,7,
		5,4,7,7,4,6,
		4,0,6,6,0,2,
		0,1,3,3,2,0,
		7,6,3,3,6,2,
		0,4,1,1,4,5
	};

	Shader shader;
	std::unique_ptr<SoftwareRenderer> renderer;

public:
	BoxDrawer(SDL_Surface *surface) {
		renderer = std::make_unique<SoftwareRenderer>(reinterpret_cast<uint32_t*>(surface->pixels), surface->w, surface->h, surface->pitch);

		//swrenderer.setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES_WIREFRAME);
		renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES);
		renderer->bindShader(&shader);
		renderer->setBackfaceCull(true);
		renderer->setVertexArray(box_points, 8);

	}

	void draw(mat4f &trans) {

		v3f camAt = { 0, 0, -5 };
		v3f lookAt = (v3f(0.0f, 0.0f, 1.0f) - camAt).normalized();
		v3f upAt = - v3f(0.0f, 1.0f, 0.0f).normalized();

		mat4f Mview = make_view_matrix(camAt, lookAt, upAt);
		//mat4f Mortho = make_ortho_matrix(-2, 2, 1.5, -1.5, -2, -10);
		mat4f Mortho = make_prespective_matrix(PI * 60 / 360, 3.0f / 4.0f, -2, -10);
		mat4f MVP = Mortho * Mview * trans;
		shader.setModelView(MVP);
		renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES);
		renderer->drawIndexed(box_indices, 36);
		renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES_WIREFRAME);
		renderer->drawIndexed(box_indices, 36);
	}

};

class TriangleDrawer {
	struct Vertex {
		Eigen::Vector2f pos;
		Eigen::Vector3f color;
	};

	class Shader : public IShader, private ShaderUtils {
		const ShaderDescriptor desc = { sizeof(Vertex), 0 };
	public:
		const ShaderDescriptor& getDesc() noexcept final {return desc;}

		void vertexShader(const RenderContext* pCtx, const void* inputDatas, Eigen::VectorXf& vertex_out) noexcept final {
			auto input = extractParam<Vertex>(inputDatas);
			vertex_out.resize(7);
			vertex_out.segment<4>(0) = Eigen::Vector4f(input.pos.x(), input.pos.y(), -1.0f, 1.0f);
			vertex_out.segment<3>(4) = input.color;
		}

		void fragmentShader(const RenderContext* pCtx, const Eigen::VectorXf& inputData, Eigen::Vector4f& color_out) noexcept final {
			color_out.segment<3>(0) = inputData.segment<3>(4);
			// alpha
			color_out(3) = 1.0f;
		}
	};

	const Vertex vertices[3] = {
		{{ -0.5f, 0.5f }, {1.0f, 0.0f, 0.0f}},
		{{ 0.0f, -0.5f }, {0.0f, 0.0f, 1.0f}},
		{{ 0.5f,  0.5f }, {0.0f, 1.0f, 0.0f}}, 
	};

	Shader shader;
	std::unique_ptr<SoftwareRenderer> renderer;
public:
	TriangleDrawer(SDL_Surface* surface) {
		renderer = std::make_unique<SoftwareRenderer>(reinterpret_cast<uint32_t*>(surface->pixels), surface->w, surface->h, surface->pitch);
		renderer->setBackfaceCull(true);
		renderer->bindShader(&shader);
		renderer->setVertexArray(vertices, sizeof(vertices) / sizeof(Vertex));


	}
	void draw(mat4f& trans) {
		renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES);
		renderer->draw();
		renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES_WIREFRAME);
		renderer->draw();
	}
};


int main(int argc, char* args[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

	auto *screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE);

	mat4f rotate = mat4f::Identity();
	BoxDrawer renderer(screen);
	//TriangleDrawer renderer(screen);

	uint32_t lastTime = 0, currentTime;
	uint32_t fpsCount = 0;

	for (;;) {
		currentTime = SDL_GetTicks();
		uint32_t timeElasped = currentTime - lastTime;
		fpsCount++;
		if (timeElasped >= 1000) {
			std::stringstream ss;
			ss << "Yiheng's Software Renderer - FPS: " << fpsCount;
			SDL_WM_SetCaption(ss.str().c_str(), nullptr);
			
			fpsCount = 0;
			lastTime = currentTime;
		}

		SDL_FillRect(screen, nullptr, 0);
		renderer.draw(rotate);
		SDL_Flip(screen);

		SDL_Event event;
		SDL_PollEvent(&event);

		mat3f rotate_acc = mat3f::Identity();

		if (event.type == SDL_QUIT) {
			break;
		}
		else if (event.type == SDL_KEYDOWN) {
			switch (event.key.keysym.sym) {
			case SDLK_UP:
				rotate_acc = AAf((1.0f / 180) * PI, v3f(1,0,0)).toRotationMatrix();
				break;
			case SDLK_DOWN:
				rotate_acc = AAf((-1.0f / 180) * PI, v3f(1,0,0)).toRotationMatrix();
				break;
			case SDLK_LEFT:
				rotate_acc = AAf((1.0f / 180) * PI, v3f(0,1,0)).toRotationMatrix();
				break;
			case SDLK_RIGHT:
				rotate_acc = AAf((-1.0f / 180) * PI, v3f(0,1,0)).toRotationMatrix();
				break;


			}
		}

		mat4f rotate_acc_4f = mat4f::Identity();
		rotate_acc_4f.block(0, 0, 3, 3) = rotate_acc;

		rotate = rotate_acc_4f * rotate;

		//SDL_Delay(1000 / 60); // Running at 30 frame pre second
	}

	SDL_Quit();

	return 0;
}