#if !defined(MATH2D_H)
#define MATH2D_H

#include <cmath>

inline constexpr float Pi = 3.14159265358979323846f;

/////////////////////////////////////STRUCTURES/////////////////////////////////////////
typedef struct POINT2D
{
	float	x, y;
	POINT2D *next;
}*LPPOINT2D;

struct VECTOR2D
{
	float x = 0.0f;
	float y = 0.0f;

	constexpr VECTOR2D() noexcept = default;
	constexpr VECTOR2D(float x_, float y_) noexcept : x(x_), y(y_) {}
	// Angle constructor — produces a unit vector at angle fTheta (radians).
	// Not constexpr until C++26 makes sin/cos constexpr.
	VECTOR2D(float fTheta) noexcept;

	[[nodiscard]] constexpr VECTOR2D operator-() const noexcept { return {-x, -y}; }

	constexpr VECTOR2D& operator+=(VECTOR2D const &A) noexcept { x += A.x; y += A.y; return *this; }
	constexpr VECTOR2D& operator-=(VECTOR2D const &A) noexcept { x -= A.x; y -= A.y; return *this; }
	constexpr VECTOR2D& operator*=(float fM)          noexcept { x *= fM;  y *= fM;  return *this; }
	constexpr VECTOR2D& operator/=(float fD)          noexcept { x /= fD;  y /= fD;  return *this; }
};
using LPVECTOR2D = VECTOR2D*;
using Vec2       = VECTOR2D;  // forward-looking alias; usage may migrate later

class MATRIX
{
public:

	union
	{
		struct
		{
			float e11;
			float e12;
			float e21;
			float e22;
		};

		float e[2][2];
	};

	constexpr MATRIX(float _e11, float _e12, float _e21, float _e22) noexcept
	    : e11(_e11), e12(_e12), e21(_e21), e22(_e22) {}

	MATRIX() noexcept {}

	MATRIX(float angle) noexcept
	{
		float c = std::cos(angle);
		float s = std::sin(angle);

		e11 = c; e12 = s;
		e21 =-s; e22 = c;
	}

	[[nodiscard]] constexpr float  operator()(int i, int j) const noexcept { return e[i][j]; }
	[[nodiscard]] constexpr float& operator()(int i, int j)       noexcept { return e[i][j]; }


	[[nodiscard]] const VECTOR2D& operator[](int i) const noexcept
	{
		return reinterpret_cast<const VECTOR2D&>(e[i][0]);
	}

	[[nodiscard]] VECTOR2D& operator[](int i) noexcept
	{
		return reinterpret_cast<VECTOR2D&>(e[i][0]);
	}

	[[nodiscard]] static MATRIX Identity() noexcept
	{
		return MATRIX(1.0f, 0.0f, 0.0f, 1.0f);
	}

	[[nodiscard]] static MATRIX Zer0() noexcept
	{
		return MATRIX(0.0f, 0.0f, 0.0f, 0.0f);
	}


	[[nodiscard]] MATRIX Tranpose() const noexcept
	{
		MATRIX T;
		T.e11 = e11;
		T.e21 = e12;
		T.e12 = e21;
		T.e22 = e22;
		return T;
	}

	[[nodiscard]] constexpr MATRIX operator * (const MATRIX& M) const noexcept
	{
		return MATRIX(
			e11 * M.e11 + e12 * M.e21,
			e11 * M.e12 + e12 * M.e22,
			e21 * M.e11 + e22 * M.e21,
			e21 * M.e12 + e22 * M.e22);
	}

	[[nodiscard]] constexpr MATRIX operator ^ (const MATRIX& M) const noexcept
	{
		return MATRIX(
			e11 * M.e11 + e12 * M.e12,
			e11 * M.e21 + e12 * M.e22,
			e21 * M.e11 + e22 * M.e12,
			e21 * M.e21 + e22 * M.e22);
	}

	[[nodiscard]] constexpr MATRIX operator * (float s) const noexcept
	{
		return MATRIX(e11 * s, e12 * s, e21 * s, e22 * s);
	}
};

////////////////////////////////////FUNCTIONS////////////////////////////////////////////
[[nodiscard]] constexpr int Sign(float r) noexcept
{
	if (r > 0) return 1;
	if (r < 0) return -1;
	return 0;
}

[[nodiscard]] constexpr float DegToRad(float fDeg) noexcept { return fDeg / 180.0f * Pi; }
[[nodiscard]] constexpr float RadToDeg(float fRad) noexcept { return fRad / Pi * 180.0f; }

[[nodiscard]] float    Orient(POINT2D const &A, POINT2D const &B, POINT2D const &C) noexcept;
[[nodiscard]] float    Distance(VECTOR2D const &A, VECTOR2D const &B, VECTOR2D const &C) noexcept;

[[nodiscard]] constexpr VECTOR2D operator-(VECTOR2D const &A, VECTOR2D const &B) noexcept
{
	return { A.x - B.x, A.y - B.y };
}
[[nodiscard]] constexpr VECTOR2D operator+(VECTOR2D const &A, VECTOR2D const &B) noexcept
{
	return { A.x + B.x, A.y + B.y };
}
[[nodiscard]] constexpr VECTOR2D operator*(VECTOR2D const &A, float B) noexcept
{
	return { A.x * B, A.y * B };
}
[[nodiscard]] constexpr VECTOR2D operator/(VECTOR2D const &A, float B) noexcept
{
	return { A.x / B, A.y / B };
}

[[nodiscard]] VECTOR2D operator*(VECTOR2D const &A, MATRIX const &M) noexcept;
[[nodiscard]] VECTOR2D operator^(VECTOR2D const &A, MATRIX const &M) noexcept;

[[nodiscard]] constexpr float DotProduct(VECTOR2D const &A, VECTOR2D const &B) noexcept
{
	return A.x * B.x + A.y * B.y;
}
[[nodiscard]] constexpr float PerpDotProduct(VECTOR2D const &A, VECTOR2D const &B) noexcept
{
	return A.x * B.y - A.y * B.x;
}

[[nodiscard]] VECTOR2D Projection(VECTOR2D const &A, VECTOR2D const &B) noexcept;

[[nodiscard]] constexpr VECTOR2D Perp(VECTOR2D const &A) noexcept
{
	return { -A.y, A.x };
}
[[nodiscard]] VECTOR2D Normalize(VECTOR2D const &A) noexcept;

[[nodiscard]] float Length(VECTOR2D const &A) noexcept;

void Rotate(LPVECTOR2D A, LPVECTOR2D B, float fAngle) noexcept;

void AddPoint(LPPOINT2D *point, float x, float y);

#endif
