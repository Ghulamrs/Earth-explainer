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
    CHECK(sizeof(SceneVertex) == 44);
    CHECK(sizeof(FrameUniforms) == 176);

    {
        RigidTransform T;
        T.rotateWorld(CVector<3>(1, 2, 3), 0.7);
        T.translate(CVector<3>(0.5, -1, 2));
        CVector<3> p(0.3, -0.4, 1.1);
        CHECK(near(transformPoint(T.matrix(), p), T.orientation.rotate(p) + T.position));
        CHECK(near(T.matrix()[3][3], 1.0));
    }

    {
        RigidTransform T;
        for (int i = 0; i < 100000; ++i) {
            T.rotateWorld(CVector<3>(1, 1, 0), 0.013);
            T.rotateLocal(CVector<3>(0, 0.3, 1), -0.007);
        }
        CHECK(T.rotation().isValid(1e-9));
        CHECK(near(T.orientation.norm(), 1.0, 1e-12));
    }

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

    {
        Scene s;
        s.setControl(RotXPos, true);
        s.setControl(RotYPos, true);
        s.update(0.25);
        CVector<3> expectedAxis = CVector<3>(1, 1, 0).normalized();
        CHECK(near(s.frame1().pose().orientation.axis(), expectedAxis, 1e-12));
        CHECK(near(s.frame1().pose().orientation.angle(), Scene::kRotateSpeed * 0.25, 1e-12));
        CHECK(s.needsFrames());
        s.clearControls();
        CHECK(!s.needsFrames());
    }

    {
        const double quarter = 3.14159265358979323846 / 2;
        Scene s;
        CHECK(s.isLocal());
        s.setControl(RotZPos, true);
        s.update(quarter / Scene::kRotateSpeed);
        s.setControl(RotZPos, false);
        CHECK(near(s.frame1().axis(0), CVector<3>(0, 1, 0), 1e-12));
        CVector<3> y1 = s.frame1().axis(1);
        s.setControl(RotXPos, true);
        s.update(0.5);
        CHECK(near(s.frame1().axis(0), CVector<3>(0, 1, 0), 1e-12));
        CHECK(!near(s.frame1().axis(1), y1, 1e-3));
        CHECK(near(dot(s.frame1().axis(1), s.frame1().axis(0)), 0.0, 1e-12));

        AxisFrame f(0.4);
        f.rotate(CVector<3>(0, 0, 1), quarter);
        f.move(CVector<3>(1, 0, 0));
        CHECK(near(f.origin(), CVector<3>(0, 1, 0), 1e-12));
        CHECK(near(transformPoint(f.drawMatrix(), CVector<3>(1, 0, 0)), CVector<3>(0, 1.4, 0), 1e-12));
        f.reset();
        CHECK(near(f.origin(), CVector<3>()) && near(f.axis(2), CVector<3>(0, 0, 1)));
    }

    {
        Scene s;
        EarthShape earth;
        const AxisFrame& f1 = s.frame1();
        const AxisFrame& f2 = s.frame2();
        CHECK(near(f2.origin(), earth.direction(0, 0) * Scene::kEarthRadius, 1e-12));
        CHECK(dot(f2.origin(), s.eye() - f2.origin()) > 0.0);
        CHECK(near(f2.scale(), Scene::kFrame2Scale));

        s.setControl(RotXPos, true); s.setControl(RotYNeg, true);
        s.update(0.7);
        s.clearControls();
        s.setControl(MoveZPos, true);
        s.update(0.4);
        s.clearControls();
        s.dragRotate(30, -12);

        CVector<3> contact = transformPoint(f1.matrix(), CVector<3>(0, 0, -Scene::kEarthRadius));
        CHECK(near(f2.origin(), contact, 1e-12));
        CHECK(near(distance(f2.origin(), f1.origin()), Scene::kEarthRadius, 1e-12));
        for (int i = 0; i < 3; ++i) CHECK(near(f2.axis(i), f1.axis(i), 1e-12));
        CHECK(near(transformPoint(f2.drawMatrix(), CVector<3>(0, 1, 0)), f2.origin() + f2.axis(1) * Scene::kFrame2Scale, 1e-12));

        s.reset();
        CHECK(near(f2.origin(), CVector<3>(0, 0, -Scene::kEarthRadius), 1e-12));

        SceneMesh m = buildScene();
        CHECK(m.arrowLineIndexCount > 0 && m.arrowLineIndexCount < (int)m.lines.size());
        for (int i = 0; i < m.arrowLineIndexCount; ++i) CHECK(m.lines[i] < m.arrowVertexCount);
    }

    {
        const double deg = 3.14159265358979323846 / 180.0;
        CQuat qx = CQuat::fromAxisAngle(CVector<3>(1, 0, 0),  74 * deg);
        CQuat qy = CQuat::fromAxisAngle(CVector<3>(0, 1, 0), -34 * deg);
        CQuat qz = CQuat::fromAxisAngle(CVector<3>(0, 0, 1),  35 * deg);
        CQuat expected[3] = { qx, qy * qx, qy * qx * qz };

        Scene s;
        CVector<3> contact = s.frame2().origin();
        CVector<3> where[3] = { qx.rotate(contact), (qy * qx).rotate(contact), (qy * qx).rotate(contact) };
        for (int k = 0; k < 3; ++k) {
            CHECK(s.turnFrame2());
            CHECK(!s.turnFrame2());
            CHECK(s.needsFrames());
            s.update(Scene::kTurnSeconds * 0.5);
            CHECK(s.isTurningFrame2());
            s.update(Scene::kTurnSeconds * 0.5);
            CHECK(!s.isTurningFrame2());
            CRotrix got(s.frame2().pose().orientation), want(expected[k]);
            for (int i = 0; i < 3; ++i) CHECK(near(got[i], want[i], 1e-12));
            CHECK(near(s.frame2().origin(), where[k], 1e-12));
            CHECK(near(distance(s.frame2().origin(), s.frame1().origin()), Scene::kEarthRadius, 1e-12));
        }
        CHECK(near(where[0], CVector<3>(0, std::sin(74 * deg), -std::cos(74 * deg)), 1e-12));
        CHECK(s.frame2TurnsStarted() == Scene::kFrame2TurnCount);
        CHECK(!s.turnFrame2());
        CHECK(!s.needsFrames());

        CVector<3> z2 = s.frame2().axis(2);
        CHECK(near(z2, CRotrix(qy * qx) * CVector<3>(0, 0, 1), 1e-12));
        s.reset();
        CHECK(near(s.frame2().axis(2), z2, 1e-12));
    }

    {
        Scene s;
        s.setControl(SelectX, true);
        CHECK(!s.needsFrames());
        s.setControl(SelectY, true);
        s.setControl(MoveXPos, true);
        CHECK(s.needsFrames());
        s.update(0.25);
        CHECK(near(s.frame1().pose().orientation.axis(), CVector<3>(1, 1, 0).normalized(), 1e-12));
        CHECK(near(s.frame1().pose().orientation.angle(), Scene::kRotateSpeed * 0.25, 1e-12));
        CHECK(near(s.frame1().pose().position, CVector<3>()));

        Scene t;
        t.setControl(SelectZ, true);
        t.setControl(MoveXNeg, true);
        t.update(0.25);
        CHECK(near(t.frame1().pose().orientation.axis(), CVector<3>(0, 0, -1), 1e-12));
        t.setControl(SelectZ, false);
        t.update(0.25);
        CHECK(near(t.frame1().origin(), t.frame1().axis(0) * (-Scene::kMoveSpeed * 0.25), 1e-12));
    }

    {
        Scene s;
        CHECK(near(s.solidAlpha(), 0.0));
        s.toggleSolid();
        CHECK(s.needsFrames());
        int frames = 0;
        while (s.update(1.0 / 60.0)) ++frames;
        CHECK(near(s.solidAlpha(), 1.0));
        CHECK(frames > 30 && frames < 40);
    }

    {
        CMatrix<4, 4> P = perspective(1.0, 1.5, 0.1, 100.0);
        CHECK(near(transformPoint(P, CVector<3>(0, 0, -0.1))[2], 0.0, 1e-12));
        CHECK(near(transformPoint(P, CVector<3>(0, 0, -100.0))[2], 1.0, 1e-9));
    }

    {
        CVector<3> eye(3, 4, 5);
        ViewBasis b;
        CMatrix<4, 4> V = lookAt(eye, CVector<3>(), CVector<3>(0, 1, 0), &b);
        CHECK(near(transformPoint(V, eye), CVector<3>()));
        CVector<3> t = transformPoint(V, CVector<3>());
        CHECK(near(t, CVector<3>(0, 0, -eye.length()), 1e-12));
        CHECK(near(dot(b.right, b.up), 0.0, 1e-12));
    }

    {
        CMatrix<4, 4> M;
        for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) M[r][c] = 10 * r + c;
        float f[16];
        toColumnMajor(M, f);
        CHECK(f[1] == 10.0f && f[4] == 1.0f && f[14] == 23.0f);
    }

    ArrowShape shape;
    EarthShape earth;
    SceneMesh m = buildScene(shape, earth);
    {
        CHECK(m.vertices.size() < 65536);
        CHECK(m.arrowTriangles.size() % 3 == 0 && m.earthTriangles.size() % 3 == 0 && m.lines.size() % 2 == 0);
        for (size_t i = 0; i < m.arrowTriangles.size(); ++i) CHECK(m.arrowTriangles[i] < m.arrowVertexCount);
        for (size_t i = 0; i < m.earthTriangles.size(); ++i)
            CHECK(m.earthTriangles[i] >= m.arrowVertexCount && m.earthTriangles[i] < m.vertices.size());
        for (size_t i = 0; i < m.lines.size(); ++i) CHECK(m.lines[i] < m.vertices.size());

        int perArrow = m.arrowVertexCount / 3;
        for (int a = 0; a < 3; ++a) {
            CVector<3> axis = CVector<3>::unit(a);
            double tipMax = 0.0, offAxisMax = 0.0;
            for (int i = a * perArrow; i < (a + 1) * perArrow; ++i) {
                const SceneVertex& v = m.vertices[i];
                CVector<3> p(v.position.x, v.position.y, v.position.z);
                CVector<3> n(v.normal.x, v.normal.y, v.normal.z);
                CHECK(near(n.length(), 1.0, 1e-5));
                CHECK(p.dot(axis) >= -1e-6);
                if (p.dot(axis) > tipMax) tipMax = p.dot(axis);
                double off = p.rejectedFrom(axis).length();
                if (off > offAxisMax) offAxisMax = off;
            }
            CHECK(near(tipMax, shape.totalLength(), 1e-5));
            CHECK(near(offAxisMax, shape.headRadius, 1e-5));
        }
        CHECK(shape.totalLength() > earth.radius * 2.0);
        CHECK(shape.shaftRadius <= 0.035 && shape.headRadius <= 0.09);
        CHECK(shape.headRadius > shape.shaftRadius * 2.0);
        const SceneVertex& zv = m.vertices[2 * perArrow];
        CHECK(zv.color.x < 0.05f && zv.color.y < 0.05f && zv.color.z < 0.05f);
        CHECK(m.vertices[0].color.x > 0.8f && m.vertices[perArrow].color.y > 0.7f);
    }

    {
        for (size_t i = m.arrowVertexCount; i < m.vertices.size(); ++i) {
            const SceneVertex& v = m.vertices[i];
            CVector<3> p(v.position.x, v.position.y, v.position.z);
            CVector<3> n(v.normal.x, v.normal.y, v.normal.z);
            CHECK(near(p.length(), earth.radius, 1e-5));
            CHECK(near(n.length(), 1.0, 1e-5) && near(p.normalized(), n, 1e-5));
            CHECK(v.uv.u >= 0.0f && v.uv.u <= 1.0f && v.uv.v >= 0.0f && v.uv.v <= 1.0f);
            CHECK(v.color.z > v.color.x && v.color.z > v.color.y);
        }
        const double d = 3.14159265358979323846 / 180.0;
        CHECK(near(earth.direction(90 * d, 0),     CVector<3>(1, 0, 0), 1e-12));
        CHECK(near(earth.direction(0, 0),          CVector<3>(0, 0, -1), 1e-12));
        CHECK(near(earth.direction(0, 90 * d),     CVector<3>(0, 1, 0), 1e-12));
        CHECK(near(earth.direction(0, 180 * d),    CVector<3>(0, 0, 1), 1e-12));
        const SceneVertex& np = m.vertices[m.arrowVertexCount];
        CHECK(near(np.position.x, earth.radius, 1e-6) && np.uv.v == 0.0f);
    }

    {
        const std::vector<uint16_t>* lists[2] = { &m.arrowTriangles, &m.earthTriangles };
        int inward = 0, degenerate = 0;
        for (int l = 0; l < 2; ++l)
            for (size_t i = 0; i < lists[l]->size(); i += 3) {
                const SceneVertex* v[3] = { &m.vertices[(*lists[l])[i]], &m.vertices[(*lists[l])[i + 1]], &m.vertices[(*lists[l])[i + 2]] };
                CVector<3> p0(v[0]->position.x, v[0]->position.y, v[0]->position.z);
                CVector<3> p1(v[1]->position.x, v[1]->position.y, v[1]->position.z);
                CVector<3> p2(v[2]->position.x, v[2]->position.y, v[2]->position.z);
                CVector<3> g = cross(p1 - p0, p2 - p0);
                if (g.length() < 1e-12) { ++degenerate; continue; }
                CVector<3> n = CVector<3>(v[0]->normal.x, v[0]->normal.y, v[0]->normal.z)
                             + CVector<3>(v[1]->normal.x, v[1]->normal.y, v[1]->normal.z)
                             + CVector<3>(v[2]->normal.x, v[2]->normal.y, v[2]->normal.z);
                if (g.dot(n) <= 0.0) ++inward;
            }
        CHECK(inward == 0);
        CHECK(degenerate == 0);
        std::printf("mesh: %d vertices (%d arrows), %d triangles, %d line segments\n",
                    (int)m.vertices.size(), m.arrowVertexCount,
                    (int)(m.arrowTriangles.size() + m.earthTriangles.size()) / 3, (int)m.lines.size() / 2);
    }

    {
        CHECK(near(cross(CVector<3>::unit(0), CVector<3>::unit(1)), CVector<3>::unit(2)));
        Scene s;
        const ViewBasis& b = s.cameraBasis();
        CHECK(b.up.dot(CVector<3>::unit(0))      > 0.9);
        CHECK(b.right.dot(CVector<3>::unit(1))   > 0.9);
        CHECK(b.forward.dot(CVector<3>::unit(2)) > 0.8);
        CHECK(near(cross(b.right, b.up), -b.forward, 1e-12));

        AxisLabel x = s.projectAxisLabel(0, 800, 600, false);
        AxisLabel y = s.projectAxisLabel(1, 800, 600, false);
        AxisLabel z = s.projectAxisLabel(2, 800, 600, false);
        CHECK(x.visible && y.visible && z.visible);
        CHECK(x.y > 450 && std::fabs(x.x - 400) < 150);
        CHECK(y.x > 550);
        CHECK(std::fabs(z.x - 400) < 200 && std::fabs(z.y - 300) < 200);

        Scene h;
        h.dragRotate(0.0, angleBetween(CVector<3>::unit(0), h.cameraBasis().forward) / 0.008);
        CHECK(near(h.frame1().axis(0), h.cameraBasis().forward, 1e-9));
        CHECK(h.projectAxisLabel(0, 800, 600, true).occluded);
        CHECK(!h.projectAxisLabel(0, 800, 600, false).occluded);
        CHECK(!s.projectAxisLabel(1, 800, 600, true).occluded);
    }

    std::printf(failures ? "%d FAILURE(S)\n" : "all core tests passed\n", failures);
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
