#ifndef ARROWS_RIGID_TRANSFORM_H
#define ARROWS_RIGID_TRANSFORM_H

#include "CRotrix.h"

class RigidTransform {
public:
    CQuat      orientation;
    CVector<3> position;

    RigidTransform() : orientation(CQuat::identity()), position() {}

    void rotateWorld(const CVector<3>& axis, double radians) {
        orientation = (CQuat::fromAxisAngle(axis, radians) * orientation).normalized();
    }

    void rotateLocal(const CVector<3>& axis, double radians) {
        orientation = (orientation * CQuat::fromAxisAngle(axis, radians)).normalized();
    }

    void translate(const CVector<3>& delta) { position += delta; }

    void reset() { orientation = CQuat::identity(); position = CVector<3>(); }

    CRotrix rotation() const { return CRotrix(orientation); }

    CMatrix<4, 4> matrix() const {
        CRotrix R = rotation();
        return CMatrix<4, 4>(R[0].resized<4>(position[0]),
                             R[1].resized<4>(position[1]),
                             R[2].resized<4>(position[2]),
                             CVector<4>(0.0, 0.0, 0.0, 1.0));
    }
};

#endif
