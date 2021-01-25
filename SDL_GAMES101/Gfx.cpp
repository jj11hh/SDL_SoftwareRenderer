#include "Gfx.h"
#include <algorithm>

#define ABS(x) ((x) >= 0 ? (x) : -(x))

using vec2f = Eigen::Vector2f;
using vec3f = Eigen::Vector3f;
using vec4f = Eigen::Vector4f;

static Eigen::Vector4f color_to_v4f(const Gfx::Color color) {
	return { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
}

static Gfx::Color v4f_to_color(const Eigen::Vector4f vec) {
	return {
		static_cast<uint8_t>(vec(0) * 255.0f),
		static_cast<uint8_t>(vec(1) * 255.0f),
		static_cast<uint8_t>(vec(2) * 255.0f),
		static_cast<uint8_t>(vec(3) * 255.0f),
	};
}

template <typename T>
T lerp(const T& v1, const T& v2, float factor) {
	return (1.0f - factor) * v1 + factor * v2;
}

//static Eigen::Vector3f 

void Gfx::drawLine(v2i begin, v2i end, Color color)
{
	int x1, y1, x2, y2, x, y, dx, dy, dx1, dy1, px, py, xe, ye;

	x1 = begin.x(); y1 = begin.y();
	x2 = end.x(); y2 = end.y();

	if (x1 == x2) {
		return fastDrawLineV(y1, y2, x1, color);
	}

	if (y1 == y2) {
		return fastDrawLineH(x1, x2, y1, color);
	}

	dx = x2 - x1;
	dy = y2 - y1;

	dx1 = ABS(dx);
	dy1 = ABS(dy);

	px = 2*dy1 - dx1;
	py = 2*dx1 - dy1;

	if (dy1 <= dx1) {
		if (dx >= 0) {
			x = x1;
			y = y1;
			xe = x2;
		}
		else {
			x = x2;
			y = y2;
			xe = x1;
		}

		drawPixel(x, y, color);

		while (x < xe) {
			++x;

			if (px < 0) {
				px += 2 * dy1;
			}
			else {
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
					++y;
				else
					--y;
				px += 2 * (dy1 - dx1);
			}

			drawPixel(x, y, color);
		}
	}
	else {
		if (dy >= 0) {
			x = x1; y = y1; ye = y2;
		}
		else {
			x = x2; y = y2; ye = y1;
		}

		drawPixel(x, y, color);

		while (y < ye) {
			++y;
			if (py <= 0) {
				py += 2 * dx1;
			}
			else {
				if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
					++x;
				else
					--x;

				py += 2 * (dx1 - dy1);
			}

			drawPixel(x, y, color);
		}
	}
}

void Gfx::strokeTriangle(const Eigen::Vector3f vertices[3], Color color)
{
#define _DRAWLINE(s, e) drawLine(\
	v2i(lroundf(vertices[s].x()), lroundf(vertices[s].y())),\
	v2i(lroundf(vertices[e].x()), lroundf(vertices[e].y())),\
	color\
)
	_DRAWLINE(0, 1);
	_DRAWLINE(0, 2);
	_DRAWLINE(1, 2);
#undef _DRAWLINE
}

void Gfx::fastDrawLineH(int x1, int x2, int y, Color color)
{
	if (x1 > x2) std::swap(x1, x2);
	for (; x1 <= x2; ++x1) {
		drawPixel(x1, y, color);
	}
}

void Gfx::fastDrawLineV(int y1, int y2, int x, Color color)
{
	if (y1 > y2) std::swap(y1, y2);
	for (; y1 <= y2; ++y1) {
		drawPixel(x, y1, color);
	}

}

static void drawFlatTriangle(Gfx* gfx, 
	float y1, float y2, 
	float x1, float x2, float x3, 
	const Gfx::Color colors[3], const float depthes[3]) {

	//                (x2,y2)
	//                  /\
	//              k1 /  \ k2
	//                /    \
	//         (x1,y1)------(x3,y1)

	if (fabs(y1 - y2) < 1.0f) {
		return;
	}

	vec4f colors_vec[] = {
		color_to_v4f(colors[0]),
		color_to_v4f(colors[1]),
		color_to_v4f(colors[2]),
	};

	bool up = y2 > y1;

	if (x1 > x3) {
		std::swap(x1, x3);
		std::swap(colors_vec[0], colors_vec[2]);
	}

	float _1_k1 = (x2 - x1) / (y2 - y1);
	float _1_k2 = (x2 - x3) / (y2 - y1);

	float xs = x1;
	float xe = x3;
	int yi = lroundf(y1);

	vec4f color_yacc_left = (colors_vec[1] - colors_vec[0]) / fabs(y2 - y1);
	vec4f color_yacc_right = (colors_vec[1] - colors_vec[2]) / fabs(y2 - y1);

	vec4f color_left = colors_vec[0];
	vec4f color_right = colors_vec[2];

	float depth_yacc_left = (depthes[1] - depthes[0]) / fabs(y2 - y1);
	float depth_yacc_right = (depthes[1] - depthes[2]) / fabs(y2 - y1);

	float depth_left = depthes[0];
	float depth_right = depthes[2];

	while ((up && yi <= y2) || (!up && yi >= y2)) {
		int xstop = lroundf(xe);

		vec4f color_acc = (color_right - color_left) / (xe - xs);
		vec4f color = color_left;

		float depth_acc = (depth_right - depth_left) / (xe - xs);
		float depth = depth_left;

		for (int i = lroundf(xs); i <= xstop; ++i) {
			float zbuf = gfx->getZBuffer(i, yi);
			if (depth < zbuf) {
				gfx->drawPixel(i, yi, v4f_to_color(color));
				gfx->setZBuffer(i, yi, depth);
			}
			color += color_acc;
		}

		if (up) {
			yi++;
			xs += _1_k1;
			xe += _1_k2;
			color_left += color_yacc_left;
			color_right += color_yacc_right;
		}
		else {
			yi--;
			xs -= _1_k1;
			xe -= _1_k2;
			color_left -= color_yacc_left;
			color_right -= color_yacc_right;
		}
	}
}

static bool isCCW(const Eigen::Vector3f vertices[3]) {
	using v2f = Eigen::Vector2f;
	v2f A(vertices[0].x(), vertices[0].y());
	v2f B(vertices[1].x(), vertices[1].y());
	v2f C(vertices[2].x(), vertices[2].y());

	// if AB x AC > 0, triangle is CCW
	return (B.x() - A.x()) * (C.y() - A.y()) - (B.y() - A.y()) * (C.x() - A.x()) > 0;
}

void Gfx::drawTriangle(const Eigen::Vector3f vertices[3], const Color colors[3])
{
	if (!isCCW(vertices)) {
		// Cull out!
		return;
	}

	//            * sorted[2]
	//            **
	//            * *
	//            *  *
	//   sorted[1]*---* v4 (x4, sorted[1].y())
	//              *  *
	//                * *
	//                  **
	//                    * sorted[0]

	// Sort vertices by y
	Eigen::Vector3f sorted[3] = { vertices[0], vertices[1], vertices[2] };
	Color sortedColors[3] = { colors[0], colors[1], colors[2] };

	if (sorted[1].y() < sorted[0].y()) {
		std::swap(sorted[1], sorted[0]);
		std::swap(sortedColors[1], sortedColors[0]);
	}
	if (sorted[2].y() < sorted[0].y()) {
	
		std::swap(sorted[2], sorted[0]);
		std::swap(sortedColors[2], sortedColors[0]);
	}
	if (sorted[2].y() < sorted[1].y()) {
		std::swap(sorted[1], sorted[2]);
		std::swap(sortedColors[1], sortedColors[2]);
	}

	float dy = sorted[0].y() - sorted[2].y();

	if (fabs(dy) < 1.0f)
		return;

	float dx = sorted[0].x() - sorted[2].x();
	float dz = sorted[0].z() - sorted[2].z();
	float dy1 = sorted[1].y() - sorted[2].y();

	//           alpha        P      beta
	// A ---------------------.------------------ B

	float alpha = dy1 / dy;
	float beta = 1 - alpha;

	float x4 = alpha * dx + sorted[2].x();
	float z4 = alpha * dz + sorted[2].z();

	Color color4 = Color::blend(colors[0], colors[2], alpha);

	Color colors1[] = { colors[1], colors[0], color4 };
	Color colors2[] = { colors[1], colors[2], color4 };

	float depthes1[] = { sorted[1].z(), sorted[0].z(), z4 };
	float depthes2[] = { sorted[1].z(), sorted[2].z(), z4 };

	// Color drawColors[] = { drawColor,drawColor,drawColor, };

	drawFlatTriangle(this, sorted[1].y(), sorted[0].y(), sorted[1].x(), sorted[0].x(), x4, colors1, depthes1);
	drawFlatTriangle(this, sorted[1].y(), sorted[2].y(), sorted[1].x(), sorted[2].x(), x4, colors2, depthes2);

	//drawFlatTriangle(this, sorted[1].y(), sorted[0].y(), sorted[1].x(), sorted[0].x(), x4, drawColors, depthes1);
	//drawFlatTriangle(this, sorted[1].y(), sorted[2].y(), sorted[1].x(), sorted[2].x(), x4, drawColors, depthes1);
}
