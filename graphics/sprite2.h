
#if !defined(SPRITE2_H)
#define SPRITE2_H

#include <d3d9.h>
#include <d3dx9.h>

class SPRITE
{
private:
	LPD3DXSPRITE	   pSprite;
	LPDIRECT3DTEXTURE9 pTexture;
	
	float fxPos, fyPos;
	float fRotation;
	float fScale;

	float fRotationX, fRotationY;

	int iOrientation;

public:
	SPRITE();
	~SPRITE();
	
	int iWidth, iHeight;

	inline float GetXPos() const {return fxPos;}
	inline float GetYPos() const {return fyPos;}
	inline float GetRotation() const {return fRotation;}
	inline float GetRotationX() const {return fRotationX;}
	inline float GetRotationY() const {return fRotationY;}
	inline float GetScale() const {return fScale;}
	inline int	 GetOrientation() const {return iOrientation;}

	inline void SetXPos(float fX) {fxPos = fX;}
	inline void SetYPos(float fY) {fyPos = fY;}
	inline void SetXYPos(float fX, float fY) {fxPos = fX; fyPos = fY;}
	inline void SetRotation(float fTheta) {fRotation = fTheta;}
	inline void SetRotationX(float fRX) {fRotationX = fRX;}
	inline void SetRotationY(float fRY) {fRotationY = fRY;}
	inline void SetRotationXY(float fRX, float fRY) {fRotationX = fRX; fRotationY = fRY;}
	inline void SetScale(float fScale) {this->fScale = fScale;}
	inline void SetOrientation(int iOrientation) {this->iOrientation = iOrientation;}

	inline void MoveX(float fdX) {fxPos += fdX;}
	inline void MoveY(float fdY) {fyPos += fdY;}
	inline void MoveXY(float fdX, float fdY) {fxPos += fdX; fyPos += fdY;}
	inline void Rotate(float fTheta) {fRotation += fTheta;}

	inline void Flip() {iOrientation = 1-iOrientation;}

	HRESULT Init(LPDIRECT3DDEVICE9 &pDevice, const char* szFileName);
	HRESULT Init(LPDIRECT3DDEVICE9 &pDevice, LPVOID memptr, int nFileSize);
	HRESULT Draw(BYTE Alpha = 0xFF);
	HRESULT Draw(D3DXMATRIX *matInit, BYTE Alpha = 0xFF);
	HRESULT Draw(float fX, float fY, float fRotation, float fRotationX, float fRotationY, float fScale = 1.0f, BYTE Alpha = 0xFF);

	D3DXMATRIX GetTransformationMatrix();
};

#endif