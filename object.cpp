
#include "object.h"

void GAMEOBJECT::Add(SPRITE *sprite, RIGIDBODY *body, 
		DWORD TimeToLive, BYTE AlphaColor)
{
	GAMEOBJECT *tmp = new GAMEOBJECT;
	tmp->sprite = sprite;
	tmp->body = body;
	tmp->AlphaColor = AlphaColor;
	tmp->TimeToLive = TimeToLive;
	tmp->prev = this;
	tmp->next = this->next;
	
	this->next = tmp;
}

void GAMEOBJECT::Delete()
{
	prev->next = this->next;
	delete this;
}