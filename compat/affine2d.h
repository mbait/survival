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
    static Affine2D compose(const Affine2D& parent, const Affine2D& child)
    {
        Affine2D r;
        r.scale     = parent.scale * child.scale;
        r.angle_rad = parent.angle_rad + child.angle_rad;
        // Child's translation in its local frame; rotate+scale+flip by parent
        // to lift it into the parent's frame, then add parent's translation.
        const float c = std::cos(parent.angle_rad);
        const float s = std::sin(parent.angle_rad);
        const float fx = parent.flip_x ? -1.0f : 1.0f;
        const float px = fx * child.tx * parent.scale;
        const float py =       child.ty * parent.scale;
        r.tx = parent.tx + (c * px - s * py);
        r.ty = parent.ty + (s * px + c * py);
        r.flip_x = parent.flip_x ^ child.flip_x;
        return r;
    }
};

#endif  // COMPAT_AFFINE2D_H
