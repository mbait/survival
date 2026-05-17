#include "model.h"

#include <cstdio>
#include <cstring>

#include "compat/affine2d.h"
#include "compat/win32_compat.h"

MODEL::MODEL()
{
	fxPos = 0.0f;
	fyPos = 0.0f;
	fRotation = 0.0f;
	fScale = 1.0f;
	iOrientation = 0;

	frames = 0;
	iNumFrames = 0;
	std::memset(&animation, 0, sizeof(animation));
	bAnimationActive = false;

	counter = 0;
	cur_frame = 0;
	anim_framecnt = 0;
}

MODEL::~MODEL()
{
	if (frames) {
		delete[] frames;
	}
}

HRESULT MODEL::LoadFromFile(SDL_Renderer* renderer, const char* szFileName)
{
	if (frames) {
		delete[] frames;
		frames = nullptr;
	}

	FILE* f = std::fopen(szFileName, "rb");
	if (!f) {
		return E_FAIL;
	}

	// Load textures — each part has its texture bytes inline in the .m2d.
	for (int i = 0; i < NUM_PARTS; i++) {
		int blocksize = 0;
		std::fread(&blocksize, sizeof(int), 1, f);
		if (blocksize == -1) {
			std::fclose(f);
			return E_FAIL;
		}
		void* rawbuf = new BYTE[blocksize];
		std::fread(rawbuf, blocksize, 1, f);
		HRESULT hr = aParts[i].Init(renderer, rawbuf, blocksize);
		delete[] static_cast<BYTE*>(rawbuf);
		if (FAILED(hr)) {
			std::fclose(f);
			return E_FAIL;
		}
	}

	// Per-part offset + rotation pivot.
	for (int i = 0; i < NUM_PARTS; i++) {
		float vals[4];
		std::fread(vals, sizeof(vals), 1, f);
		aParts[i].SetXYPos(vals[0], vals[1]);
		aParts[i].SetRotationXY(vals[2], vals[3]);
	}

	// Animation key-frames.
	int framecount = 0;
	std::fread(&framecount, sizeof(framecount), 1, f);
	if (framecount > MAX_FRAMES) {
		framecount = MAX_FRAMES;
	}
	if (framecount > 0) {
		frames = new KEYFRAME[framecount];
		std::fread(frames, sizeof(KEYFRAME), framecount, f);
		iNumFrames = framecount;
	}

	std::fclose(f);
	return S_OK;
}

HRESULT MODEL::Draw(SPRITE* pSprite, SPRITE* pFire, BYTE Alpha)
{
	// Position the root (belt) at the model's world location.
	aParts[BELT].SetXYPos(fxPos, fyPos);
	aParts[BELT].SetOrientation(iOrientation);
	aParts[BELT].SetScale(fScale);

	// Layer the current key-frame rotation deltas on top of each part's
	// authored rotation, interpolating between adjacent frames. Original
	// behaviour preserved verbatim — including the curiosity that
	// aParts[BELT].Rotate(fRotation) runs OUTSIDE the if-block (i.e. even
	// when the model isn't animated).
	if (bAnimationActive && iNumFrames > 0) {
		for (int i = 0; i < NUM_PARTS; i++) {
			float frame = animation.iStartFrame + cur_frame * animation.fRate;
			float frac = frame - static_cast<int>(frame);

			float fRotation = frames[static_cast<int>(frame)][i] * (1 - frac)
			                + frames[static_cast<int>(frame + 1)][i] * frac;
			aParts[i].SetRotation(aParts[i].GetRotation() + fRotation);
		}
	}
	aParts[BELT].Rotate(fRotation);

	// Hierarchy:
	//   belt -> body -> { head, shoulder -> arm -> hand, ... }
	//   belt -> thigh -> leg -> foot
	const Affine2D matBelt     = aParts[BELT].GetTransform();
	const Affine2D matBody     = Affine2D::compose(matBelt,    aParts[BODY].GetTransform());
	const Affine2D matRSh      = Affine2D::compose(matBody,    aParts[RSHOULDER].GetTransform());
	const Affine2D matRArm     = Affine2D::compose(matRSh,     aParts[RARM].GetTransform());
	const Affine2D matRThigh   = Affine2D::compose(matBelt,    aParts[RTHIGH].GetTransform());
	const Affine2D matRLeg     = Affine2D::compose(matRThigh,  aParts[RLEG].GetTransform());

	HRESULT hr;
	//right side
	if (FAILED(hr = aParts[RHAND].Draw(matRArm, Alpha))) return hr;
	if (FAILED(hr = aParts[RARM].Draw(matRSh, Alpha)))   return hr;
	if (FAILED(hr = aParts[RSHOULDER].Draw(matBody, Alpha))) return hr;
	if (FAILED(hr = aParts[RTHIGH].Draw(matBelt, Alpha)))    return hr;
	if (FAILED(hr = aParts[RFOOT].Draw(matRLeg, Alpha)))     return hr;
	if (FAILED(hr = aParts[RLEG].Draw(matRThigh, Alpha)))    return hr;

	//main parts
	if (FAILED(hr = aParts[HEAD].Draw(matBody, Alpha))) return hr;
	if (FAILED(hr = aParts[BODY].Draw(matBelt, Alpha))) return hr;
	if (FAILED(hr = aParts[BELT].Draw(Alpha)))          return hr;

	const Affine2D matLSh      = Affine2D::compose(matBody,    aParts[LSHOULDER].GetTransform());
	const Affine2D matLArm     = Affine2D::compose(matLSh,     aParts[LARM].GetTransform());
	const Affine2D matLHand    = Affine2D::compose(matLArm,    aParts[LHAND].GetTransform());
	const Affine2D matLThigh   = Affine2D::compose(matBelt,    aParts[LTHIGH].GetTransform());
	const Affine2D matLLeg     = Affine2D::compose(matLThigh,  aParts[LLEG].GetTransform());

	//draw weapon
	if (pSprite) {
		const Affine2D matWeapon = Affine2D::compose(matLHand, pSprite->GetTransform());
		hr = pSprite->Draw(matWeapon, Alpha);
		if (FAILED(hr)) return hr;

		if (pFire) {
			// Legacy offset: translate by (80, 80) then rotate by pi/4
			// relative to the weapon transform.
			Affine2D matFire = matWeapon;
			matFire = Affine2D::compose(matFire, Affine2D::translation(80.0f, 80.0f));
			Affine2D rot;
			rot.angle_rad = D3DX_PI / 4.0f;
			matFire = Affine2D::compose(matFire, rot);
			hr = pFire->Draw(matFire, Alpha);
			if (FAILED(hr)) return hr;
		}
	}

	//left side
	if (FAILED(hr = aParts[LTHIGH].Draw(matBelt, Alpha)))   return hr;
	if (FAILED(hr = aParts[LFOOT].Draw(matLLeg, Alpha)))    return hr;
	if (FAILED(hr = aParts[LLEG].Draw(matLThigh, Alpha)))   return hr;

	if (FAILED(hr = aParts[LHAND].Draw(matLArm, Alpha)))    return hr;
	if (FAILED(hr = aParts[LARM].Draw(matLSh, Alpha)))      return hr;
	return aParts[LSHOULDER].Draw(matBody, Alpha);
}

void MODEL::SetAnimation(ANIMATION* a)
{
	this->animation = *a;
	anim_framecnt = (a->iEndFrame - a->iStartFrame) / a->fRate;
}

void MODEL::StartAnimation()
{
	counter = 0;
	cur_frame = 0;
	bAnimationActive = true;

	for (int i = 0; i < NUM_PARTS; i++) {
		prev_state[i] = aParts[i].GetRotation();
	}
}

void MODEL::Tick(DWORD dwTime)
{
	counter += dwTime;
	if (counter >= animation.dwTime) {
		switch (animation.iType) {
			case ANIMATION_SINGLE:
				bAnimationActive = false;
				break;
			case ANIMATION_LOOP:
				if (animation.dwTime) {
					counter = counter % animation.dwTime;
				}
				cur_frame = static_cast<DWORD>(anim_framecnt *
				    (static_cast<float>(counter) / animation.dwTime));
				break;
			case ANIMATION_RETURN:
				bAnimationActive = false;
				for (int i = 0; i < NUM_PARTS; i++) {
					aParts[i].SetRotation(prev_state[i]);
				}
				break;
		}
	} else {
		cur_frame = static_cast<DWORD>(anim_framecnt *
		    (static_cast<float>(counter) / animation.dwTime));
	}
}
