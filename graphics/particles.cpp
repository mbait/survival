
#include "particles.h"

void PARTICLE::Add(PARTICLE *particle)
{
	PARTICLE *tmp = (PARTICLE*)malloc(sizeof(PARTICLE));
	*tmp = *particle;
	if(next)
		next->prev = tmp;
	tmp->prev = this;
	tmp->next = next;
	next = tmp;

	tmp->counter = 0;
}

void PARTICLE::Delete()
{
	if(prev)
		prev->next = next;
	if(next)
		next->prev = prev;
	free(this);
}

bool PARTICLE::Update(DWORD dwTime, VECTOR2D vGravity)
{
		counter += dwTime;
		
		float fTime = dwTime/1000.0f;
		Pos += Velocity*fTime;
		Velocity += (Acceleration+vGravity)*fTime;	

		color_current = color_start;
		
		color_current.Alpha += (int)
			(color_end.Alpha-color_start.Alpha)*
			((float)counter/(float)TTL);

		color_current.R += (int)
			(color_end.R-color_start.R)*
			((float)counter/(float)TTL);

		color_current.G += (int)
			(color_end.G-color_start.G)*
			((float)counter/(float)TTL);

		color_current.B += (int)
			(color_end.B-color_start.B)*
			((float)counter/(float)TTL);

		if(counter>TTL)
			return false;
		else
			return true;
}