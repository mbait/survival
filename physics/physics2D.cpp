
#include "physics2D.h"

#include <cmath>

#include "compat/win32_compat.h"

#include "polygon.h"

void swap(float& a, float& b) noexcept
{
	float tmp = a;
	a = b;
	b = tmp;
}

//////////////////////////////////IMPLEMENTATION/////////////////////////////////////////
// Members of type VECTOR2D (Pos, Velocity, Force) are zero-initialised
// implicitly by VECTOR2D's own default ctor before this body runs — the
// original code had local variables of the same names that did nothing
// (no_op assignments that shadowed the members) plus a stray MATRIX
// mOrientation that never matched any member at all. All removed; the
// observable state of a default-constructed RIGIDBODY is unchanged.
RIGIDBODY::RIGIDBODY()
{
	fMass = 0.0f;
	fInertia = 0.0f;
	fOrientation = 0.0f;
	fAngVelocity = 0.0f;
	fTorque = 0.0f;
	fRestitution = 1.0f;
	fFriction = 0.0f;
	iNumVertices = 0;
	lMaterialID = 0;
	// lpVertices is a std::vector — its default ctor already produces empty.
}

RIGIDBODY::RIGIDBODY(LPVECTOR2D _lpVertices, int _iNumVertices, bool fStatic) : RIGIDBODY()
{
	iNumVertices = _iNumVertices;
	lpVertices.assign(_lpVertices, _lpVertices + _iNumVertices);

	// calculate mass center
	Pos = ConvexPolygonMassCenter(_lpVertices, _iNumVertices);

	float maxr = 0;
	for (int i = 0; i < iNumVertices; i++)
	{
		const float rlen = Length(_lpVertices[i] - Pos);
		if (rlen > maxr)
		{
			maxr = rlen;
		}
	}

	// calculate mass and inertia
	if (!fStatic)
	{
		fMass = maxr * maxr;
		fInertia = 0.5f * fMass * fMass;
	}
	else
	{
		fMass = 0.0f;
		fInertia = 0.0f;
	}
}

void RIGIDBODY::ApplyForce(VECTOR2D const& F)
{
	if (IsStatic())
		return;

	Force += F;
}

void RIGIDBODY::ApplyForce(float F)
{
	if (IsStatic())
		return;

	fTorque += F;
}

void RIGIDBODY::Update(float dt)
{
	if (IsStatic())
	{
		Velocity = VECTOR2D(0.0f, 0.0f);
		fOrientation = 0.0f;
		return;
	}

	// Update position
	Pos += Velocity * dt;
	fOrientation += fAngVelocity * dt;
	for (int i = 0; i < iNumVertices; i++)
	{
		lpVertices[i] += Velocity * dt;
		::Rotate(&lpVertices[i], &Pos, fAngVelocity * dt);
	}

	// Update velocity
	Velocity += Force * dt / fMass;
	fAngVelocity += fTorque * dt / fInertia;

	if (std::fabs(fAngVelocity) < MIN_V)
		fAngVelocity = 0.0f;

	// Release forces
	Force = VECTOR2D(0.0f, 0.0f);
	fTorque = 0.0f;
}

void CalcInterval(VECTOR2D Axis, RIGIDBODY& body, float& min, float& max)
{
	min = max = DotProduct(body.lpVertices[0] - body.Pos, Axis);
	for (int i = 0; i < body.iNumVertices; i++)
	{
		VECTOR2D V = body.lpVertices[i] - body.Pos;
		float dp = DotProduct(V, Axis);
		if (dp < min)
			min = dp;
		else if (dp > max)
			max = dp;
	}
}

float CalcSeparateAxis(VECTOR2D Axis, RIGIDBODY& A, RIGIDBODY& B)
{
	float fOffset = DotProduct(B.Pos - A.Pos, Axis);
	float mina, minb, maxa, maxb;

	CalcInterval(Axis, A, mina, maxa);
	CalcInterval(Axis, B, minb, maxb);
	minb += fOffset;
	maxb += fOffset;

	float d0 = maxb - mina;
	float d1 = maxa - minb;

	if (d0 <= 0.0f || d1 <= 0.0f)
		return -1;
	else
		return (d0 > d1 ? d1 : d0);
}

bool RIGIDBODY::Collide(RIGIDBODY& body, VECTOR2D& MTD, float& t)
{
	/*if(IsStatic() && body.IsStatic())
	    return false;*/

	// quick rejection
	VECTOR2D v_mina(_HUGE, _HUGE), v_maxa(-_HUGE, -_HUGE), v_minb(_HUGE, _HUGE),
	    v_maxb(-_HUGE, -_HUGE);

	// build A BB
	for (int i = 0; i < iNumVertices; i++)
	{
		if (lpVertices[i].x < v_mina.x)
			v_mina.x = lpVertices[i].x;
		else if (lpVertices[i].x > v_maxa.x)
			v_maxa.x = lpVertices[i].x;

		if (lpVertices[i].y < v_mina.y)
			v_mina.y = lpVertices[i].y;
		else if (lpVertices[i].y > v_maxa.y)
			v_maxa.y = lpVertices[i].y;
	}

	// build B BB
	for (int i = 0; i < body.iNumVertices; i++)
	{
		if (body.lpVertices[i].x < v_minb.x)
			v_minb.x = body.lpVertices[i].x;
		else if (body.lpVertices[i].x > v_maxb.x)
			v_maxb.x = body.lpVertices[i].x;

		if (body.lpVertices[i].y < v_minb.y)
			v_minb.y = body.lpVertices[i].y;
		else if (body.lpVertices[i].y > v_maxb.y)
			v_maxb.y = body.lpVertices[i].y;
	}

	bool horz_interval = (v_minb.x > v_maxa.x) || (v_mina.x > v_maxb.x);
	bool vert_interval = (v_minb.y > v_maxa.y) || (v_mina.y > v_maxb.y);

	if (horz_interval && vert_interval)
		return false;

	float minpd = _HUGE;

	/*if(fSVel>1.0E-4f)
	VECTOR2D Vel = Velocity-body.Velocity;
	{
	    VECTOR2D N = Normalize(Perp(Vel));
	    float pd = CalcSeparateAxis(N, *this, body);
	    if(pd<0)
	        return false;

	    MTD = N;
	    minpd = pd;
	}*/

	// test self axis
	for (int i = 0, j = iNumVertices - 1; i < iNumVertices; j = i, i++)
	{
		VECTOR2D N = Normalize(Perp(lpVertices[i] - lpVertices[j]));
		float pd = CalcSeparateAxis(N, *this, body);
		if (pd < -0.5f)
			return false;
		else if (pd < minpd)
		{
			MTD = N;
			minpd = pd;
		}
	}

	// test body axis
	for (int i = 0, j = body.iNumVertices - 1; i < body.iNumVertices; j = i, i++)
	{
		VECTOR2D N = Normalize(Perp(body.lpVertices[i] - body.lpVertices[j]));
		float pd = CalcSeparateAxis(N, *this, body);
		if (pd < -0.5f)
			return false;
		else if (pd < minpd)
		{
			MTD = N;
			minpd = pd;
		}
	}

	/*if(minpd<=Epsilon)
	    minpd = 1.0f;*/
	minpd *= 0.9f;

	if (DotProduct(MTD, body.Pos - Pos) < 0)
	{
		MTD.x = -MTD.x;
		MTD.y = -MTD.y;
	}
	MTD.x = -MTD.x;
	MTD.y = -MTD.y;
	t = minpd;

	return true;
}

bool RayIntersect(RIGIDBODY& body, VECTOR2D raystart, VECTOR2D rayend, float& t, VECTOR2D& Nt)
{
	VECTOR2D N;
	float tnear = _HUGE;
	bool bCross = false;

	int iNumVertices = body.iNumVertices;
	for (int j = iNumVertices - 1, i = 0; i < iNumVertices; j = i, i++)
	{
		VECTOR2D A = body.lpVertices[j];
		VECTOR2D B = body.lpVertices[i];

		// x11, x12, y11, y12 - ray
		// x21, x22, y21, y22 - segment
		float x11 = raystart.x;
		float x12 = rayend.x;
		float y11 = raystart.y;
		float y12 = rayend.y;

		float x21 = A.x;
		float x22 = B.x;
		float y21 = A.y;
		float y22 = B.y;

		// x
		float ax = x12 - x11;
		float bx = x21 - x22;
		float cx = x21 - x11;
		// y
		float ay = y12 - y11;
		float by = y21 - y22;
		float cy = y21 - y11;

		float t1;
		float t2;
		float d = (by * ax - bx * ay);
		if (d)
		{
			t2 = (cy * ax - cx * ay) / d;
			if (ax)
				t1 = (cx - t2 * bx) / ax;
			else
				t1 = (cy - t2 * by) / ay;
			if ((t1 >= 0) && (t2 >= 0.0f) && (t2 <= 1.0f))
				if (t1 < tnear)
				{
					tnear = t1;
					N = Perp(A - B);
					bCross = true;
				}
		}

		/*VECTOR2D E0 = body.lpVertices[j];
	VECTOR2D E1 = body.lpVertices[i];
	VECTOR2D E  = E1-E0;


	//VECTOR2D En = VECTOR2D(E.y, -E.x);
	VECTOR2D En = Normalize(E);
	VECTOR2D D  = E0-raystart+body.Pos;

	float denom = DotProduct(D, En);
	float numer = DotProduct(raydir, En);

	if (fabs(numer)<Epsilon)
	{
		// origin outside the plane, no intersection
		if (denom < 0.0f)
		    return false;
	}
	else
	{
		float tclip = denom / numer;

		// near intersection
		if (numer < 0.0f)
		{
		    if (tclip > tfar)
		        return false;

		    if (tclip > tnear)
		        tnear = tclip;
		}
		// far intersection
		else
		{
		    if (tclip < tnear)
		        return false;

		    if (tclip < tfar)
		        tfar = tclip;
		}
	}
	*/
	}

	t = tnear;
	Nt = N;
	return bCross;
}

bool CircleIntersect(RIGIDBODY& body, VECTOR2D center, float fRadius, float& t, VECTOR2D& Nt)
{
	VECTOR2D Axis = Normalize(center - body.Pos);

	RIGIDBODY circle;
	circle.Pos = center;
	circle.lpVertices = {center - Axis * fRadius, center + Axis * fRadius};
	circle.iNumVertices = 2;

	VECTOR2D N;
	float minpd = _HUGE;

	float pd = _HUGE;
	pd = CalcSeparateAxis(Axis, body, circle);
	if (pd < -0.5f)
		return false;
	else
	{
		minpd = pd;
		N = Axis;
	}
	for (int i = 0, j = body.iNumVertices - 1; i < body.iNumVertices; j = i, i++)
	{
		Axis = Normalize(Perp(body.lpVertices[i] - body.lpVertices[j]));
		circle.lpVertices[0] = center + Axis * fRadius;
		circle.lpVertices[1] = center - Axis * fRadius;
		pd = CalcSeparateAxis(Axis, body, circle);
		if (pd < -0.5f)
			return false;
		else if (pd < minpd)
		{
			minpd = pd;
			N = Axis;
		}
	}

	if (DotProduct(N, body.Pos - center) < 0)
	{
		N.x = -N.x;
		N.y = -N.y;
	}
	// N.x = -N.x;
	// N.y = -N.y;

	t = minpd;
	Nt = N;

	return true;
}

int FindContactPoints(LPVECTOR2D lpVertices, RIGIDBODY const& body, VECTOR2D N, float t)
{
	/*float min;
	float *d = new float[body.iNumVertices];
	 min = d[0] = DotProduct(body.lpVertices[0]-body.Pos, N);

	for(int i=1; i<body.iNumVertices; i++)
	{
	    d[i] = DotProduct(body.lpVertices[i]-body.Pos, N);
	    if(d[i]<min)
	        min = d[i];
	}

	int iNum = 0;
	const float eps = 1.0E-3f;
	float s[2] = 0.0f
	float sign = false;

	VECTOR2D P = Perp(N);

	for(int i=0; i<body.iNumVertices; i++)
	{
	    if(d[i]<min+eps)
	    {

	    }
	}*/
	float min = DotProduct(body.lpVertices[0] - body.Pos, N);
	lpVertices[0] = body.lpVertices[0];

	std::vector<float> d(body.iNumVertices);
	d[0] = min;

	int mini = 0;
	for (int i = 1; i < body.iNumVertices; i++)
	{
		float dp = DotProduct(body.lpVertices[i] - body.Pos, N);
		if (dp < min)
		{
			lpVertices[0] = body.lpVertices[i];
			min = dp;
			mini = i;
		}
		d[i] = dp;
	}

	int pNum = 1;
	float min2 = min;
	for (int i = 0; i < body.iNumVertices; i++)
	{
		if (mini != i && d[i] < min + Epsilon)
		{
			lpVertices[1] = body.lpVertices[i];
			min2 = d[i];
			pNum = 2;
		}
		else if ((pNum == 2) && (d[i] < min2))
		{
			lpVertices[1] = body.lpVertices[i];
			min2 = d[i];
		}
	}

	return pNum;
}

VECTOR2D ProjectPoint(VECTOR2D const& P, VECTOR2D const& A, VECTOR2D const& B)
{
	VECTOR2D S = B - A;
	VECTOR2D Sp = Normalize(Perp(S));
	float d = Distance(A, B, P);

	return P - Sp * d;
}

float SolveContact(RIGIDBODY& A, RIGIDBODY& B, VECTOR2D N, VECTOR2D P, float t)
{
	float AntiMa, AntiMb;
	float AntiIa, AntiIb;
	if (!A.IsStatic() && !B.IsStatic())
	{
		AntiMa = 1 / A.fMass;
		AntiMb = 1 / B.fMass;
		AntiIa = 1 / A.fInertia;
		AntiIb = 1 / B.fInertia;
	}
	else if (!A.IsStatic() && B.IsStatic())
	{
		AntiMa = 1 / A.fMass;
		AntiMb = 0.0f;
		AntiIa = 1 / A.fInertia;
		AntiIb = 0.0f;
	}
	else
	{
		AntiMa = 0.0f;
		AntiMb = 1 / B.fMass;
		AntiIa = 0.0f;
		AntiIb = 1 / B.fInertia;
	}

	VECTOR2D Ap = P - A.Pos;
	VECTOR2D Bp = P - B.Pos;
	VECTOR2D Vap = A.Velocity + Perp(Ap) * A.fAngVelocity;
	VECTOR2D Vbp = B.Velocity + Perp(Bp) * B.fAngVelocity;
	VECTOR2D Vab = Vap - Vbp;
	VECTOR2D Normal = N;

	float fRest = (A.fRestitution + B.fRestitution) * 0.5f;
	float Num = -(1 + fRest) * DotProduct(Vab, Normal);
	float pda = PerpDotProduct(Ap, N);
	float pdb = PerpDotProduct(Bp, N);
	float Denom =
	    DotProduct(Normal, Normal) * (AntiMa + AntiMb) + pda * pda * AntiIa + pdb * pdb * AntiIb;
	//----!!!!!!!----//
	float fImpulse;
	if (Denom)
		fImpulse = Num / Denom;
	else
		fImpulse = 0.0f;
	return fImpulse;
	// A.ApplyImpulse( fImpulse, Normal, P, Vn);
	// B.ApplyImpulse(-fImpulse, Normal, P, Vn);
}

void RIGIDBODY::ResolveCollision(RIGIDBODY& body, VECTOR2D N, float t)
{
	if (IsStatic() && body.IsStatic())
		return;

	// calculate contact points
	VECTOR2D C[2][2];

	VECTOR2D negN(-N.x, -N.y);
	int iNum0 = FindContactPoints(C[0], *this, N, t);
	int iNum1 = FindContactPoints(C[1], body, negN, t);
	int cNum;

	if (iNum0 == 1 && iNum1 == 1)
	{
		C[0][1] = C[1][0];
		cNum = 1;
	}
	else if (iNum0 == 1 && iNum1 == 2)
	{
		C[0][1] = ProjectPoint(C[0][0], C[1][0], C[1][1]);
		cNum = 1;
	}
	else if (iNum0 == 2 && iNum1 == 1)
	{
		C[0][0] = ProjectPoint(C[1][0], C[0][0], C[0][1]);
		C[0][1] = C[1][0];
		cNum = 1;
	}
	else
	{
		VECTOR2D perp = Perp(Normalize(N));

		float min0 = DotProduct(C[0][0], perp);
		float max0 = DotProduct(C[0][1], perp);
		if (min0 > max0)
		{
			float ftmp = min0;
			min0 = max0;
			max0 = ftmp;

			VECTOR2D tmp = C[0][0];
			C[0][0] = C[0][1];
			C[0][1] = tmp;
		}
		float min1 = DotProduct(C[1][0], perp);
		float max1 = DotProduct(C[1][1], perp);
		if (min1 > max1)
		{
			float ftmp = min1;
			min1 = max1;
			max1 = ftmp;

			VECTOR2D tmp = C[1][0];
			C[1][0] = C[1][1];
			C[1][1] = tmp;
		}

		VECTOR2D C2[2][2];
		C2[0][0] = C[0][0];
		C2[0][1] = C[0][1];
		C2[1][0] = C[1][0];
		C2[1][1] = C[1][1];
		if (min0 < min1)
		{
			C2[0][1] = ProjectPoint(C[1][0], C[0][0], C[0][1]);
			C2[0][0] = C[1][0];
		}
		else
		{
			C2[0][1] = ProjectPoint(C[0][0], C[1][0], C[1][1]);
			C2[0][0] = C[0][0];
		}
		if (max0 < max1)
		{
			C2[1][1] = ProjectPoint(C[0][1], C[1][0], C[1][1]);
			C2[1][0] = C[0][1];
		}
		else
		{
			C2[1][0] = ProjectPoint(C[1][1], C[0][0], C[0][1]);
			C2[1][1] = C[1][1];
		}

		C[0][0] = C2[0][0];
		C[0][1] = C2[0][1];
		C[1][0] = C2[1][0];
		C[1][1] = C2[1][1];

		cNum = 2;
	}

	// calculate contact impulse

	VECTOR2D CP[2];
	if (!IsStatic() && !body.IsStatic())
	{
		Move(N * t);
		body.Move(negN * t);
		for (int i = 0; i < cNum; i++)
			CP[i] = C[i][0] + (C[i][1] - C[i][0]) / 2;
	}
	else if (!IsStatic() && body.IsStatic())
	{
		Move(N * t);
		for (int i = 0; i < cNum; i++)
			CP[i] = C[i][0] + N;
	}
	else
	{
		body.Move(negN * t);
		for (int i = 0; i < cNum; i++)
			CP[i] = C[i][1] + negN;
	}

	float fImpulse = SolveContact(*this, body, N, CP[0], t);
	ApplyImpulse(fImpulse, Normalize(N), CP[0]);
	body.ApplyImpulse(-fImpulse, Normalize(N), CP[0]);
	if (cNum == 2)
	{
		fImpulse = SolveContact(*this, body, N, CP[1], t);
		ApplyImpulse(fImpulse, Normalize(N), CP[1]);
		body.ApplyImpulse(-fImpulse, Normalize(N), CP[1]);
	}
	// apply friction
	/*VECTOR2D F(-Velocity.x, -Velocity.y);
	ApplyForce(F*CoF*fMass);
	F.x = -body.Velocity.x;
	F.y = -body.Velocity.y;
	body.ApplyForce(F*CoF*body.fMass);
	*/
	VECTOR2D D = Perp(Normalize(N));
	float dp = DotProduct(Velocity, D);
	VECTOR2D NV = D * dp;
	VECTOR2D T = Velocity - NV;
	Velocity = T + NV * (1 - body.fFriction);

	// second body
	D = Perp(Normalize(negN));
	dp = DotProduct(body.Velocity, D);
	NV = D * dp;
	T = body.Velocity - NV;
	body.Velocity = T + NV * (1 - fFriction);
	// Contact = CP[0];
}

void RIGIDBODY::ApplyImpulse(float fImpulse, VECTOR2D N, VECTOR2D P)
{
	if (IsStatic())
		return;

	VECTOR2D R = P - Pos;
	Velocity += N * fImpulse / fMass;
	fAngVelocity += PerpDotProduct(R, N) * fImpulse / fInertia;
}

void RIGIDBODY::Move(VECTOR2D const& D)
{
	Pos += D;
	for (int i = 0; i < iNumVertices; i++)
		lpVertices[i] += D;
}

void RIGIDBODY::Rotate(float fAngle)
{
	fOrientation += fAngle;
	for (int i = 0; i < iNumVertices; i++)
		::Rotate(&lpVertices[i], &Pos, fAngle);
}

JOINT::JOINT()
{
	aBodies[0] = nullptr;
	aBodies[1] = nullptr;
	aPoints[0] = VECTOR2D();
	aPoints[1] = VECTOR2D();
}

// Delegating ctor — the original C++98 code used `JOINT();` as a statement
// here, which creates and immediately discards a temporary instead of
// initialising *this. The four assignments below were doing the real work
// anyway; the temporary was just noise. C++11+ delegating syntax makes
// the intent explicit.
JOINT::JOINT(LPRIGIDBODY body0, LPRIGIDBODY body1, VECTOR2D point0, VECTOR2D point1) : JOINT()
{
	aBodies[0] = body0;
	aBodies[1] = body1;
	aPoints[0] = point0;
	aPoints[1] = point1;
}

void JOINT::CalcForce()
{
	bool bStatic0 = aBodies[0]->IsStatic();
	bool bStatic1 = aBodies[1]->IsStatic();

	if (bStatic0 && bStatic1)
		return;

	VECTOR2D P0 = aPoints[0];
	VECTOR2D P1 = aPoints[1];
	Rotate(&P0, nullptr, aBodies[0]->fOrientation);
	Rotate(&P1, nullptr, aBodies[1]->fOrientation);

	VECTOR2D V0 = aBodies[0]->Velocity + Perp(P0) * aBodies[0]->fAngVelocity;
	VECTOR2D V1 = aBodies[1]->Velocity + Perp(P1) * aBodies[1]->fAngVelocity;
	VECTOR2D Vab = V1 - V0;

	VECTOR2D spring_vec = aBodies[1]->Pos - aBodies[0]->Pos + P1 - P0;

	float fKSpring = 100;
	float fKDamping = 30;

	VECTOR2D spring_force = -(spring_vec * fKSpring);

	// aBodies[0]->Force -= spring_force;
	// aBodies[1]->Force += spring_force;
	float numer = DotProduct(Vab, spring_vec);
	float denom = DotProduct(spring_vec, spring_vec);
	VECTOR2D Damping = spring_vec * (-fKDamping * numer / denom);

	spring_force += Damping;

	aBodies[0]->Force -= spring_force;
	aBodies[0]->fTorque -= PerpDotProduct(P0, spring_force);
	aBodies[1]->Force += spring_force;
	aBodies[1]->fTorque += PerpDotProduct(P1, spring_force);

	/*VECTOR2D P0 = aPoints[0];
	VECTOR2D P1 = aPoints[1];
	Rotate(&P0, 0, aBodies[0]->fOrientation);
	Rotate(&P1, 0, aBodies[1]->fOrientation);

	VECTOR2D V0 = aBodies[0]->Velocity+
	    Perp(P0)*aBodies[0]->fAngVelocity;
	VECTOR2D V1 = aBodies[1]->Velocity+
	    Perp(P1)*aBodies[1]->fAngVelocity;
	VECTOR2D Vab = V1-V0;

	VECTOR2D spring  = aBodies[1]->Pos-aBodies[0]->Pos
	    +P1-P0;
	VECTOR2D N	= Normalize(spring);
	VECTOR2D P  = aBodies[0]->Pos+
	    P0+spring*0.5f;

	if(!bStatic0 && !bStatic1)
	{
	    aBodies[0]->Move(spring*0.5f);
	    aBodies[1]->Move(-spring*0.5f);
	}
	else if(!bStatic0 && bStatic1)
	    aBodies[0]->Move(spring);
	else
	    aBodies[1]->Move(spring);

	float fImpulse = SolveContact(*aBodies[0],
	    *aBodies[1], N, P, 0);
	//float fImpulse =
	//	DotProduct(spring, spring);
	aBodies[0]->ApplyImpulse(fImpulse, N, P);
	aBodies[1]->ApplyImpulse(-fImpulse, N, P);
	*/
}