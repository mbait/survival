
#if !defined(PHYSICS2D_H)
#define PHYSICS2D_H

#include <vector>

#include "math2D.h"

float const Epsilon = 1.0E-6f;
float const g = 5.098E+2f;
float const CoR = 0.550f;
float const CoF = 0.08f;
float const CoS = 0.00f;
float const MIN_V = 0.00f;

/////////////////////////////////////STRUCTURES/////////////////////////////////////////
typedef struct RIGIDBODY
{
	RIGIDBODY();
	RIGIDBODY(LPVECTOR2D _lpVertices, int _iNumVertices, bool fStatic = false);

	//~RIGIDBODY();

	void ApplyForce(VECTOR2D const &F);
	void ApplyForce(float F);
	void ApplyImpulse(float fImpulse, VECTOR2D N, VECTOR2D Pdd);
	bool Collide(RIGIDBODY &body, VECTOR2D &MTD, float &t);
	void Move(VECTOR2D const &D);
	void Rotate(float fAngle);
	void ResolveCollision(RIGIDBODY &body, VECTOR2D N, float t);
	bool IsStatic() const {return fMass == 0.0f;}
	void Update(float dt);

	float fMass;
	float fInertia;

    VECTOR2D Pos;
	float fOrientation;
	
	VECTOR2D Velocity;
	float   fAngVelocity;

	VECTOR2D Force;
	float fTorque;
	
	float fRestitution;
	float fFriction;

	unsigned int lMaterialID;

	// Vertex storage owns its memory. Existing call sites that do
	// `body.lpVertices[i]` keep working since std::vector supports the
	// same indexing syntax. iNumVertices is kept for compatibility but
	// is always equal to lpVertices.size().
	std::vector<VECTOR2D> lpVertices;
	int iNumVertices;
}*LPRIGIDBODY;	

typedef struct JOINT
{
	LPRIGIDBODY aBodies[2];
	VECTOR2D aPoints[2];
	
	JOINT();
	JOINT(LPRIGIDBODY body0, LPRIGIDBODY body1, 
		  VECTOR2D point0, VECTOR2D point1);
	void CalcForce();
}*LPJOINT;

bool RayIntersect(RIGIDBODY &body, VECTOR2D raystart, VECTOR2D rayend, float &t, VECTOR2D &Nt);
bool CircleIntersect(RIGIDBODY &body, VECTOR2D center, float fRadius, float &t, VECTOR2D &Nt);

/*typedef struct TERRAIN
{
	TERRAIN() : CoR(0.7f), CoF(0.5f), lpVertices(0)
	{
	}
	
	int iNumVertices;
	LPVECTOR2D lpVertices;

	float CoR;
	float CoF;
}*LPTERRAIN;*/

////////////////////////////////////FUNCTIONS////////////////////////////////////////////

#endif