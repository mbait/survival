
#include "math2D.h"

VECTOR2D::VECTOR2D(void) : x(0), y(0)
{
}

VECTOR2D::VECTOR2D(float _x, float _y) : x(_x), y(_y)
{
}

VECTOR2D::VECTOR2D(float fTheta) : x(cos(fTheta)), y(sin(fTheta))
{
}

VECTOR2D VECTOR2D::operator-()
{
	return VECTOR2D(-x, -y);
}

VECTOR2D &VECTOR2D::operator+=(VECTOR2D const &A)
{
	x += A.x;
	y += A.y;
	return *this;
}

VECTOR2D &VECTOR2D::operator-=(VECTOR2D const &A)
{
	x -= A.x;
	y -= A.y;
	return *this;
}

VECTOR2D &VECTOR2D::operator*=(float fM)
{
	x *= fM;
	y *= fM;
	return *this;
}

VECTOR2D &VECTOR2D::operator/=(float fD)
{
	x /= fD;
	y /= fD;
	return *this;
}

float DegToRad(float fDeg)
{
	return fDeg/180*Pi;
}

float RadToDeg(float fRad)
{
	return fRad/Pi*180;
}

float Orient(POINT2D const &A, POINT2D const &B, POINT2D const &C)
{
	return (B.x-A.x)*(B.y-A.y)-(B.x-A.x)*(C.y-A.y);
}

float Distance(VECTOR2D const &A, VECTOR2D const &B, VECTOR2D const &C)
{
	float dx = B.x-A.x;
	float dy = B.y-A.y;
	float cdp = dx*(C.y-A.y)-dy*(C.x-A.x);
	float l = sqrt(dx*dx+dy*dy);
	if(l)
		return cdp/sqrt(dx*dx+dy*dy);
	else
		return 0.0f;
}

VECTOR2D operator-(VECTOR2D const &A, VECTOR2D const &B)
{
	return VECTOR2D(A.x-B.x, A.y-B.y);
}

VECTOR2D operator+(VECTOR2D const &A, VECTOR2D const &B)
{
	return VECTOR2D(A.x+B.x, A.y+B.y);
}

VECTOR2D operator*(VECTOR2D const &A, float B)
{

	return VECTOR2D(A.x*B, A.y*B);
}

VECTOR2D operator*(VECTOR2D const &A, MATRIX const &M)
{
	VECTOR2D T;
	T.x = A.x*M.e11 + A.y*M.e12;
	T.y = A.x*M.e21 + A.y*M.e22;
	return T;
}

VECTOR2D operator^ (VECTOR2D const &A, MATRIX const &M)
{
	VECTOR2D T;
	T.x = A.x * M.e11 + A.y * M.e21;
	T.y = A.x * M.e12 + A.y * M.e22;
	return T;
}

VECTOR2D operator/(VECTOR2D const &A, float B)
{
	return VECTOR2D(A.x/B, A.y/B);
}

float DotProduct(VECTOR2D const &A, VECTOR2D const &B)
{
	return A.x*B.x + A.y*B.y;
}

float PerpDotProduct(VECTOR2D const &A, VECTOR2D const &B)
{
	return A.x*B.y - A.y*B.x;
}

VECTOR2D Projection(VECTOR2D const &A, VECTOR2D const &B)
{
	float l = DotProduct(A, B);
	VECTOR2D n = Normalize(B);

	return (B*l);
}

VECTOR2D Perp(VECTOR2D const &A)
{
	return VECTOR2D(-A.y, A.x);
}

VECTOR2D Normalize(VECTOR2D const &A)
{
	float l = Length(A);
	if(l)
		return VECTOR2D(A.x/l, A.y/l);
	else
		return VECTOR2D(float(0), float(0));
}

float Length(VECTOR2D const &A)
{
	return sqrt(A.x*A.x+A.y*A.y);
}

void Rotate(LPVECTOR2D A, LPVECTOR2D B, float fAngle)
{
	
	float x, y;
	if(B)
	{	
		x = A->x-B->x;
		y = A->y-B->y;
	}
	else
	{
		x = A->x;
		y = A->y;
	}

	float x2 = x*cos(fAngle) - y*sin(fAngle);
	A->y = y*cos(fAngle) + x*sin(fAngle);
	A->x = x2;
	if(B)
	{
		A->x += B->x;
		A->y += B->y;
	}
	
}

void AddPoint(LPPOINT2D *point, float x, float y)
{
	if(!*point)
	{
		*point = new POINT2D;
		(*point)->x = x;
		(*point)->y = y;
		(*point)->next = 0;
	}
	else
	{
		LPPOINT2D p = *point;
		while(p->next)
			p = p->next;
		p->next = new POINT2D;
		p->next->x = x;
		p->next->y = y;
		p->next->next = 0;
		/*LPPOINT2D p = point;
		do
		{
			point = point->next;
		}while(point);
		point = new POINT2D;
		point->x = x;
		point->y = y;
		point->next = 0;
		p->next = point;
		point = p;*/
	}
}


//inline VECTOR2D operator*(MATRIX3x3 const &Mat, VECTOR2D const &A);
