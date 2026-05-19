#ifndef MODEL_H
#define MODEL_H

#include "sprite2.h"

#define HEAD 0
#define BODY 1
#define BELT 2
#define RSHOULDER 3
#define LSHOULDER 4
#define RARM 5
#define LARM 6
#define RHAND 7
#define LHAND 8
#define RTHIGH 9
#define LTHIGH 10
#define RLEG 11
#define LLEG 12
#define RFOOT 13
#define LFOOT 14
#define NUM_PARTS 15

#define MAX_FRAMES 20

#define ANIMATION_SINGLE 1001
#define ANIMATION_LOOP 1002
#define ANIMATION_RETURN 1003

typedef float KEYFRAME[NUM_PARTS];

struct ANIMATION
{
	int iStartFrame;
	int iEndFrame;
	float fRate;
	DWORD dwTime;
	int iType;
};

class MODEL
{
private:
	SPRITE aParts[NUM_PARTS];

	float fxPos, fyPos;
	float fRotation;
	float fScale;
	int iOrientation;

	KEYFRAME* frames;
	int iNumFrames;
	ANIMATION animation;
	bool bAnimationActive;

	// animation variables
	DWORD counter;
	DWORD cur_frame;
	DWORD anim_framecnt;

	KEYFRAME prev_state;

public:
	MODEL();
	~MODEL();

	inline float GetXPos() const
	{
		return fxPos;
	}
	inline float GetYPos() const
	{
		return fyPos;
	}
	inline float GetRotation() const
	{
		return fRotation;
	}
	inline float GetScale() const
	{
		return fScale;
	}
	inline int GetOrientation() const
	{
		return iOrientation;
	}

	inline void SetXPos(float fX)
	{
		fxPos = fX;
	}
	inline void SetYPos(float fY)
	{
		fyPos = fY;
	}
	inline void SetXYPos(float fX, float fY)
	{
		fxPos = fX;
		fyPos = fY;
	}
	inline void SetRotation(float fTheta)
	{
		fRotation = fTheta;
	}
	inline void SetScale(float fScale)
	{
		this->fScale = fScale;
	}
	inline void SetOrientation(int iOrientation)
	{
		this->iOrientation = iOrientation;
	}

	inline void MoveX(float fdX)
	{
		fxPos += fdX;
	}
	inline void MoveY(float fdY)
	{
		fyPos += fdY;
	}
	inline void MoveXY(float fdX, float fdY)
	{
		fxPos += fdX;
		fyPos += fdY;
	}
	inline void Rotate(float fTheta)
	{
		fRotation += fTheta;
	}

	inline bool IsAnimated() const
	{
		return bAnimationActive;
	}

	inline void Flip()
	{
		iOrientation = 1 - iOrientation;
	}

	inline SPRITE* GetPart(int nPartIndex)
	{
		return &aParts[nPartIndex];
	}

	HRESULT LoadFromFile(SDL_Renderer* renderer, const char* szFileName);

	HRESULT Draw(SPRITE* pSprite = 0, SPRITE* pFire = 0, BYTE Alpha = 0xFF);

	void SetAnimation(ANIMATION* animation);
	void StartAnimation();
	inline void ResetAnimation()
	{
		counter = 0;
		cur_frame = 0;
	}
	inline void StopAnimation()
	{
		bAnimationActive = false;
	}
	inline void SetCurrentFrame(DWORD dwFrameIndex)
	{
		cur_frame = dwFrameIndex;
	}
	inline void SetAnimationTime(DWORD dwTime)
	{
		animation.dwTime = dwTime;
	}
	void Tick(DWORD dwTime);
};

#endif
