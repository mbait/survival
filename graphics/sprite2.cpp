#include "sprite2.h"

#include <SDL_image.h>

namespace {

constexpr float kRadToDeg = 57.295779513082320876f;  // 180 / pi

// SDL_RenderCopyEx rotates clockwise in screen space (Y down); the
// legacy code treats positive Z-rotation as CCW in math convention. In
// screen space those agree (Y is flipped), so the angle goes through
// unmodified — verify visually during Phase 2 reference matching.
inline double to_sdl_angle_deg(float angle_rad)
{
    return static_cast<double>(angle_rad) * kRadToDeg;
}

// Build the SDL destination rect + rotation center for a sprite of size
// (w,h) drawn so that its local pivot point lands at world (px,py).
void compose_dst(float px, float py,
                 float pivot_x, float pivot_y,
                 int   w,       int   h,
                 float scale,
                 SDL_Rect& out_dst, SDL_Point& out_center)
{
    const float scaled_w     = w * scale;
    const float scaled_h     = h * scale;
    const float scaled_pivot_x = pivot_x * scale;
    const float scaled_pivot_y = pivot_y * scale;

    out_dst.x = static_cast<int>(px - scaled_pivot_x);
    out_dst.y = static_cast<int>(py - scaled_pivot_y);
    out_dst.w = static_cast<int>(scaled_w);
    out_dst.h = static_cast<int>(scaled_h);

    out_center.x = static_cast<int>(scaled_pivot_x);
    out_center.y = static_cast<int>(scaled_pivot_y);
}

HRESULT finish_init(SDL_Renderer* renderer, SDL_Texture* tex,
                    SDL_Texture*& out_tex, SDL_Renderer*& out_renderer,
                    int& out_w, int& out_h,
                    float& out_pivot_x, float& out_pivot_y)
{
    if (!tex) {
        return E_FAIL;
    }
    int w = 0;
    int h = 0;
    if (SDL_QueryTexture(tex, nullptr, nullptr, &w, &h) != 0) {
        SDL_DestroyTexture(tex);
        return E_FAIL;
    }
    out_tex      = tex;
    out_renderer = renderer;
    out_w        = w;
    out_h        = h;
    out_pivot_x  = w / 2.0f;
    out_pivot_y  = h / 2.0f;
    return S_OK;
}

}  // namespace

SPRITE::~SPRITE()
{
    if (pTexture) {
        SDL_DestroyTexture(pTexture);
        pTexture = nullptr;
    }
}

HRESULT SPRITE::Init(SDL_Renderer* renderer, const char* szFileName)
{
    if (!renderer || !szFileName) {
        return E_FAIL;
    }
    SDL_Texture* tex = IMG_LoadTexture(renderer, szFileName);
    return finish_init(renderer, tex, pTexture, pRenderer,
                       iWidth, iHeight, fRotationX, fRotationY);
}

HRESULT SPRITE::Init(SDL_Renderer* renderer, const void* memptr, int nFileSize)
{
    if (!renderer || !memptr || nFileSize <= 0) {
        return E_FAIL;
    }
    SDL_RWops* rw = SDL_RWFromConstMem(memptr, nFileSize);
    if (!rw) {
        return E_FAIL;
    }
    // freesrc=1 → SDL closes the RWops for us.
    SDL_Texture* tex = IMG_LoadTexture_RW(renderer, rw, 1);
    return finish_init(renderer, tex, pTexture, pRenderer,
                       iWidth, iHeight, fRotationX, fRotationY);
}

HRESULT SPRITE::Draw(BYTE Alpha)
{
    if (!pRenderer || !pTexture) {
        return E_FAIL;
    }

    SDL_Rect  dst;
    SDL_Point center;
    compose_dst(fxPos, fyPos, fRotationX, fRotationY,
                iWidth, iHeight, fScale, dst, center);

    SDL_SetTextureAlphaMod(pTexture, Alpha);
    SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND);

    const SDL_RendererFlip flip = iOrientation ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    if (SDL_RenderCopyEx(pRenderer, pTexture, nullptr, &dst,
                         to_sdl_angle_deg(fRotation), &center, flip) != 0) {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT SPRITE::Draw(const Affine2D& parent, BYTE Alpha)
{
    if (!pRenderer || !pTexture) {
        return E_FAIL;
    }

    // Treat the SPRITE's own (pos, rot, scale, flip) as its local
    // transform; concatenate the parent in front, then render.
    const Affine2D world = Affine2D::compose(parent, GetTransform());

    SDL_Rect  dst;
    SDL_Point center;
    compose_dst(world.tx, world.ty,
                fRotationX, fRotationY,
                iWidth, iHeight, world.scale, dst, center);

    SDL_SetTextureAlphaMod(pTexture, Alpha);
    SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND);

    const SDL_RendererFlip flip = world.flip_x ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    if (SDL_RenderCopyEx(pRenderer, pTexture, nullptr, &dst,
                         to_sdl_angle_deg(world.angle_rad), &center, flip) != 0) {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT SPRITE::Draw(float fX, float fY, float fR,
                     float fRX, float fRY,
                     float fS, BYTE Alpha)
{
    if (!pRenderer || !pTexture) {
        return E_FAIL;
    }

    SDL_Rect  dst;
    SDL_Point center;
    compose_dst(fX, fY, fRX, fRY, iWidth, iHeight, fS, dst, center);

    SDL_SetTextureAlphaMod(pTexture, Alpha);
    SDL_SetTextureBlendMode(pTexture, SDL_BLENDMODE_BLEND);

    const SDL_RendererFlip flip = iOrientation ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    if (SDL_RenderCopyEx(pRenderer, pTexture, nullptr, &dst,
                         to_sdl_angle_deg(fR), &center, flip) != 0) {
        return E_FAIL;
    }
    return S_OK;
}

Affine2D SPRITE::GetTransform() const
{
    Affine2D a;
    a.tx        = fxPos;
    a.ty        = fyPos;
    a.angle_rad = fRotation;
    a.scale     = fScale;
    a.flip_x    = (iOrientation != 0);
    return a;
}
