#pragma once

#include <SDL/SDL.h>
#include <eigen3/Eigen/Eigen>
#include <memory>
#include <limits>

class Gfx
{
private:
	SDL_Surface* surface;
	std::unique_ptr<float[]> z_buffer;
	int w;
	int h;

public:
	Gfx(SDL_Surface* surface) : surface(surface) {
		z_buffer = std::make_unique<float[]>((std::size_t)surface->w * surface->h);
		w = surface->w;
		h = surface->h;
		clearZBuffer();
	};
	struct Color {
		Color() : r(0), g(0), b(0), a(0) {};
		Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r(r), g(g), b(b), a(a) {};
		Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b), a(255) {};
		Color operator+ (const Color& c) {
			return Color(r + c.r, g + c.g, b + c.b, a + c.a);
		}
		Color operator- (const Color& c) {
			return Color(r - c.r, g - c.g, b - c.b, a - c.a);
		}

		static Color blend(const Color& c1, const Color& c2, const float alpha) {
			float beta = 1 - alpha;
			uint8_t r = static_cast<uint8_t>(alpha * c1.r + beta * c2.r);
			uint8_t g = static_cast<uint8_t>(alpha * c1.g + beta * c2.g);
			uint8_t b = static_cast<uint8_t>(alpha * c1.b + beta * c2.b);
			uint8_t a = static_cast<uint8_t>(alpha * c1.a + beta * c2.a);

			return { r, g, b, a };
		}

		union {
			struct {
				uint8_t b;
				uint8_t g;
				uint8_t r;
				uint8_t a;
			};
			uint32_t pixel;
		};
	};

	using v2i = Eigen::Vector2i;
	void fastDrawLineV(int y1, int y2, int x, Color color);
	void fastDrawLineH(int x1, int x2, int y, Color color);
	void drawLine(v2i begin, v2i end, Color color);
	void strokeTriangle(const Eigen::Vector3f vertices[3], Color color);
	//void drawTriangle(v2i points[3], Color fillcolor);
	void drawTriangle(const Eigen::Vector3f vertices[3], const Color colors[3]);
	inline void drawPixel_Unsafe(int x, int y, Color color) {
		uint8_t* target_u8 = reinterpret_cast<uint8_t *>(surface->pixels)
			+ static_cast<uint64_t>(y) * surface->pitch
			+ x * sizeof(uint32_t);

		uint32_t* target = reinterpret_cast<uint32_t *>(target_u8);
		*target = color.pixel;
	}
	inline void setZBuffer_Unsafe(int x, int y, float depth) {
		z_buffer[w * y + x] = depth;
	}
	inline void setZBuffer(int x, int y, float depth) {
		if (x < 0 || x >= w || y < 0 || y >= h)
			return;
		setZBuffer_Unsafe(x, y, depth);
	}
	inline float getZBuffer_Unsafe(int x, int y) {
		return z_buffer[w * y + x];
	}
	inline float getZBuffer(int x, int y) {
		if (x < 0 || x >= w || y < 0 || y >= h)
			return std::numeric_limits<float>::infinity();

		return getZBuffer_Unsafe(x, y);
	}
	inline void drawPixel(int x, int y, Color color) {
		if (x < 0 || x >= w || y < 0 || y >= h)
			return;
		drawPixel_Unsafe(x, y, color);
	}
	inline void flip() {
		SDL_Flip(surface);
	}
	inline void clearZBuffer() {
		std::size_t size = (std::size_t)surface->w * surface->h;

		for (std::size_t i = 0; i < size; ++i) {
			// Set Z Buffer to Pos Infinity
			z_buffer[i] = std::numeric_limits<float>::infinity();
		}
	}
};

