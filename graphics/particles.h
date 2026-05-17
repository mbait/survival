
#ifndef PARTICLES_H
#define PARTICLES_H

#include <windows.h>
#include "../physics/math2D.h"

struct COLOR
{
	byte Alpha;
	byte R;
	byte G;
	byte B;

	COLOR() {memset(this, 0, sizeof(COLOR));}
	COLOR(byte Alpha, byte R, byte G, byte B)
		{this->Alpha = Alpha; this->R = R; this->G = G; this->B = B;}
	
	inline DWORD GetColor()
		const {return (((Alpha&0xFF)<<24)|((R&0xFF)<<16)|((G&0xFF)<<8)|(B&0xFF));}

};

struct PARTICLE
{
	VECTOR2D Pos;
	VECTOR2D Velocity;
	VECTOR2D Acceleration;
	
	DWORD counter;
	DWORD TTL;
	
	COLOR color_start;
	COLOR color_end;
	COLOR color_current;
	
	PARTICLE *prev;
	PARTICLE *next;

	PARTICLE() {memset(this, 0, sizeof(PARTICLE));}
	
	void Add(PARTICLE *particle);
	void Delete();
	bool Update(DWORD dwTime, VECTOR2D vGravity);
};

#endif