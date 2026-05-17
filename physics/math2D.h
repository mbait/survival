
#if !defined(MATH2D_H)
#define MATH2D_H

#include <math.h>

const float Pi = 3.14159265358979323846;

/////////////////////////////////////STRUCTURES/////////////////////////////////////////
typedef struct POINT2D
{
	float	x, y;
	POINT2D *next;
}*LPPOINT2D;

typedef struct VECTOR2D
{
	float x, y;

	VECTOR2D(void);
	VECTOR2D(float x, float y);
	VECTOR2D(float fTheta);

	VECTOR2D operator-();
	VECTOR2D &operator+=(VECTOR2D const &A);
	VECTOR2D &operator-=(VECTOR2D const &A);
	VECTOR2D &operator*=(float fM);
	VECTOR2D &operator/=(float fD);
}*LPVECTOR2D;

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

	MATRIX(float _e11, float _e12, float _e21, float _e22)
	: e11(_e11)
	, e12(_e12)
	, e21(_e21)
	, e22(_e22)
	{}
	
	MATRIX()
	{}

	MATRIX(float angle)
	{
		float c = cos(angle);
		float s = sin(angle);

		e11 = c; e12 = s;
		e21 =-s; e22 = c;
	}

	float  operator()(int i, int j) const { return e[i][j]; }
	float& operator()(int i, int j)       { return e[i][j]; }

	
	const VECTOR2D& operator[](int i) const
	{
		return reinterpret_cast<const VECTOR2D&>(e[i][0]);
	}
	
	VECTOR2D& operator[](int i)
	{
		return reinterpret_cast<VECTOR2D&>(e[i][0]);
	}		

	static MATRIX Identity()
	{
		static const MATRIX T(1.0f, 0.0f, 0.0f, 1.0f);

		return T;
	}

	static MATRIX Zer0()
	{
		static const MATRIX T(0.0f, 0.0f, 0.0f, 0.0f);

		return T;
	}


	MATRIX Tranpose() const
	{
		MATRIX T;

		T.e11 = e11;
		T.e21 = e12;
		T.e12 = e21;
		T.e22 = e22;

		return T;
	}

	MATRIX operator * (const MATRIX& M) const 
	{
		MATRIX T;

		T.e11 = e11 * M.e11 + e12 * M.e21;
		T.e21 = e21 * M.e11 + e22 * M.e21;
		T.e12 = e11 * M.e12 + e12 * M.e22;
		T.e22 = e21 * M.e12 + e22 * M.e22;
		
		return T;
	}

	MATRIX operator ^ (const MATRIX& M) const 
	{
		MATRIX T;

		T.e11 = e11 * M.e11 + e12 * M.e12;
		T.e21 = e21 * M.e11 + e22 * M.e12;
		T.e12 = e11 * M.e21 + e12 * M.e22;
		T.e22 = e21 * M.e21 + e22 * M.e22;
		
		return T;
	}

	inline MATRIX operator * ( float s) const
	{
		MATRIX T;

		T.e11 = e11 * s;
		T.e21 = e21 * s;
		T.e12 = e12 * s;
		T.e22 = e22 * s;
		
		return T;
	}
};

////////////////////////////////////FUNCTIONS////////////////////////////////////////////
inline int Sign(float r)
{
	if(r>0)
		return 1;
	else if(r<0)
		return -1;
	else
		 return 0;
}

float DegToRad(float fDeg);
float RadToDeg(float fRad);

float Orient(POINT2D const &A, POINT2D const &B, POINT2D const &C);
float Distance(VECTOR2D const &A, VECTOR2D const &B, VECTOR2D const &C);

VECTOR2D operator-(VECTOR2D const &A, VECTOR2D const &B);
VECTOR2D operator+(VECTOR2D const &A, VECTOR2D const &B);
VECTOR2D operator*(VECTOR2D const &A, float B);
VECTOR2D operator*(VECTOR2D const &A, MATRIX const &M);
VECTOR2D operator^(VECTOR2D const &A, MATRIX const &M);
VECTOR2D operator/(VECTOR2D const &A, float B);

float DotProduct(VECTOR2D const &A, VECTOR2D const &B);
float PerpDotProduct(VECTOR2D const &A, VECTOR2D const &B);

VECTOR2D Projection(VECTOR2D const &A, const VECTOR2D &B);

VECTOR2D Perp(VECTOR2D const &A);
VECTOR2D Normalize(VECTOR2D const &A);

float	Length(VECTOR2D const &A);

void Rotate(LPVECTOR2D A, LPVECTOR2D B, float fAngle);

void AddPoint(LPPOINT2D *point, float x, float y);

#endif