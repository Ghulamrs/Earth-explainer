// Geometry for three arrows whose tails meet at the origin.
// Built once at start-up; the GPU keeps it for the life of the app.
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
    int    segments;      // around the circumference

    ArrowShape()
        : shaftRadius(0.05), shaftLength(1.6),
          headRadius(0.13), headLength(0.4), segments(32) {}

    double totalLength() const { return shaftLength + headLength; }
};

struct ArrowSpec {
    CVector<3> direction;   // where the arrow points (need not be unit)
    CVector<3> color;       // linear RGB
};

struct TriadMesh {
    std::vector<ArrowVertex> vertices;      // shared by both index lists
    std::vector<uint16_t>    triangles;     // solid
    std::vector<uint16_t>    lines;         // wireframe
};

// Appends one arrow to the mesh.
void appendArrow(TriadMesh& mesh, const ArrowShape& shape, const ArrowSpec& spec);

// Red along +X, green along +Y, blue along +Z.
TriadMesh buildTriad(const ArrowShape& shape = ArrowShape());

#endif
