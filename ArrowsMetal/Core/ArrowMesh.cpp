#include "ArrowMesh.h"

namespace {
const double kTwoPi = 6.28318530717958647692;

struct Placer {
    CRotrix    R;
    CVector<3> color;

    SceneVertex make(const CVector<3>& localPos, const CVector<3>& localNormal) const {
        CVector<3> p = R * localPos;
        CVector<3> n = R * localNormal;
        SceneVertex v;
        v.position.x = (float)p[0]; v.position.y = (float)p[1]; v.position.z = (float)p[2];
        v.normal.x   = (float)n[0]; v.normal.y   = (float)n[1]; v.normal.z   = (float)n[2];
        v.color.x    = (float)color[0]; v.color.y = (float)color[1]; v.color.z = (float)color[2];
        v.uv.u = 0.0f; v.uv.v = 0.0f;
        return v;
    }
};

template <class NormalFn>
uint16_t ring(SceneMesh& m, const Placer& pl, const std::vector<CVector<2> >& circle,
              double z, double r, NormalFn normalOf) {
    uint16_t first = (uint16_t)m.vertices.size();
    for (size_t i = 0; i < circle.size(); ++i) {
        CVector<3> p(circle[i][0] * r, circle[i][1] * r, z);
        m.vertices.push_back(pl.make(p, normalOf((int)i)));
    }
    return first;
}

void quadStrip(SceneMesh& m, uint16_t a, uint16_t b, int S) {
    for (int i = 0; i < S; ++i) {
        uint16_t j = (uint16_t)((i + 1) % S);
        uint16_t a0 = (uint16_t)(a + i), a1 = (uint16_t)(a + j);
        uint16_t b0 = (uint16_t)(b + i), b1 = (uint16_t)(b + j);
        uint16_t t[6] = { a0, a1, b1, a0, b1, b0 };
        m.arrowTriangles.insert(m.arrowTriangles.end(), t, t + 6);
    }
}

void ringLines(SceneMesh& m, uint16_t a, int S) {
    for (int i = 0; i < S; ++i) {
        m.lines.push_back((uint16_t)(a + i));
        m.lines.push_back((uint16_t)(a + (i + 1) % S));
    }
}

struct Radial {
    const std::vector<CVector<2> >* c;
    CVector<3> operator()(int i) const { return CVector<3>((*c)[i][0], (*c)[i][1], 0.0); }
};

struct Constant {
    CVector<3> n;
    CVector<3> operator()(int) const { return n; }
};

struct ConeNormal {
    const std::vector<CVector<2> >* c;
    double slopeXY, slopeZ;
    CVector<3> operator()(int i) const {
        return CVector<3>((*c)[i][0] * slopeXY, (*c)[i][1] * slopeXY, slopeZ).normalized();
    }
};

}

void appendArrow(SceneMesh& m, const ArrowShape& s, const ArrowSpec& spec) {
    const int S = s.segments;
    Placer pl;
    pl.R     = CRotrix::between(CVector<3>(0, 0, 1), spec.direction);
    pl.color = spec.color;

    std::vector<CVector<2> > circle(S);
    for (int i = 0; i < S; ++i)
        circle[i] = CMatrix<2, 2>::rotation(kTwoPi * i / S) * CVector<2>(1.0, 0.0);

    const CVector<3> back(0, 0, -1);
    const double zs = s.shaftLength, zt = s.totalLength();

    Radial     radial = { &circle };
    Constant   backN  = { back };
    ConeNormal coneN  = { &circle, s.headLength, s.headRadius };

    uint16_t capCentre = (uint16_t)m.vertices.size();
    m.vertices.push_back(pl.make(CVector<3>(), back));
    uint16_t capRing = ring(m, pl, circle, 0.0, s.shaftRadius, backN);
    for (int i = 0; i < S; ++i) {
        uint16_t t[3] = { capCentre, (uint16_t)(capRing + (i + 1) % S), (uint16_t)(capRing + i) };
        m.arrowTriangles.insert(m.arrowTriangles.end(), t, t + 3);
    }

    uint16_t shaft0 = ring(m, pl, circle, 0.0, s.shaftRadius, radial);
    uint16_t shaft1 = ring(m, pl, circle, zs,  s.shaftRadius, radial);
    quadStrip(m, shaft0, shaft1, S);

    uint16_t under0 = ring(m, pl, circle, zs, s.shaftRadius, backN);
    uint16_t under1 = ring(m, pl, circle, zs, s.headRadius,  backN);
    quadStrip(m, under0, under1, S);

    uint16_t coneBase = ring(m, pl, circle, zs, s.headRadius, coneN);
    uint16_t tip = (uint16_t)m.vertices.size();
    for (int i = 0; i < S; ++i) {
        CVector<2> mid = (circle[i] + circle[(i + 1) % S]).normalized();
        CVector<3> n = CVector<3>(mid[0] * s.headLength, mid[1] * s.headLength, s.headRadius).normalized();
        m.vertices.push_back(pl.make(CVector<3>(0, 0, zt), n));
    }
    for (int i = 0; i < S; ++i) {
        uint16_t t[3] = { (uint16_t)(coneBase + i), (uint16_t)(coneBase + (i + 1) % S), (uint16_t)(tip + i) };
        m.arrowTriangles.insert(m.arrowTriangles.end(), t, t + 3);
    }

    ringLines(m, shaft0, S);
    ringLines(m, shaft1, S);
    ringLines(m, coneBase, S);
    for (int i = 0; i < S; i += 4) {
        uint16_t l[6] = { (uint16_t)(shaft0 + i),  (uint16_t)(shaft1 + i),
                          (uint16_t)(shaft1 + i),  (uint16_t)(coneBase + i),
                          (uint16_t)(coneBase + i),(uint16_t)(tip + i) };
        m.lines.insert(m.lines.end(), l, l + 6);
    }
}

void appendEarth(SceneMesh& m, const EarthShape& e) {
    const int first = (int)m.vertices.size();
    const int row   = e.slices + 1;
    const double pi = kTwoPi * 0.5;

    for (int i = 0; i <= e.stacks; ++i) {
        double lat = 0.5 * pi - pi * i / e.stacks;
        for (int j = 0; j <= e.slices; ++j) {
            double lon = -pi + kTwoPi * j / e.slices;
            CVector<3> n = e.direction(lat, lon);
            CVector<3> p = n * e.radius;
            SceneVertex v;
            v.position.x = (float)p[0]; v.position.y = (float)p[1]; v.position.z = (float)p[2];
            v.normal.x   = (float)n[0]; v.normal.y   = (float)n[1]; v.normal.z   = (float)n[2];
            v.color.x    = (float)e.color[0]; v.color.y = (float)e.color[1]; v.color.z = (float)e.color[2];
            v.uv.u = (float)j / e.slices;
            v.uv.v = (float)i / e.stacks;
            m.vertices.push_back(v);
        }
    }

    for (int i = 0; i < e.stacks; ++i) {
        for (int j = 0; j < e.slices; ++j) {
            uint16_t a = (uint16_t)(first + i * row + j);
            uint16_t b = (uint16_t)(a + row);
            if (i != e.stacks - 1) {
                uint16_t t[3] = { b, (uint16_t)(b + 1), a };
                m.earthTriangles.insert(m.earthTriangles.end(), t, t + 3);
            }
            if (i != 0) {
                uint16_t t[3] = { a, (uint16_t)(b + 1), (uint16_t)(a + 1) };
                m.earthTriangles.insert(m.earthTriangles.end(), t, t + 3);
            }
        }
    }

    for (int i = e.gridEvery; i < e.stacks; i += e.gridEvery)
        for (int j = 0; j < e.slices; ++j) {
            m.lines.push_back((uint16_t)(first + i * row + j));
            m.lines.push_back((uint16_t)(first + i * row + j + 1));
        }
    for (int j = 0; j < e.slices; j += e.gridEvery)
        for (int i = 0; i < e.stacks; ++i) {
            m.lines.push_back((uint16_t)(first + i * row + j));
            m.lines.push_back((uint16_t)(first + (i + 1) * row + j));
        }
}

SceneMesh buildScene(const ArrowShape& shape, const EarthShape& earth) {
    SceneMesh mesh;
    const ArrowSpec specs[3] = {
        { CVector<3>::unit(0), CVector<3>(0.90, 0.06, 0.05) },
        { CVector<3>::unit(1), CVector<3>(0.06, 0.80, 0.10) },
        { CVector<3>::unit(2), CVector<3>(0.02, 0.02, 0.02) },
    };
    for (int i = 0; i < 3; ++i) appendArrow(mesh, shape, specs[i]);
    mesh.arrowVertexCount = (int)mesh.vertices.size();
    appendEarth(mesh, earth);
    return mesh;
}
