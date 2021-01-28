#include "Rasterizer.h"
#include "SoftwareRenderer.h"
#include "IShader.h"
#include <cmath>

#define ABS(x) ((x) >= 0 ? (x) : -(x))

static inline void m_setPixel(uint32_t* surface, int pitch, int w, int h, int x, int y, uint32_t color) {
	uint8_t* target_u8 = reinterpret_cast<uint8_t *>(surface)
		+ static_cast<uint64_t>(y) * pitch
		+ x * sizeof(uint32_t);

	uint32_t* target = reinterpret_cast<uint32_t *>(target_u8);
	*target = color;
}

static inline void m_getPixel(uint32_t* surface, int pitch, int w, int h, int x, int y, uint32_t *color) {
	uint8_t* target_u8 = reinterpret_cast<uint8_t *>(surface)
		+ static_cast<uint64_t>(y) * pitch
		+ x * sizeof(uint32_t);

	*color = *reinterpret_cast<uint32_t *>(target_u8);
}


static bool line_clipping(int Xmin, int Xmax, int Ymin, int Ymax, int& x0, int& y0, int& x1, int& y1) {
	uint8_t code1 = 0;
	uint8_t code2 = 0;

	if (x0 < Xmin)	code1 |= 0b0001;
	if (x0 >= Xmax) code1 |= 0b0010;
	if (y0 < Ymin)	code1 |= 0b0100;
	if (y0 >= Ymax) code1 |= 0b1000;

	if (x1 < Xmin)	code2 |= 0b0001;
	if (x1 >= Xmax) code2 |= 0b0010;
	if (y1 < Ymin)	code2 |= 0b0100;
	if (y1 >= Ymax) code2 |= 0b1000;

	//           |              |
	//   1001    |     1000     |    1010
	//------------------------------------------
	//           |              |
	//   0001    |     0000     |    0010
	//           |              |
	//------------------------------------------
	//   0101    |     0100     |    0110
	//           |              |

	if ((code1 | code2) == 0) return true;
	if ((code1 & code2) != 0) return false;

	if (x0 < Xmin) {
		int dx = x1 - x0;
		float dydx = float(y1 - y0) / dx;
		float newy = (Xmin - x0) * dydx + y0;
		y0 = lroundf(newy);
		x0 = Xmin;
	}
	else if (x1 < Xmin) {
		int dx = x0 - x1;
		float dydx = float(y0 - y1) / dx;
		float newy = (Xmin - x1) * dydx + y1;
		y1 = lroundf(newy);
		x1 = Xmin;
	}

	if (x0 >= Xmax) {
		int dx = x0 - x1;
		float dydx = float(y0 - y1) / dx;
		float newy = (Xmax - 1 - x1) * dydx + y1;
		y0 = lroundf(newy);
		x0 = Xmax - 1;
	}
	else if (x1 >= Xmax) {
		int dx = x1 - x0;
		float dydx = float(y1 - y0) / dx;
		float newy = (Xmax - 1 - x0) * dydx + y0;
		y1 = lroundf(newy);
		x1 = Xmax - 1;
	}

	if ((y0 < Ymin && y1 < Ymin) || (y0 >= Ymax && y1 >= Ymax))
		return false;

	if (y0 < Ymin) {
		int dy = y1 - y0;
		float dxdy = float(x1 - x0) / dy;
		float newx = (Ymin - y0) * dxdy + x0;
		x0 = lroundf(newx);
		y0 = Ymin;
	}
	else if (y1 < Ymin) {
		int dy = y0 - y1;
		float dxdy = float(x0 - x1) / dy;
		float newx = (Ymin - y1) * dxdy + x1;
		x1 = lroundf(newx);
		y1 = Ymin;
	}


	if (y0 >= Ymax) {
		int dy = y0 - y1;
		float dxdy = float(x0 - x1) / dy;
		float newx = (Ymax - 1 - y1) * dxdy + x1;
		x0 = lroundf(newx);
		y0 = Ymax - 1;
	}
	else if (y1 >= Ymax) {
		int dy = y1 - y0;
		float dxdy = float(x1 - x0) / dy;
		float newx = (Ymax - 1 - y0) * dxdy + x0;
		x1 = lroundf(newx);
		y1 = Ymax - 1;
	}

	return true;
}

void Rasterizer::bresenhamDrawLine(uint32_t* surface, int pitch, int w, int h, int x1, int y1, int x2, int y2, uint32_t color)
{
	if (!line_clipping(0, w, 0, h, x1, y1, x2, y2)) return;

	int dx = ABS(x2 - x1), sx = x1 < x2 ? 1 : -1;
	int dy = ABS(y2 - y1), sy = y1 < y2 ? 1 : -1;
	int err = (dx > dy ? dx : - dy) / 2;

	while (1) {
		m_setPixel(surface, pitch, w, h, x1, y1, color);
		if (x1 == x2 && y1 == y2) break;
		
		int e2 = err;
		if (e2 > -dx) { err -= dy; x1 += sx; }
		if (e2 <  dy) { err += dx; y1 += sy; }
	}
}

void Rasterizer::setPixel(uint32_t* surface, int pitch, int w, int h, int x, int y, uint32_t color)
{
	m_setPixel(surface, pitch, w, h, x, y, color);
}

static Eigen::Vector3f barycentricCoordinates(float x, float y, const Eigen::Vector3f triangle[3], Eigen::Vector3f acc[2]) {
	const Eigen::Vector3f& A = triangle[0];
	const Eigen::Vector3f& B = triangle[1];
	const Eigen::Vector3f& C = triangle[2];

	const float dyAB = A.y() - B.y();
	const float dxBA = B.x() - A.x();
	const float crossAB = A.x() * B.y() - B.x() * A.y();
	const float deltaAB = dyAB * C.x() + dxBA * C.y() + crossAB;
	const float gamma = (dyAB * x + dxBA * y + crossAB) / deltaAB;
	acc[0].z() = dyAB / deltaAB;
	acc[1].z() = dxBA / deltaAB;

	const float dyAC = A.y() - C.y();
	const float dxCA = C.x() - A.x();
	const float crossAC = A.x() * C.y() - C.x() * A.y();
	const float deltaAC = dyAC * B.x() + dxCA * B.y() + crossAC;
	const float beta = (dyAC * x + dxCA * y + crossAC) / deltaAC;
	acc[0].y() = dyAC / deltaAC;
	acc[1].y() = dxCA / deltaAC;

	const float dyBC = B.y() - C.y();
	const float dxCB = C.x() - B.x();
	const float crossBC = B.x() * C.y() - C.x() * B.y();
	const float deltaBC = dyBC * A.x() + dxCB * A.y() + crossBC;
	const float alpha = (dyBC * x + dxCB * y + crossBC) / deltaBC;
	acc[0].x() = dyBC / deltaBC;
	acc[1].x() = dxCB / deltaBC;

	return { alpha, beta, gamma };
}

void Rasterizer::drawTriangleSample(SoftwareRenderer *renderer, RenderContext *ctx, Eigen::VectorXf vertices[3])
{

	auto pShader = renderer->pShader;
	auto surface = renderer->frameBuffer;
	auto pitch = renderer->pitch;
	auto w = renderer->w;
	auto h = renderer->h;
	auto zBufferEnabled = renderer->zBufferEnabled;
	auto pcEnabled = renderer->perspectiveCorrectEnabled;

	assert(pShader != nullptr && "shader is null!");

	auto &desc = pShader->getDesc();

	Eigen::Vector3f points[3];

	for (int i = 0; i < 3; ++i) {
		// Do Clip (TODO: NOT IMPLEMENTED)

		// Do Perspective Division
		const float infW = 1 / vertices[i](desc.positionPlacement + 3); // the W

		// Do perspective division on all attributes 
		// only when perspective correction is enabled.
		// Otherwise, division is only be applied on position
		if (pcEnabled)
			vertices[i] *= infW;
		else
			vertices[i].segment<4>(desc.positionPlacement) *= infW;

		vertices[i](desc.positionPlacement + 3) = infW;

		Eigen::Vector4f position = desc.extractPosition(vertices[i]);

		// Scale to viewport
		points[i].x() = (position.x()+1.0f)/2 * w;
		points[i].y() = (position.y()+1.0f)/2 * h;
		points[i].z() = position.z();
	}

	Eigen::Vector2f AB = points[1].segment<2>(0) - points[0].segment<2>(0);
	Eigen::Vector2f AC = points[2].segment<2>(0) - points[0].segment<2>(0);

	const float CrossResult = (AB.x() * AC.y() - AC.x() * AB.y());
	const bool isCCW = CrossResult > 0.0f;

	if ((renderer->backfaceCull && (!isCCW)) || fabsf(CrossResult) < 0.01 ) {
		// cull it out
		return;
	}

	struct {
		int x0; int y0;
		int x1; int y1;
	} aabb = {INT_MAX, INT_MAX, INT_MIN, INT_MIN};

	for (int i = 0; i < 3; ++i) {
		aabb.x0 = std::min(int(points[i].x()), aabb.x0);
		aabb.x1 = std::max(int(points[i].x()) + 1, aabb.x1);
		aabb.y0 = std::min(int(points[i].y()), aabb.y0);
		aabb.y1 = std::max(int(points[i].y()) + 1, aabb.y1);
	}

	aabb.x0 = std::max(aabb.x0, 0);
	aabb.y0 = std::max(aabb.y0, 0);
	aabb.x1 = std::min(aabb.x1, w);
	aabb.y1 = std::min(aabb.y1, h);

	Eigen::Vector3f cooAcc[2];
	auto cooLine = barycentricCoordinates(aabb.x0 + 0.5f, aabb.y0 + 0.5f, points, cooAcc);
	Eigen::VectorXf attrLine = vertices[0] * cooLine.x() + vertices[1] * cooLine.y() + vertices[2] * cooLine.z();

	Eigen::VectorXf attrXAcc = cooAcc[0].x() * vertices[0] + cooAcc[0].y() * vertices[1] + cooAcc[0].z() * vertices[2];
	Eigen::VectorXf attrYAcc = cooAcc[1].x() * vertices[0] + cooAcc[1].y() * vertices[1] + cooAcc[1].z() * vertices[2];

	Eigen::VectorXf fixedAttr = vertices[0];
	Eigen::VectorXf attrPixel = vertices[0];
	Eigen::Vector3f cooPixel;
	Eigen::Vector4f fcolor;

	uint8_t color[4];
	uint8_t density = renderer->sampleDensity;

	for (int y = aabb.y0; y < aabb.y1; ++y) {
		cooPixel = cooLine;
		attrPixel = attrLine;
		for (int x = aabb.x0; x < aabb.x1; ++x) {

			// discard fragment if not in density grid
			if (!density || (x % (density + 1) == 0 && y % (density + 1) == 0)) {
				if (cooPixel.x() > 0 && cooPixel.y() > 0 && cooPixel.z() > 0) {
					const float infW = 1 / attrPixel(desc.positionPlacement + 3);
					bool discard = false;
					ctx->discard = [&] () { discard = true; };
					if (pcEnabled) {
						fixedAttr = attrPixel * infW;
						pShader->fragmentShader(*ctx, fixedAttr, fcolor);
					}
					else {
						pShader->fragmentShader(*ctx, attrPixel, fcolor);
					}

					if (!discard) {
						fcolor(0) = std::min(fcolor(0), 1.0f);
						fcolor(1) = std::min(fcolor(1), 1.0f);
						fcolor(2) = std::min(fcolor(2), 1.0f);
						fcolor(3) = std::min(fcolor(3), 1.0f);
						fcolor *= 255;
						color[0] = fcolor(2);
						color[1] = fcolor(1);
						color[2] = fcolor(0);
						color[3] = fcolor(3);

						float z = attrPixel(desc.positionPlacement + 2);
						if (zBufferEnabled){
							float& oldz = renderer->zBuffer[std::size_t(y) * w + x];
							if (z > oldz) {
								m_setPixel(surface, pitch, w, h, x, h - y - 1, *reinterpret_cast<uint32_t*>(color));
								oldz = z;
							}
						}
						else {
							m_setPixel(surface, pitch, w, h, x, h - y - 1, *reinterpret_cast<uint32_t*>(color));
						}
					}
				}
			}
			cooPixel += cooAcc[0];
			attrPixel += attrXAcc;
		}
		cooLine += cooAcc[1];
		attrLine += attrYAcc;
	}
}

void Rasterizer::drawTriangleWireframe(SoftwareRenderer *renderer, RenderContext *ctx, Eigen::VectorXf vertices[3])
{
	auto pShader = renderer->pShader;
	auto surface = renderer->frameBuffer;
	auto pitch = renderer->pitch;
	auto w = renderer->w;
	auto h = renderer->h;

	assert(pShader != nullptr && "shader is null!");

	auto &desc = pShader->getDesc();

	int points[3][2];

	for (volatile int i = 0; i < 3; ++i) {
		Eigen::Vector4f position = desc.extractPosition(vertices[i]);

		// Do Perspective Division
		position /= position.w();

		// Scale to viewport
		points[i][0] = lroundf((position.x()+1.0f)/2 * w);
		points[i][1] = lroundf((position.y()+1.0f)/2 * h);
	}
	Eigen::Vector2f AB(points[1][0] - points[0][0], points[1][1] - points[0][1]);
	Eigen::Vector2f AC(points[2][0] - points[0][0], points[2][1] - points[0][1]);

	bool isCCW = (AB.x() * AC.y() - AC.x() * AB.y()) > 0;

	if (renderer->backfaceCull && (!isCCW)) {
		// cull it out
		return;
	}

	bresenhamDrawLine(surface, pitch, w, h, points[0][0], h - points[0][1], points[1][0], h - points[1][1], 0xFFFFFFFF);
	bresenhamDrawLine(surface, pitch, w, h, points[1][0], h - points[1][1], points[2][0], h - points[2][1], 0xFFFFFFFF);
	bresenhamDrawLine(surface, pitch, w, h, points[0][0], h - points[0][1], points[2][0], h - points[2][1], 0xFFFFFFFF);
}
