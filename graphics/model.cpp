
#include <stdio.h>
#include "model.h"

MODEL::MODEL()
{
	fxPos = 0.0f;
	fyPos = 0.0f;
	fRotation = 0.0f;
	fScale = 1.0f;
	iOrientation = 0;
	
	frames = 0;
	iNumFrames = 0;
	memset(&animation, 0, sizeof(animation));
	bAnimationActive = 0;
	
	DWORD dwTimeCounter = 0;
}

MODEL::~MODEL()
{
	if(frames)
		delete [] frames;
}

HRESULT MODEL::LoadFromFile(LPDIRECT3DDEVICE9 &pDevice, const char *szFileName)
{
	if(frames)
		delete [] frames;
	
	FILE *f = fopen(szFileName, "rb");
	if(!f)
		return E_FAIL;

	//load textures
	for(int i=0; i<NUM_PARTS; i++)
	{
		int blocksize = 0;
		fread((void*)&blocksize, sizeof(int), 1, f);
		if(blocksize == -1)
		{
			fclose(f);
			return E_FAIL;
		}
		void *rawbuf = new byte[blocksize];
		fread(rawbuf, blocksize, 1, f);
		HRESULT hr = aParts[i].Init(pDevice, rawbuf, blocksize);
		if(FAILED(hr))
		{
			fclose(f);
			return E_FAIL;
		}

		delete [] rawbuf;
	}

	//load model params
	for(int i=0; i<NUM_PARTS; i++)
	{
		float vals[4];
		fread((void*)&vals, sizeof(vals), 1, f);
		aParts[i].SetXYPos(vals[0], vals[1]);
		aParts[i].SetRotationXY(vals[2], vals[3]);
	}

	//load animation
	int framecount = 0;
	fread((void*)&framecount, sizeof(framecount), 1, f);
	if(framecount>MAX_FRAMES)
		framecount = MAX_FRAMES;

	if(framecount>0)
	{
		frames = new KEYFRAME[framecount];
		fread((void*)frames, sizeof(KEYFRAME), framecount, f);
		iNumFrames = framecount;
	}
	
	return S_OK;
}

HRESULT MODEL::Draw(SPRITE *pSprite, SPRITE *pFire, BYTE Alpha)
{
	aParts[BELT].SetXYPos(fxPos, fyPos);
	aParts[BELT].SetOrientation(iOrientation);
	aParts[BELT].SetScale(fScale);
	
	if(bAnimationActive && iNumFrames>0)		
		for(int i=0; i<NUM_PARTS; i++)
		{
			float frame = animation.iStartFrame+cur_frame*animation.fRate;
			float frac = frame-(int)frame;

			float fRotation = frames[(int)frame][i]*(1-frac) + frames[(int)(frame+1)][i]*frac;
			aParts[i].SetRotation(aParts[i].GetRotation()+fRotation);
		}
	aParts[BELT].Rotate(fRotation);
		
	D3DXMATRIX matBelt = aParts[BELT].GetTransformationMatrix();                               
	D3DXMATRIX matLast = matBelt;                                                      
	D3DXMATRIX matBody;                                                                
	D3DXMatrixMultiply(&matBody, &aParts[BODY].GetTransformationMatrix(), &matBelt);           
	D3DXMATRIX matShoulder;                                                            
	D3DXMatrixMultiply(&matShoulder, &aParts[RSHOULDER].GetTransformationMatrix(), &matBody);   
	D3DXMATRIX matArm;                                                                 
	D3DXMatrixMultiply(&matArm, &aParts[RARM].GetTransformationMatrix(), &matShoulder);         
	D3DXMATRIX matHand;
	D3DXMatrixMultiply(&matHand, &aParts[RHAND].GetTransformationMatrix(), &matArm);
	D3DXMATRIX matThigh;                                                               
	D3DXMatrixMultiply(&matThigh, &aParts[RTHIGH].GetTransformationMatrix(), &matBelt);         
			                                                                                
	
	HRESULT hr;
	//right side                                                                       
	if(FAILED(hr = aParts[RHAND].Draw(&matArm, Alpha)))
		return hr;
	if(FAILED(hr = aParts[RARM].Draw(&matShoulder, Alpha)))
		return hr;
	if(FAILED(aParts[RSHOULDER].Draw(&matBody, Alpha)))
		return hr;
		                                                                                
	if(FAILED(aParts[RTHIGH].Draw(&matBelt, Alpha)))
		return hr;
	if(FAILED(aParts[RFOOT].Draw(D3DXMatrixMultiply(&matLast, 
			  &aParts[RLEG].GetTransformationMatrix(), &matThigh), Alpha)))
	    return hr;
	if(FAILED(aParts[RLEG].Draw(&matThigh, Alpha)))
		return hr;
		                                                                                
	//main part                                                                        
	if(FAILED(aParts[HEAD].Draw(&matBody, Alpha)))
		return hr;
	if(FAILED(aParts[BODY].Draw(&matBelt, Alpha)))
		return hr;
	if(FAILED(aParts[BELT].Draw(Alpha)))
		return hr;
	//aParts[BELT].Draw();                                                                     
	
	D3DXMatrixMultiply(&matBody, &aParts[BODY].GetTransformationMatrix(), &matBelt);           
	D3DXMatrixMultiply(&matShoulder, &aParts[LSHOULDER].GetTransformationMatrix(), &matBody);   
	D3DXMatrixMultiply(&matArm, &aParts[LARM].GetTransformationMatrix(), &matShoulder);         
	D3DXMatrixMultiply(&matHand, &aParts[LHAND].GetTransformationMatrix(), &matArm);
	D3DXMatrixMultiply(&matThigh, &aParts[LTHIGH].GetTransformationMatrix(), &matBelt);         

	//draw weapon
	if(pSprite)
	{
		hr = pSprite->Draw(D3DXMatrixMultiply(&matLast, &pSprite->GetTransformationMatrix(),
			&matHand));
		if(FAILED(hr))
			return hr;
	}

	if(pFire)
	{
		D3DXMATRIX matFire;
		D3DXMatrixTranslation(&matFire, 80.0f, 80.0f, 0.0f);
		D3DXMatrixMultiply(&matLast, &matFire, &matLast);
		D3DXMatrixRotationZ(&matFire, D3DX_PI/4.0f);
		D3DXMatrixMultiply(&matLast, &matFire, &matLast);
		
		hr = pFire->Draw(&matLast);
		if(FAILED(hr))
			return hr;
	}

	//left side                                                                        
	if(FAILED(aParts[LTHIGH].Draw(&matBelt, Alpha)))
		return hr;
	if(FAILED(aParts[LFOOT].Draw(D3DXMatrixMultiply(&matLast, 
			  &aParts[LLEG].GetTransformationMatrix(), &matThigh), Alpha)))
		return hr;
	if(FAILED(aParts[LLEG].Draw(&matThigh, Alpha)))
		return hr;
		                                                                                
	if(FAILED(aParts[LHAND].Draw(&matArm, Alpha)))
		return hr;
	if(FAILED(aParts[LARM].Draw(&matShoulder, Alpha)))
		return hr;
	return aParts[LSHOULDER].Draw(&matBody, Alpha);
}

void MODEL::SetAnimation(ANIMATION *animation)
{
	this->animation = *animation;
	anim_framecnt = (animation->iEndFrame-animation->iStartFrame)
		/animation->fRate;
}

void MODEL::StartAnimation()
{
	counter = 0; 
	cur_frame = 0; 
	bAnimationActive = true;

	for(int i=0; i<NUM_PARTS; i++)
		prev_state[i] = aParts[i].GetRotation();
}

void MODEL::Tick(DWORD dwTime)
{
	counter += dwTime;
	if(counter>=animation.dwTime)
		switch(animation.iType)
			{
				case ANIMATION_SINGLE:
					{
						bAnimationActive = false;
					}break;
				case ANIMATION_LOOP:
					{
						if(animation.dwTime)
							counter = counter%animation.dwTime; 
						cur_frame = (int)(anim_framecnt*((float)counter/animation.dwTime));
					}break;
				case ANIMATION_RETURN:
					{
						bAnimationActive = false;
						for(int i=0; i<NUM_PARTS; i++)
							aParts[i].SetRotation(prev_state[i]);
					}break;

			}

	else
		cur_frame = (int)(anim_framecnt*((float)counter/animation.dwTime));
}