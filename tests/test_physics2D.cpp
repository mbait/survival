// Unit tests for physics/physics2D. Focuses on the rigid-body collision
// detection (SAT MTD + ray + circle intersect) and the JOINT primitive,
// which together cover the surface area gameplay depends on.

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <vector>

#include "physics/physics2D.h"

using Catch::Approx;
constexpr float kEps = 1e-3f;

namespace
{
std::vector<VECTOR2D> unit_square(float cx, float cy, float half = 0.5f)
{
	return {
	    {cx - half, cy - half},
	    {cx + half, cy - half},
	    {cx + half, cy + half},
	    {cx - half, cy + half},
	};
}
} // namespace

TEST_CASE("RIGIDBODY default ctor is zero-initialised", "[physics][rigidbody]")
{
	RIGIDBODY b;
	CHECK(b.fMass == 0.0f);
	CHECK(b.fInertia == 0.0f);
	CHECK(b.Pos.x == 0.0f);
	CHECK(b.Pos.y == 0.0f);
	CHECK(b.iNumVertices == 0);
	CHECK(b.lpVertices.empty());
	CHECK(b.IsStatic());
}

TEST_CASE("RIGIDBODY param ctor: square", "[physics][rigidbody]")
{
	auto verts = unit_square(10.0f, 20.0f);

	SECTION("static body has zero mass")
	{
		RIGIDBODY w(verts.data(), 4, /*fStatic=*/true);
		CHECK(w.iNumVertices == 4);
		CHECK(w.lpVertices.size() == 4u);
		CHECK(w.IsStatic());
		CHECK(w.fMass == 0.0f);
		CHECK(w.Pos.x == Approx(10.0f));
		CHECK(w.Pos.y == Approx(20.0f));
	}

	SECTION("dynamic body has non-zero mass + inertia")
	{
		RIGIDBODY w(verts.data(), 4, /*fStatic=*/false);
		CHECK(!w.IsStatic());
		CHECK(w.fMass > 0.0f);
		CHECK(w.fInertia > 0.0f);
	}
}

TEST_CASE("RIGIDBODY::Collide", "[physics][collide]")
{
	SECTION("overlapping squares return true with MTD pointing away")
	{
		auto va = unit_square(0.0f, 0.0f);
		auto vb = unit_square(0.5f, 0.0f); // overlaps along +X
		RIGIDBODY A(va.data(), 4), B(vb.data(), 4);
		VECTOR2D MTD;
		float t = 0.0f;
		REQUIRE(A.Collide(B, MTD, t));
		// MTD should be approximately +/- X axis; the contract is that
		// pushing A by MTD*t separates them. Whatever the sign convention,
		// |MTD| ~= 1 and Y-component near zero.
		CHECK(std::fabs(MTD.y) == Approx(0.0f).margin(0.05f));
		CHECK(std::fabs(MTD.x) > 0.5f);
		CHECK(t > 0.0f);
	}
	SECTION("non-overlapping squares return false")
	{
		auto va = unit_square(0.0f, 0.0f);
		auto vb = unit_square(5.0f, 5.0f);
		RIGIDBODY A(va.data(), 4), B(vb.data(), 4);
		VECTOR2D MTD;
		float t = 0.0f;
		CHECK_FALSE(A.Collide(B, MTD, t));
	}
}

TEST_CASE("RayIntersect", "[physics][ray]")
{
	auto verts = unit_square(0.0f, 0.0f);
	RIGIDBODY box(verts.data(), 4);

	SECTION("ray hitting the box from the left")
	{
		VECTOR2D N;
		float t = 0.0f;
		REQUIRE(RayIntersect(box, VECTOR2D(-2.0f, 0.0f), VECTOR2D(2.0f, 0.0f), t, N));
		CHECK(t > 0.0f);
		CHECK(t < 1.0f);
		// Normal should be along ±X.
		CHECK(std::fabs(N.y) <= std::fabs(N.x));
	}

	SECTION("ray missing the box")
	{
		VECTOR2D N;
		float t = 0.0f;
		CHECK_FALSE(RayIntersect(box, VECTOR2D(-2.0f, 10.0f), VECTOR2D(2.0f, 10.0f), t, N));
	}
}

TEST_CASE("CircleIntersect", "[physics][circle]")
{
	auto verts = unit_square(0.0f, 0.0f);
	RIGIDBODY box(verts.data(), 4);

	SECTION("circle inside / overlapping the box")
	{
		VECTOR2D N;
		float t = 0.0f;
		REQUIRE(CircleIntersect(box, VECTOR2D(0.3f, 0.0f), 0.3f, t, N));
	}
	SECTION("circle far away misses")
	{
		VECTOR2D N;
		float t = 0.0f;
		CHECK_FALSE(CircleIntersect(box, VECTOR2D(10.0f, 10.0f), 0.5f, t, N));
	}
}

TEST_CASE("JOINT parameterised ctor populates members", "[physics][joint]")
{
	auto v1 = unit_square(0.0f, 0.0f);
	auto v2 = unit_square(2.0f, 0.0f);
	RIGIDBODY a(v1.data(), 4), b(v2.data(), 4);

	JOINT j(&a, &b, VECTOR2D(0.5f, 0.0f), VECTOR2D(-0.5f, 0.0f));

	CHECK(j.aBodies[0] == &a);
	CHECK(j.aBodies[1] == &b);
	CHECK(j.aPoints[0].x == Approx(0.5f));
	CHECK(j.aPoints[1].x == Approx(-0.5f));
}
