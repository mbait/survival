
#include "sprite2.h"

SPRITE::SPRITE()
{
	pSprite = 0;
	pTexture = 0;

	fxPos = 0.0f; 
	fyPos = 0.0f;
	fRotation = 0.0f;
	fScale = 1.0f;

	fRotationX = 0.0f;
	fRotationY = 0.0f;

	iOrientation = 0;

	iWidth = 0;
	iHeight = 0;
}

SPRITE::~SPRITE()
{
	if(pSprite)
		pSprite->Release();
	if(pTexture)
		pTexture->Release();
}

HRESULT SPRITE::Init(LPDIRECT3DDEVICE9 &pDevice, const char* szFileName)
{
	pSprite = 0;
	pTexture = 0;
	
	HRESULT hr;
	if(FAILED(hr = D3DXCreateSprite(pDevice, &pSprite)))
		return hr;

	if(FAILED(hr = D3DXCreateTextureFromFile(pDevice, szFileName, &pTexture)))
		return hr;

	D3DSURFACE_DESC d3dsd;
	if(FAILED(hr = pTexture->GetLevelDesc(0, &d3dsd)))
		return hr;

	iWidth = d3dsd.Width;
	iHeight = d3dsd.Height;

	fRotationX = iWidth / 2.0f;
	fRotationY = iHeight / 2.0f;

	return S_OK;
	/*if(FAILED(hr = D3DXCreateTextureFromFileEx(pDevice, szFileName, 0, 0, 0, 0,
											   D3DFMT_A8R8G8B8, D3DPOOL_MANAGED,
											   D3DX_FILTER_NONE, D3DX_DEFAULT,
											   0x00000000, 0, 0, &pTexture)))
		return hr;*/
}

HRESULT SPRITE::Init(LPDIRECT3DDEVICE9 &pDevice, LPVOID memptr, int nFileSize)
{
	pSprite = 0;
	pTexture = 0;
	
	HRESULT hr;
	if(FAILED(hr = D3DXCreateSprite(pDevice, &pSprite)))
		return hr;

	if(FAILED(hr = D3DXCreateTextureFromFileInMemory(pDevice, 
					   memptr, nFileSize, &pTexture)))
		return hr;

	D3DSURFACE_DESC d3dsd;
	if(FAILED(hr = pTexture->GetLevelDesc(0, &d3dsd)))
		return hr;

	iWidth = d3dsd.Width;
	iHeight = d3dsd.Height;

	fRotationX = iWidth / 2.0f;
	fRotationY = iHeight / 2.0f;

	return S_OK;
}

HRESULT SPRITE::Draw(BYTE Alpha)
{
	pSprite->Begin();
	
	HRESULT hr;
	/*if(iOrientation>0)
		hr = pSprite->Draw(pTexture, 0, &D3DXVECTOR2(fScale, fScale), &D3DXVECTOR2(fRotationX, fRotationY),
						   fRotation, &D3DXVECTOR2(fxPos, fyPos), 0xFFFFFFFF);
	else
		hr = pSprite->Draw(pTexture, 0, &D3DXVECTOR2(-fScale,  fScale), 
						   &D3DXVECTOR2(fRotationX-iWidth, fRotationY),
						   fRotation, &D3DXVECTOR2(fxPos+iWidth, fyPos), 0xFFFFFFFF);*/
	
	D3DXMATRIX mScale;
	D3DXMATRIX mRotationY, mRotationZ;
	D3DXMATRIX mTranslation;
	D3DXMATRIX mCenter;

	D3DXMatrixScaling(&mScale, fScale, fScale, fScale);
	D3DXMatrixRotationY(&mRotationY, iOrientation*D3DX_PI);
	D3DXMatrixRotationZ(&mRotationZ, fRotation);
	D3DXMatrixTranslation(&mTranslation, fxPos, fyPos, 0.0f);
	D3DXMatrixTranslation(&mCenter, -fRotationX, -fRotationY, 0.0f);

	
	D3DXMatrixMultiply(&mRotationZ, &mRotationZ, &mRotationY);
	D3DXMatrixMultiply(&mRotationZ, &mRotationZ, &mScale);
	D3DXMatrixMultiply(&mRotationZ, &mCenter, &mRotationZ);
	D3DXMatrixMultiply(&mTranslation, &mRotationZ, &mTranslation);

	hr = pSprite->DrawTransform(pTexture, 0, &mTranslation, D3DCOLOR_ARGB(Alpha, 255, 255, 255));
	if(FAILED(hr))
		return hr;

	return pSprite->End();
}

HRESULT SPRITE::Draw(D3DXMATRIX *matInit, BYTE Alpha)
{
	pSprite->Begin();

	HRESULT hr;

	D3DXMATRIX mScale;
	D3DXMATRIX mRotationY, mRotationZ;
	D3DXMATRIX mTranslation;
	D3DXMATRIX mCenter;

	D3DXMatrixScaling(&mScale, fScale, fScale, fScale);
	D3DXMatrixRotationY(&mRotationY, iOrientation*D3DX_PI);
	D3DXMatrixRotationZ(&mRotationZ, fRotation);
	D3DXMatrixTranslation(&mTranslation, fxPos, fyPos, 0.0f);
	D3DXMatrixTranslation(&mCenter, -fRotationX, -fRotationY, 0.0f);

	
	D3DXMatrixMultiply(&mRotationZ, &mRotationZ, &mRotationY);
	D3DXMatrixMultiply(&mRotationZ, &mRotationZ, &mScale);
	D3DXMatrixMultiply(&mRotationZ, &mCenter, &mRotationZ);
	D3DXMatrixMultiply(&mTranslation, &mRotationZ, &mTranslation);
	
	D3DXMATRIX matResult;
	D3DXMatrixMultiply(&matResult, &mTranslation, matInit);
	
	hr = pSprite->DrawTransform(pTexture, 0, &matResult, D3DCOLOR_ARGB(Alpha, 255, 255, 255));
	if(FAILED(hr))
		return hr;

	return pSprite->End();
}

HRESULT SPRITE::Draw(float fX, float fY, float fRotation, 
					 float fRotationX, float fRotationY, 
					 float fScale, BYTE Alpha)
{
	pSprite->Begin();

	HRESULT hr = pSprite->Draw(pTexture, 0, &D3DXVECTOR2(fScale, fScale), 
							   &D3DXVECTOR2(fRotationX, fRotationY),
							   fRotation, &D3DXVECTOR2(fX-fRotationX, 
							   fY-fRotationY), D3DCOLOR_ARGB(Alpha, 255, 255, 255));
	if(FAILED(hr))
		return hr;

	return pSprite->End();
}

D3DXMATRIX SPRITE::GetTransformationMatrix()
{
	D3DXMATRIX mScale;
	D3DXMATRIX mRotationY, mRotationZ;
	D3DXMATRIX mTranslation;
	D3DXMATRIX mCenter;

	D3DXMatrixScaling(&mScale, fScale, fScale, fScale);
	D3DXMatrixRotationY(&mRotationY, iOrientation*D3DX_PI);
	D3DXMatrixRotationZ(&mRotationZ, fRotation);
	D3DXMatrixTranslation(&mTranslation, fxPos, fyPos, 0.0f);
	D3DXMatrixTranslation(&mCenter, -fRotationX, -fRotationY, 0.0f);

	
	D3DXMatrixMultiply(&mRotationZ, &mRotationZ, &mRotationY);
	D3DXMatrixMultiply(&mRotationZ, &mRotationZ, &mScale);
	//D3DXMatrixMultiply(&mRotationZ, &mCenter, &mRotationZ);
	D3DXMatrixMultiply(&mTranslation, &mRotationZ, &mTranslation);

	return mTranslation;
}