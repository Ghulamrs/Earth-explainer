#ifndef ARROWS_ARROW_MESH_H
#define ARROWS_ARROW_MESH_H

#include "../ShaderTypes.h"
#include "CRotrix.h"

#include <vector>

struct ArrowShape {
    double shaftRadius;
    double shaftLength;
    double headRadius;
    double headLength;
    int    segments;

    ArrowShape()
        : shaftRadius(0.032), shaftLength(1.95),
          headRadius(0.085), headLength(0.45), segments(32) {}

    double totalLength() const { return shaftLength + headLength; }
};

struct ArrowSpec {
    CVector<3> direction;
    CVector<3> color;
};

struct EarthShape {
    double     radius;
    int        stacks;
    int        slices;
    int        gridEvery;
    CVector<3> north;
    CVector<3> greenwich;
    CVector<3> color;

    EarthShape()
        : radius(1.0), stacks(48), slices(96), gridEvery(4),
          north(1, 0, 0), greenwich(0, 0, -1), color(0.10, 0.35, 1.0) {}

    CVector<3> direction(double latitude, double longitude) const {
        CVector<3> onEquator = CRotrix::axisAngle(north, longitude) * greenwich;
        return north * std::sin(latitude) + onEquator * std::cos(latitude);
    }
};

struct SceneMesh {
    std::vector<SceneVertex> vertices;
    std::vector<uint16_t>    arrowTriangles;
    std::vector<uint16_t>    earthTriangles;
    std::vector<uint16_t>    lines;
    int                      arrowVertexCount;
};

void appendArrow(SceneMesh& mesh, const ArrowShape& shape, const ArrowSpec& spec);
void appendEarth(SceneMesh& mesh, const EarthShape& shape);

SceneMesh buildScene(const ArrowShape& arrows = ArrowShape(), const EarthShape& earth = EarthShape());

#endif
