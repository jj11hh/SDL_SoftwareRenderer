#include <SDL/SDL.h>
#include <eigen3/Eigen/Eigen>
#include "Gfx.h"

#include <windows.h>
#include <cmath>
#include <sstream>

#undef near
#undef far

using v2i = Gfx::v2i;
using v3f = Eigen::Vector3f;
using v4f = Eigen::Vector4f;
using mat4f = Eigen::Matrix4f;
using mat3f = Eigen::Matrix3f;
using AAf = Eigen::AngleAxisf;

constexpr float PI = 3.1415926535897932384f;

mat4f make_view_matrix(v3f cameraAt, v3f lookAt, v3f upAt) {
	
	mat4f Rview, Tview;

	Rview.fill(0);
	Tview <<	1, 0, 0, cameraAt(0),
				0, 1, 0, cameraAt(1),
				0, 0, 1, cameraAt(2),
				0, 0, 0, 1;

	v3f newX = lookAt.cross(upAt);

	Rview << newX.x(), newX.y(), newX.z(), 0,
			 upAt.x(), upAt.y(), upAt.z(), 0,
			 -lookAt.x(), -lookAt.y(), -lookAt.z(), 0,
			 0,        0,        0,        1;

	return Rview * Tview;
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

class Renderer {

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

	const uint8_t box_indices[36] = {
		0,6,4,0,2,6,
		0,1,2,1,3,2,
		1,5,7,1,7,3,
		0,6,5,5,6,7,
		2,7,6,2,3,7,
		0,4,5,0,5,1,
	};

public:

	void draw(Gfx &ctx, mat4f &trans) {

	//	^ y
	//	|   -\ z
	//	|   /
	//	|  /
	//	| /
	//	|/o
	//	-------------> x

		v3f camAt = { 0, 0, -2 };
		v3f lookAt = (v3f(0.0f, 0.0f, 1.0f) - camAt).normalized();
		v3f upAt = - v3f(1.0f, 0.0f, 0.0f).cross(lookAt).normalized();


		//camAt = trans *  camAt;
		//lookAt = trans *  lookAt;
		//upAt = trans * upAt;

		mat4f Mview = make_view_matrix(camAt, lookAt, upAt) * trans;
		mat4f Mortho = make_ortho_matrix(-2, 2, 1.5, -1.5, 0, 10);
		mat4f MVP = Mortho * Mview;

		for (int i = 0; i < sizeof(box_indices) / sizeof(uint8_t); i += 3) {
			v3f triangle[3];
			Gfx::Color colors[3];

			for (int j = 0; j < 3; ++j) {
	#define _P box_points[box_indices[i+j]]
				v4f point(_P.x(), _P.y(), _P.z(), 1);
				point = MVP * point;
				triangle[j].x() = point.x() * 400 + 400;
				triangle[j].y() = 600 - (point.y() * 300 + 300);
				triangle[j].z() = point.z();

				colors[j] = { static_cast<uint8_t>(_P.x() * 192),
					static_cast<uint8_t>(_P.y() * 192), 
					static_cast<uint8_t>(_P.z() * 192), 255 };
	#undef _P
			}

			std::ostringstream ss;
			ss << "(" << triangle[0](0) << ", " << triangle[0](1) << "), ";
			ss << "(" << triangle[1](0) << ", " << triangle[1](1) << "), ";
			ss << "(" << triangle[2](0) << ", " << triangle[2](1) << ")" << std::endl;

			OutputDebugStringA(ss.str().c_str());

			//ctx.drawTriangle(triangle, colors);
			ctx.strokeTriangle(triangle, {255, 0, 0});
		}
	}

};


int main(int argc, char* args[]) {
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);

	auto *screen = SDL_SetVideoMode(800, 600, 32, SDL_SWSURFACE);
	auto ctx = Gfx(screen);


	// Draw Triangle
//
//	v2i prim[] = {
//		v2i(100, 400),
//		v2i(100, 100),
//		v2i(500, 250),
//		v2i(100, 400),
//	};
//
//	for (uint8_t i = 0; i < sizeof(prim) / sizeof(v2i) - 1; ++i) {
//		ctx.drawLine(prim[i], prim[i + 1], { (uint8_t)(64*i), (uint8_t)(255-64*i), 0, 255 });
//		std::cout << prim[i] << ", " << prim[i + 1] << std::endl;
//		SDL_Flip(screen);
//		SDL_Delay(1000);
//	}

	mat4f rotate = mat4f::Identity();

	Renderer renderer;

	for (;;) {
		SDL_FillRect(screen, nullptr, Gfx::Color(0, 0, 0, 0).pixel);
		ctx.clearZBuffer();
		renderer.draw(ctx, rotate);
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
				rotate_acc = AAf((5.0f / 180) * PI, v3f(1,0,0)).toRotationMatrix();
				break;
			case SDLK_DOWN:
				rotate_acc = AAf((-5.0f / 180) * PI, v3f(1,0,0)).toRotationMatrix();
				break;
			case SDLK_LEFT:
				rotate_acc = AAf((5.0f / 180) * PI, v3f(0,1,0)).toRotationMatrix();
				break;
			case SDLK_RIGHT:
				rotate_acc = AAf((-5.0f / 180) * PI, v3f(0,1,0)).toRotationMatrix();
				break;


			}
		}

		mat4f rotate_acc_4f = mat4f::Identity();
		rotate_acc_4f.block(0, 0, 3, 3) = rotate_acc;

		rotate = rotate_acc_4f * rotate;

		SDL_Delay(1000 / 30); // Running at 30 frame pre second
	}

	SDL_Quit();

	return 0;
}