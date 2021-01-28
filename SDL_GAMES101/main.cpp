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
	const float l = left;
	const float r = right;
	const float t = top;
	const float b = bottom;
	const float n = near;
	const float f = far;

	mat4f Mortho;
	Mortho <<
		2 / (r - l), 0, 0, (r + l) / (l - r),
		0, 2 / (t - b), 0, (t + b) / (b - t),
		0, 0, 2 / (n - f), (n + f) / (f - n),
		0, 0, 0, 1;

	return Mortho;
}

mat4f make_prespective_matrix(float fovy, float aspect, float near, float far) {
	float t = fabsf(near) * tanf(fovy / 2);
	float r = t / aspect;

	mat4f persp2ortho;
	persp2ortho <<
		near, 0, 0, 0,
		0, near, 0, 0,
		0, 0, near + far, -near * far,
		0, 0, 1, 0;
	return make_ortho_matrix(-r, r, t, -t, near, far) * persp2ortho;
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
		void vertexShader(const RenderContext &ctx, const void* inputDatas, Eigen::VectorXf &vertex_out) noexcept final {
			auto &vertex_in = extractParam<v3f>(inputDatas);

			vertex_out.resize(7);
			vertex_out.segment<4>(0) = modelview * v4f(vertex_in.x(), vertex_in.y(), vertex_in.z(), 1.0f);
			vertex_out.segment<3>(4) = vertex_in * 0.5f + v3f(0.2f, 0.2f, 0.2f);
		};
		void fragmentShader(const RenderContext &ctx, const Eigen::VectorXf &inputData, Eigen::Vector4f &color_out) noexcept final {
			int face = ctx.primitiveID / 2;

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
		renderer->setZBufferEnabled(false);
		renderer->setPerspectiveCorrect(false);
		renderer->setVertexArray(box_points, 8);
		renderer->setSampleDensity(0);
	}

	void draw(mat4f &trans) {

		v3f camAt = { 0, 0, -5 };
		v3f lookAt = (v3f(0.0f, 0.0f, 1.0f) - camAt).normalized();
		v3f upAt = - v3f(1.0f, 1.0f, 0.0f).normalized();

		mat4f Mview = make_view_matrix(camAt, lookAt, upAt);
		//mat4f Mortho = make_ortho_matrix(-2, 2, 1.5, -1.5, -2, -10);
		mat4f Mortho = make_prespective_matrix(PI * 60 / 360, 3.0f / 4.0f, -2, -10);
		mat4f MVP = Mortho * Mview * trans;
		shader.setModelView(MVP);
		renderer->clearZBuffer();
		renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES);
		renderer->drawIndexed(box_indices, 36);
		//renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES_WIREFRAME);
		//renderer->drawIndexed(box_indices, 36);
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

		void vertexShader(const RenderContext &ctx, const void* inputDatas, Eigen::VectorXf& vertex_out) noexcept final {
			auto input = extractParam<Vertex>(inputDatas);
			vertex_out.resize(7);
			vertex_out.segment<4>(0) = Eigen::Vector4f(input.pos.x(), input.pos.y(), -1.0f, 1.0f);
			vertex_out.segment<3>(4) = input.color;
		}

		void fragmentShader(const RenderContext &ctx, const Eigen::VectorXf& inputData, Eigen::Vector4f& color_out) noexcept final {
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

class SphereDrawer {
	using Vertex = Eigen::Vector3f;
	std::unique_ptr<SoftwareRenderer> renderer;

private:

	class Shader : public IShader, private ShaderUtils {
		const ShaderDescriptor desc = { sizeof(Vertex), 0 };
		mat4f modelview;
	public:

		Eigen::Vector3f lightPosition;
		Eigen::Vector3f cameraPosition;

		const ShaderDescriptor& getDesc() noexcept final {return desc;}

		void vertexShader(const RenderContext &ctx, const void* inputDatas, Eigen::VectorXf& vertex_out) noexcept final {
			auto input = extractParam<Vertex>(inputDatas);
			vertex_out.resize(10);
			// position :vec4f
			// norm :vec3f
			// world_position :vec3f
			Eigen::Vector4f position;
			Eigen::Vector3f norm;
			position.segment<3>(0) = input;
			position.w() = 1.0;
			position = modelview * position;
			norm = input;

			vertex_out.segment<4>(0) = position;
			vertex_out.segment<3>(4) = norm;
			vertex_out.segment<3>(7) = input;
		}

		void fragmentShader(const RenderContext &ctx, const Eigen::VectorXf& inputData, Eigen::Vector4f& color_out) noexcept final {
			Eigen::Vector3f color(0.3f, 0.3f, 0.0f);
			if ((ctx.primitiveID / 2) % 2 == 0) {
				color = { 0.0f, 0.3f, 0.3f };
			}
			Eigen::Vector3f norm = inputData.segment<3>(4).normalized();
			Eigen::Vector3f world_pos = inputData.segment<3>(7);

			const Eigen::Vector3f lightDirection = (lightPosition - world_pos).normalized();
			const Eigen::Vector3f cameraDirection = (cameraPosition - world_pos).normalized();

			const float lightDistance = (world_pos - lightPosition).norm();

			const Eigen::Vector3f h = (cameraDirection + lightDirection).normalized();

			const float diffuse = 1.0f * max(norm.dot(lightDirection), 0.0f);
			const float specular = 0.5f * std::powf(max(norm.dot(h), 0.0f), 32);
			const float ambient = 0.7f;

			color_out.segment<3>(0) = (diffuse + ambient) * color + specular * v3f(1.0f,1.0f,1.0f);
			color_out(3) = 1.0f;
		}

		void setModelView(mat4f& modelview) {
			this->modelview = modelview;
		}
	};

	std::vector<Eigen::Vector3f> vertices;
	std::vector<uint32_t> indices;
	Shader shader;

public:
	SphereDrawer(SDL_Surface* surface, int latDiv, int longDiv) {
		assert(latDiv >= 3);
		assert(longDiv >= 3);

		renderer = std::make_unique<SoftwareRenderer>(reinterpret_cast<uint32_t*>(surface->pixels),
			surface->w, surface->h, surface->pitch);

		constexpr float radius = 1.0f;

		const float latAngle = PI / latDiv;
		const float longAngle = 2*PI / longDiv;

		Eigen::Vector3f base(0.0f, 0.0f, radius);

		// Pole vertices
		vertices.push_back(base);

		Eigen::AngleAxisf rotateX(latAngle, Eigen::Vector3f(1.0f, 0.0f, 0.0f));
		Eigen::AngleAxisf rotateZ(longAngle, Eigen::Vector3f(0.0f, 0.0f, 1.0f));

		const uint32_t num_vertices = (latDiv - 1) * longDiv + 2;
		const uint32_t north_pole = 0;
		const uint32_t south_pole = num_vertices - 1;

		for (int i = 1; i < latDiv; ++i) {
			base = rotateX * base;
			for (int j = 0; j < longDiv; ++j) {
				if (i == 1) {
					indices.push_back(north_pole);
					indices.push_back((i - 1)*longDiv + j + 1);
					if (j == longDiv - 1)
						indices.push_back((i - 1)*longDiv + 1);
					else
						indices.push_back((i - 1)*longDiv + j + 2);
				}
				else if (j != longDiv - 1) {
					indices.push_back((i - 2)*longDiv + j + 1);
					indices.push_back((i - 1)*longDiv + j + 1);
					indices.push_back((i - 1)*longDiv + j + 2);
					indices.push_back((i - 2)*longDiv + j + 1);
					indices.push_back((i - 1)*longDiv + j + 2);
					indices.push_back((i - 2)*longDiv + j + 2);
				}
				else {
					indices.push_back((i - 2)*longDiv + j + 1);
					indices.push_back((i - 1)*longDiv + j + 1);
					indices.push_back((i - 1)*longDiv + 1);
					indices.push_back((i - 2)*longDiv + j + 1);
					indices.push_back((i - 1)*longDiv + 1);
					indices.push_back((i - 2)*longDiv + 1);
				}
				vertices.push_back(base);
				base = rotateZ * base;
			}
		}

		vertices.push_back({ 0.0f, 0.0f, -radius });

		for (int i = 1; i < longDiv; ++i) {
			indices.push_back(south_pole);
			indices.push_back(south_pole - i);
			indices.push_back(south_pole - i - 1);
		}
		indices.push_back(south_pole);
		indices.push_back(south_pole - longDiv);
		indices.push_back(south_pole - 1);

		shader.lightPosition = v3f(0.0f, 0.0f, 10.0f);

		renderer->bindShader(&shader);
		renderer->setBackfaceCull(true);
		renderer->setVertexArray(vertices.data(), vertices.size());
		renderer->setSampleDensity(0);
		renderer->setZBufferEnabled(false);
		renderer->setPerspectiveCorrect(true);
	}

	void draw(v3f camAt) {
		v3f lookAt = (v3f(0.0f, 0.0f, 0.0f) - camAt).normalized();
		v3f upAt = lookAt.cross(v3f(0.0f, 1.0f, 0.0f)).cross(lookAt).normalized();

		mat4f Mview = make_view_matrix(camAt, lookAt, upAt);
		//mat4f Mproj = make_ortho_matrix(-2, 2, 1.5, -1.5, -2, -10);
		mat4f Mproj = make_prespective_matrix(PI * 60 / 360, 3.0f / 4.0f, -2, -10);
		mat4f MVP = Mproj * Mview;
		shader.cameraPosition = camAt;

		shader.setModelView(MVP);
		//renderer->clearZBuffer();
		renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES);
		renderer->drawIndexed(indices.data(), indices.size());
		//renderer->setDrawStyle(SoftwareRenderer::DrawStyle::TRIANGLES_WIREFRAME);
		//renderer->drawIndexed(indices.data(), indices.size());
	}

};

int main(int argc, char* args[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

	auto *screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE);

	float yaw = 0.0f;
	float pitch = 0.0f;

	//BoxDrawer renderer(screen);
	//TriangleDrawer renderer(screen);
	SphereDrawer renderer(screen, 10, 20);

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

		v3f camPosition = { 0,0,-5 };

		camPosition = AAf(pitch, v3f(1, 0, 0)) * camPosition;
		camPosition = AAf(yaw, v3f(0, 0, 1)) * camPosition;

		SDL_FillRect(screen, nullptr, 0);
		renderer.draw(camPosition);
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
				yaw += (1.0f / 180.0f) * PI;
				break;
			case SDLK_DOWN:
				yaw -= (1.0f / 180.0f) * PI;
				break;
			case SDLK_LEFT:
				pitch -= (1.0f / 180.0f) * PI;
				break;
			case SDLK_RIGHT:
				pitch += (1.0f / 180.0f) * PI;
				break;


			}
		}

		camPosition = rotate_acc * camPosition;

		//SDL_Delay(1000 / 60); // Running at 30 frame pre second
	}

	SDL_Quit();

	return 0;
}