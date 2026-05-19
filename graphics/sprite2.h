#ifndef SPRITE2_H
#define SPRITE2_H

#include <SDL.h>

#include "compat/affine2d.h"
#include "compat/win32_compat.h"

// Textured 2D sprite. Holds a single SDL_Texture* and renders via
// SDL_RenderCopyEx. The public interface preserves the legacy
// (fxPos, fyPos, fRotation, fScale, iOrientation, fRotationX/Y) state
// vector so model.cpp / gamecode.cpp can keep their existing call sites
// — only the underlying device/texture types change.
class SPRITE
{
private:
	SDL_Renderer* pRenderer = nullptr;
	SDL_Texture* pTexture = nullptr;

	float fxPos = 0.0f;
	float fyPos = 0.0f;
	float fRotation = 0.0f; // radians, CCW positive
	float fScale = 1.0f;

	float fRotationX = 0.0f; // rotation pivot in texture-local coords
	float fRotationY = 0.0f;

	int iOrientation = 0; // 0 = unflipped, 1 = horizontally flipped

public:
	SPRITE() = default;
	~SPRITE();

	SPRITE(const SPRITE&) = delete;
	SPRITE& operator=(const SPRITE&) = delete;

	int iWidth = 0;
	int iHeight = 0;

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
	inline float GetRotationX() const
	{
		return fRotationX;
	}
	inline float GetRotationY() const
	{
		return fRotationY;
	}
	inline float GetScale() const
	{
		return fScale;
	}
	inline int GetOrientation() const
	{
		return iOrientation;
	}

	inline void SetXPos(float x)
	{
		fxPos = x;
	}
	inline void SetYPos(float y)
	{
		fyPos = y;
	}
	inline void SetXYPos(float x, float y)
	{
		fxPos = x;
		fyPos = y;
	}
	inline void SetRotation(float theta)
	{
		fRotation = theta;
	}
	inline void SetRotationX(float rx)
	{
		fRotationX = rx;
	}
	inline void SetRotationY(float ry)
	{
		fRotationY = ry;
	}
	inline void SetRotationXY(float rx, float ry)
	{
		fRotationX = rx;
		fRotationY = ry;
	}
	inline void SetScale(float s)
	{
		fScale = s;
	}
	inline void SetOrientation(int o)
	{
		iOrientation = o;
	}

	inline void MoveX(float dx)
	{
		fxPos += dx;
	}
	inline void MoveY(float dy)
	{
		fyPos += dy;
	}
	inline void MoveXY(float dx, float dy)
	{
		fxPos += dx;
		fyPos += dy;
	}
	inline void Rotate(float theta)
	{
		fRotation += theta;
	}

	inline void Flip()
	{
		iOrientation = 1 - iOrientation;
	}

	// Load a texture from disk. `pRenderer` is borrowed (not owned).
	HRESULT Init(SDL_Renderer* renderer, const char* szFileName);

	// Load a texture from an in-memory buffer. The buffer is copied
	// internally so the caller can free it after this call returns.
	HRESULT Init(SDL_Renderer* renderer, const void* memptr, int nFileSize);

	// Draw using the SPRITE's internal pos/rotation/scale/orientation.
	HRESULT Draw(BYTE Alpha = 0xFF);

	// Draw with `parent` applied on top of the internal transform; used
	// by the articulated model layer for hierarchical bone chains.
	HRESULT Draw(const Affine2D& parent, BYTE Alpha = 0xFF);

	// Fully-specified draw — overrides the internal state.
	HRESULT Draw(float fX, float fY, float fRotation, float fRX, float fRY, float fScale = 1.0f,
	             BYTE Alpha = 0xFF);

	// The SPRITE's current local-to-parent transform (used as input to
	// the matrix overload of Draw, or to walk a hierarchy in model.cpp).
	Affine2D GetTransform() const;
};

#endif // SPRITE2_H
