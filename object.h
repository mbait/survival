
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include "graphics/sprite2.h"
#include "physics/physics2D.h"

struct GAMEOBJECT
{
	SPRITE		*sprite;
	RIGIDBODY	*body;
	
	DWORD  TimeToLive;
	BYTE   AlphaColor;

	GAMEOBJECT *prev;
	GAMEOBJECT *next;
	
	void Add(SPRITE *sprite, RIGIDBODY *body, 
		DWORD TimeToLive, BYTE AlphaColor);
	void Delete();
};

#endif