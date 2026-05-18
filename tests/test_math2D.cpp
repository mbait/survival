// Unit tests for physics/math2D — exercises the 2D vector + matrix
// primitives the physics layer is built on. These tests don't pull in
// any of the game / SDL layer.

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>

#include "physics/math2D.h"

using Catch::Approx;
constexpr float kEps = 1e-5f;

namespace {
bool near(const VECTOR2D& a, const VECTOR2D& b, float tol = kEps)
{
    return std::fabs(a.x - b.x) < tol && std::fabs(a.y - b.y) < tol;
}
}

TEST_CASE("VECTOR2D constructors", "[math2D][vector]")
{
    SECTION("default zero-inits") {
        VECTOR2D v;
        CHECK(v.x == 0.0f);
        CHECK(v.y == 0.0f);
    }
    SECTION("(x, y) ctor") {
        VECTOR2D v(3.0f, -2.0f);
        CHECK(v.x == 3.0f);
        CHECK(v.y == -2.0f);
    }
    SECTION("angle ctor produces unit vector") {
        VECTOR2D v(0.0f);
        CHECK(v.x == Approx(1.0f));
        CHECK(v.y == Approx(0.0f).margin(kEps));
        VECTOR2D w(Pi / 2.0f);
        CHECK(w.x == Approx(0.0f).margin(kEps));
        CHECK(w.y == Approx(1.0f));
    }
}

TEST_CASE("VECTOR2D arithmetic", "[math2D][vector]")
{
    VECTOR2D a(1.0f, 2.0f), b(3.0f, 4.0f);
    CHECK(near(a + b, VECTOR2D(4.0f, 6.0f)));
    CHECK(near(b - a, VECTOR2D(2.0f, 2.0f)));
    CHECK(near(a * 2.0f, VECTOR2D(2.0f, 4.0f)));
    CHECK(near(b / 2.0f, VECTOR2D(1.5f, 2.0f)));

    VECTOR2D c = a;
    c += b; CHECK(near(c, VECTOR2D(4.0f, 6.0f)));
    c -= b; CHECK(near(c, a));
    c *= 3.0f; CHECK(near(c, VECTOR2D(3.0f, 6.0f)));
    c /= 3.0f; CHECK(near(c, a));
}

TEST_CASE("DotProduct / PerpDotProduct", "[math2D][products]")
{
    VECTOR2D a(2.0f, 3.0f), b(4.0f, -1.0f);
    CHECK(DotProduct(a, b) == Approx(5.0f));        // 2*4 + 3*(-1)
    CHECK(PerpDotProduct(a, b) == Approx(-14.0f));  // 2*(-1) - 3*4

    SECTION("orthogonal vectors have zero dot product") {
        CHECK(DotProduct(VECTOR2D(1.0f, 0.0f), VECTOR2D(0.0f, 1.0f)) == Approx(0.0f));
    }
    SECTION("parallel vectors have zero perp-dot product") {
        CHECK(PerpDotProduct(VECTOR2D(2.0f, 3.0f), VECTOR2D(4.0f, 6.0f)) == Approx(0.0f));
    }
}

TEST_CASE("Length and Normalize", "[math2D][vector]")
{
    CHECK(Length(VECTOR2D(3.0f, 4.0f)) == Approx(5.0f));
    CHECK(Length(VECTOR2D()) == Approx(0.0f));

    SECTION("normalize unit-length") {
        VECTOR2D n = Normalize(VECTOR2D(3.0f, 4.0f));
        CHECK(Length(n) == Approx(1.0f));
        CHECK(n.x == Approx(0.6f));
        CHECK(n.y == Approx(0.8f));
    }
    SECTION("normalize of zero returns zero (does not divide by 0)") {
        VECTOR2D n = Normalize(VECTOR2D());
        CHECK(near(n, VECTOR2D()));
    }
}

TEST_CASE("Perp rotates 90 degrees CCW", "[math2D][vector]")
{
    CHECK(near(Perp(VECTOR2D(1.0f, 0.0f)), VECTOR2D(0.0f, 1.0f)));
    CHECK(near(Perp(VECTOR2D(0.0f, 1.0f)), VECTOR2D(-1.0f, 0.0f)));
    // Two perps = pi rotation = negate
    VECTOR2D v(2.7f, -1.3f);
    CHECK(near(Perp(Perp(v)), VECTOR2D(-v.x, -v.y)));
}

TEST_CASE("DegToRad / RadToDeg round-trip", "[math2D][angle]")
{
    CHECK(DegToRad(180.0f) == Approx(Pi));
    CHECK(RadToDeg(Pi) == Approx(180.0f));
    for (float deg : { 0.0f, 30.0f, 45.0f, 90.0f, 270.0f }) {
        CHECK(RadToDeg(DegToRad(deg)) == Approx(deg));
    }
}

TEST_CASE("MATRIX identity and rotation", "[math2D][matrix]")
{
    MATRIX I = MATRIX::Identity();
    CHECK(I.e11 == 1.0f); CHECK(I.e12 == 0.0f);
    CHECK(I.e21 == 0.0f); CHECK(I.e22 == 1.0f);

    MATRIX R = MATRIX(Pi / 2.0f);
    // R rotates (1, 0) to... see how the operator interacts with VECTOR2D
    VECTOR2D rotated = VECTOR2D(1.0f, 0.0f) * R;
    // The matrix entries: e11=cos, e12=sin, e21=-sin, e22=cos.
    // VECTOR2D * MATRIX in math2D.cpp computes (x*e11 + y*e12, x*e21 + y*e22).
    // So (1,0) * R(pi/2) = (cos(pi/2), -sin(pi/2)) = (0, -1).
    CHECK(rotated.x == Approx(0.0f).margin(kEps));
    CHECK(rotated.y == Approx(-1.0f));
}

TEST_CASE("Rotate(point, pivot, angle)", "[math2D][rotation]")
{
    SECTION("around origin: 90° CCW takes +X to +Y") {
        VECTOR2D p(1.0f, 0.0f);
        Rotate(&p, nullptr, Pi / 2.0f);
        CHECK(p.x == Approx(0.0f).margin(kEps));
        CHECK(p.y == Approx(1.0f));
    }
    SECTION("around non-origin pivot") {
        VECTOR2D p(2.0f, 0.0f);
        VECTOR2D piv(1.0f, 0.0f);
        Rotate(&p, &piv, Pi);  // 180° around (1,0) takes (2,0) to (0,0)
        CHECK(p.x == Approx(0.0f).margin(kEps));
        CHECK(p.y == Approx(0.0f).margin(kEps));
    }
}
