#ifndef COMPAT_AFFINE2D_H
#define COMPAT_AFFINE2D_H

// Stand-in for the D3DXMATRIX chain the original sprite/model layer
// uses. The legacy code only ever composed translations, uniform scales,
// Z-rotations, and horizontal flips (Y-axis pi rotations) — that subset
// is closed under composition and exactly representable as
//
//     (translate) * (flip) * (uniform scale) * (rotate Z) * (recenter)
//
// so we store the resolved [angle, scale, translation, flip] tuple and
// compose hierarchies by walking them ourselves rather than multiplying
// 4x4 matrices each frame. Render back-end (Phase 1i) feeds these
// straight into SDL_RenderCopyEx.

#include <cmath>

struct Affine2D {
    float angle_rad  = 0.0f;   // CCW positive — apply Z-rotation around the local origin
    float scale      = 1.0f;   // uniform
    float tx         = 0.0f;
    float ty         = 0.0f;
    bool  flip_x     = false;  // horizontal flip applied before scale/rotate

    static Affine2D identity() { return {}; }

    static Affine2D translation(float x, float y)
    {
        Affine2D a;
        a.tx = x;
        a.ty = y;
        return a;
    }

    // Compose: result transforms a point first by `child`, then by `parent`.
    // i.e. result = parent * child, in column-vector convention.
    //
    // Application order on the child's local position: scale -> rotate ->
    // flip -> translate. This matches the original D3DX matrix chain
    // (mTran * mScale * mRotY * mRotZ in row-vector terms). It MATTERS
    // when both the parent is rotated AND flipped — applying flip before
    // rotate gives the wrong sign on the sin components of the child's
    // translation, which is what made body-part positions diverge under
    // a flipped soldier with a tilted torso.
    static Affine2D compose(const Affine2D& parent, const Affine2D& child)
    {
        Affine2D r;
        r.scale     = parent.scale * child.scale;
        r.angle_rad = parent.angle_rad + child.angle_rad;
        r.flip_x    = parent.flip_x ^ child.flip_x;

        const float c = std::cos(parent.angle_rad);
        const float s = std::sin(parent.angle_rad);

        const float sx = child.tx * parent.scale;
        const float sy = child.ty * parent.scale;

        // rotate by parent.angle_rad (CCW in math convention)
        const float rx = c * sx - s * sy;
        const float ry = s * sx + c * sy;

        // X flip last (matches the row-vector chain ...mScale * mRotY * mRotZ)
        const float fx = parent.flip_x ? -rx : rx;

        r.tx = parent.tx + fx;
        r.ty = parent.ty + ry;
        return r;
    }
};

#endif  // COMPAT_AFFINE2D_H
