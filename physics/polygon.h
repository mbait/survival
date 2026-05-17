
#if !defined(POLYGON_H)
#define POLYGON_H

#include "math2D.h"

float TriangleSquare(VECTOR2D A, VECTOR2D B, VECTOR2D C)
{
	float a = Length(A-B);
	float b = Length(B-C);
	float c = Length(C-A);
	float p = (a+b+c)/2;

	return sqrt(p*(p-a)*(p-b)*(p-c));
}

VECTOR2D TriangleMassCenter(VECTOR2D A, VECTOR2D B, VECTOR2D C)
{
	float xc = (A.x+B.x+C.x)/3;
	float yc = (A.y+B.y+C.y)/3;
	return VECTOR2D(xc, yc);
}

float ConvexPolygonSquare(LPVECTOR2D lpVertices, int iNumVertices)
{
	if(iNumVertices<3)
		return 0.0f;

	float S = lpVertices[iNumVertices-1].x*lpVertices[0].y - 
			   lpVertices[iNumVertices-1].y*lpVertices[0].x;	
	for(int i=0; i<iNumVertices-1; i++)
		S += lpVertices[i].x*lpVertices[i+1].y;
	for(int i=0; i<iNumVertices-1; i++)
		S -= lpVertices[i].y*lpVertices[i+1].x;

	return 0.5f*fabs(S);
}

VECTOR2D ConvexPolygonMassCenter(LPVECTOR2D lpVertices, int iNumVertices)
{
	if(iNumVertices == 1)
		return lpVertices[0];
	else if(iNumVertices == 2)
		return lpVertices[0]+(lpVertices[1]-lpVertices[0])/2;
	else if(iNumVertices == 3)
		return TriangleMassCenter(lpVertices[0], 
								  lpVertices[1], 
								  lpVertices[2]);
	else
	{
		int cnt = iNumVertices-2;
		VECTOR2D *mc = new VECTOR2D[cnt]; 
		float *s = new float[cnt];
		for(int i=2; i<cnt+1; i++)
		{
			mc[i-2] = TriangleMassCenter(lpVertices[0], lpVertices[i-1], lpVertices[i]);
			s[i-2] = TriangleSquare(lpVertices[0], lpVertices[i-1], lpVertices[i]);
		}
		mc[cnt-1] = TriangleMassCenter(lpVertices[0], lpVertices[cnt], lpVertices[cnt+1]);
		s[cnt-1] = TriangleSquare(lpVertices[0], lpVertices[cnt], lpVertices[cnt+1]);

		float xc = 0, yc = 0;
		float S = ConvexPolygonSquare(lpVertices, iNumVertices);
		for(int i=0; i<cnt; i++)
		{
			xc += mc[i].x*s[i]/S;
			yc += mc[i].y*s[i]/S;
		}

		delete [] mc;
		delete [] s;
		
		return VECTOR2D(xc, yc);
	}
}

#endif