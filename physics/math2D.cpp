#include "math2D.h"

// Trigonometric ctor — sin/cos are not constexpr until C++26, so this
// one stays out-of-line. All other simple ops are constexpr in the header.
VECTOR2D::VECTOR2D(float fTheta) noexcept
    : x(std::cos(fTheta))
    , y(std::sin(fTheta))
{
}

float Orient(POINT2D const &A, POINT2D const &B, POINT2D const &C) noexcept
{
	return (B.x - A.x) * (B.y - A.y) - (B.x - A.x) * (C.y - A.y);
}

float Distance(VECTOR2D const &A, VECTOR2D const &B, VECTOR2D const &C) noexcept
{
	const float dx  = B.x - A.x;
	const float dy  = B.y - A.y;
	const float cdp = dx * (C.y - A.y) - dy * (C.x - A.x);
	const float l   = std::sqrt(dx * dx + dy * dy);
	return l ? cdp / l : 0.0f;
}

VECTOR2D operator*(VECTOR2D const &A, MATRIX const &M) noexcept
{
	return { A.x * M.e11 + A.y * M.e12,
	         A.x * M.e21 + A.y * M.e22 };
}

VECTOR2D operator^(VECTOR2D const &A, MATRIX const &M) noexcept
{
	return { A.x * M.e11 + A.y * M.e21,
	         A.x * M.e12 + A.y * M.e22 };
}

VECTOR2D Projection(VECTOR2D const &A, VECTOR2D const &B) noexcept
{
	// Matches the legacy impl (which computed a scalar `l` but only ever
	// returned B * DotProduct(A, B) without using it). Kept as-is to
	// preserve behaviour.
	return B * DotProduct(A, B);
}

VECTOR2D Normalize(VECTOR2D const &A) noexcept
{
	const float l = Length(A);
	if (!l) return {};
	return { A.x / l, A.y / l };
}

float Length(VECTOR2D const &A) noexcept
{
	return std::sqrt(A.x * A.x + A.y * A.y);
}

void Rotate(LPVECTOR2D A, LPVECTOR2D B, float fAngle) noexcept
{
	float x, y;
	if (B) {
		x = A->x - B->x;
		y = A->y - B->y;
	} else {
		x = A->x;
		y = A->y;
	}

	const float c  = std::cos(fAngle);
	const float s  = std::sin(fAngle);
	const float x2 = x * c - y * s;
	A->y           = y * c + x * s;
	A->x           = x2;
	if (B) {
		A->x += B->x;
		A->y += B->y;
	}
}

void AddPoint(LPPOINT2D *point, float x, float y)
{
	if (!*point) {
		*point = new POINT2D;
		(*point)->x    = x;
		(*point)->y    = y;
		(*point)->next = nullptr;
		return;
	}
	LPPOINT2D p = *point;
	while (p->next) p = p->next;
	p->next       = new POINT2D;
	p->next->x    = x;
	p->next->y    = y;
	p->next->next = nullptr;
}
