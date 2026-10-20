// Checks the maths and geometry behind the app; no Cocoa or Metal needed.
#include "ArrowMesh.h"
#include "Scene.h"

#include <cstdio>
#include <cstdlib>

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

static bool near(double a, double b, double tol = 1e-9) { return std::fabs(a - b) <= tol; }
template <int N> static bool near(const CVector<N>& a, const CVector<N>& b, double tol = 1e-9) {
    return (a - b).length() <= tol;
}
static CVector<3> transformPoint(const CMatrix<4, 4>& M, const CVector<3>& p) {
    CVector<4> h = M * p.resized<4>(1.0);
    return h.head<3>() / h[3];
}

int main() {
    // Shared layout must match Metal's.
    CHECK(sizeof(ArrowVertex) == 36);
    CHECK(sizeof(FrameUniforms) == 176);

    // Model matrix = [R|t], and agrees with the quaternion's own rotate().
    {
        RigidTransform T;
        T.rotateWorld(CVector<3>(1, 2, 3), 0.7);
        T.translate(CVector<3>(0.5, -1, 2));
        CVector<3> p(0.3, -0.4, 1.1);
        CHECK(near(transformPoint(T.matrix(), p), T.orientation.rotate(p) + T.position));
        CHECK(near(T.matrix()[3][3], 1.0));
    }

    // 100 000 small mixed rotations stay a proper rotation (no drift).
    {
        RigidTransform T;
        for (int i = 0; i < 100000; ++i) {
            T.rotateWorld(CVector<3>(1, 1, 0), 0.013);
            T.rotateLocal(CVector<3>(0, 0.3, 1), -0.007);
        }
        CHECK(T.rotation().isValid(1e-9));
        CHECK(near(T.orientation.norm(), 1.0, 1e-12));
    }

    // Local rotation about a == world rotation about R*a.
    {
        RigidTransform A, B;
        A.rotateWorld(CVector<3>(0.2, 1, -0.5), 1.1);
        B = A;
        CVector<3> axis(1, 0, 0);
        A.rotateLocal(axis, 0.4);
        B.rotateWorld(B.rotation() * axis, 0.4);
        CHECK(near(A.rotation().matrix[0], B.rotation().matrix[0], 1e-12));
        CHECK(near(A.rotation().matrix[1], B.rotation().matrix[1], 1e-12));
    }

    // Holding X and Y together rotates about (1,1,0).
    {
        Scene s;
        s.setControl(RotXPos, true);
        s.setControl(RotYPos, true);
        s.update(0.25);
        CVector<3> expectedAxis = CVector<3>(1, 1, 0).normalized();
        CHECK(near(s.transform().orientation.axis(), expectedAxis, 1e-12));
        CHECK(near(s.transform().orientation.angle(), Scene::kRotateSpeed * 0.25, 1e-12));
        CHECK(s.needsFrames());
        s.clearControls();
        CHECK(!s.needsFrames());
    }

    // Wire -> solid morph runs, then the scene goes idle (no redraws).
    {
        Scene s;
        CHECK(near(s.solidAlpha(), 0.0));
        s.toggleSolid();
        CHECK(s.needsFrames());
        int frames = 0;
        while (s.update(1.0 / 60.0)) ++frames;
        CHECK(near(s.solidAlpha(), 1.0));
        CHECK(frames > 30 && frames < 40);   // ~0.6 s at 60 Hz
    }

    // Projection: near plane -> depth 0, far plane -> depth 1 (Metal convention).
    {
        CMatrix<4, 4> P = perspective(1.0, 1.5, 0.1, 100.0);
        CHECK(near(transformPoint(P, CVector<3>(0, 0, -0.1))[2], 0.0, 1e-12));
        CHECK(near(transformPoint(P, CVector<3>(0, 0, -100.0))[2], 1.0, 1e-9));
    }

    // View: eye maps to the origin, target lies straight ahead on -Z.
    {
        CVector<3> eye(3, 4, 5);
        ViewBasis b;
        CMatrix<4, 4> V = lookAt(eye, CVector<3>(), CVector<3>(0, 1, 0), &b);
        CHECK(near(transformPoint(V, eye), CVector<3>()));
        CVector<3> t = transformPoint(V, CVector<3>());
        CHECK(near(t, CVector<3>(0, 0, -eye.length()), 1e-12));
        CHECK(near(dot(b.right, b.up), 0.0, 1e-12));
    }

    // Column-major conversion: element (row r, col c) lands at c*4 + r.
    {
        CMatrix<4, 4> M;
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) M[r][c] = 10 * r + c;
        float f[16];
        toColumnMajor(M, f);
        CHECK(f[1] == 10.0f && f[4] == 1.0f && f[14] == 23.0f);
    }

    // Mesh: three arrows, tails at origin, tips on the axes, sane normals/indices.
    {
        ArrowShape shape;
        TriadMesh m = buildTriad(shape);
        CHECK(m.vertices.size() < 65536);
        CHECK(m.triangles.size() % 3 == 0 && m.lines.size() % 2 == 0);
        for (size_t i = 0; i < m.triangles.size(); ++i) CHECK(m.triangles[i] < m.vertices.size());
        for (size_t i = 0; i < m.lines.size(); ++i)     CHECK(m.lines[i] < m.vertices.size());

        int perArrow = (int)m.vertices.size() / 3;
        for (int a = 0; a < 3; ++a) {
            CVector<3> axis = CVector<3>::unit(a);
            double tipMax = 0.0, offAxisMax = 0.0;
            for (int i = a * perArrow; i < (a + 1) * perArrow; ++i) {
                const ArrowVertex& v = m.vertices[i];
                CVector<3> p(v.position.x, v.position.y, v.position.z);
                CVector<3> n(v.normal.x, v.normal.y, v.normal.z);
                CHECK(near(n.length(), 1.0, 1e-5));
                CHECK(p.dot(axis) >= -1e-6);                 // nothing behind the tail
                if (p.dot(axis) > tipMax) tipMax = p.dot(axis);
                double off = p.rejectedFrom(axis).length();
                if (off > offAxisMax) offAxisMax = off;
                CHECK(v.color.x + v.color.y + v.color.z > 0.5f);
            }
            CHECK(near(tipMax, shape.totalLength(), 1e-5));
            CHECK(near(offAxisMax, shape.headRadius, 1e-5));
        }

        // Every solid triangle faces outward (counter-clockwise from outside):
        // its geometric normal agrees with the stored vertex normals.
        int inward = 0;
        for (size_t i = 0; i < m.triangles.size(); i += 3) {
            const ArrowVertex* v[3] = { &m.vertices[m.triangles[i]], &m.vertices[m.triangles[i + 1]], &m.vertices[m.triangles[i + 2]] };
            CVector<3> p0(v[0]->position.x, v[0]->position.y, v[0]->position.z);
            CVector<3> p1(v[1]->position.x, v[1]->position.y, v[1]->position.z);
            CVector<3> p2(v[2]->position.x, v[2]->position.y, v[2]->position.z);
            CVector<3> g = cross(p1 - p0, p2 - p0);
            CVector<3> n = CVector<3>(v[0]->normal.x, v[0]->normal.y, v[0]->normal.z)
                         + CVector<3>(v[1]->normal.x, v[1]->normal.y, v[1]->normal.z)
                         + CVector<3>(v[2]->normal.x, v[2]->normal.y, v[2]->normal.z);
            if (g.dot(n) <= 0.0) ++inward;
        }
        CHECK(inward == 0);
        std::printf("mesh: %d vertices, %d triangles, %d line segments\n",
                    (int)m.vertices.size(), (int)m.triangles.size() / 3, (int)m.lines.size() / 2);
    }

    std::printf(failures ? "%d FAILURE(S)\n" : "all core tests passed\n", failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
