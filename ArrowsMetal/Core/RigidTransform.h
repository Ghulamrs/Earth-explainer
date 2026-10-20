// Position + orientation of the triad. Orientation is a CQuat, so rotations
// about any axis, or any combination of axes, compose without drift or
// gimbal lock; CRotrix turns it into the 3x3 part of the model matrix.
#ifndef ARROWS_RIGID_TRANSFORM_H
#define ARROWS_RIGID_TRANSFORM_H

#include "CRotrix.h"

class RigidTransform {
public:
    CQuat      orientation;
    CVector<3> position;

    RigidTransform() : orientation(CQuat::identity()), position() {}

    // About a world-fixed axis: q' = dq * q
    void rotateWorld(const CVector<3>& axis, double radians) {
        orientation = (CQuat::fromAxisAngle(axis, radians) * orientation).normalized();
    }

    // About the object's own (body) axis: q' = q * dq
    void rotateLocal(const CVector<3>& axis, double radians) {
        orientation = (orientation * CQuat::fromAxisAngle(axis, radians)).normalized();
    }

    void translate(const CVector<3>& delta) { position += delta; }

    void reset() { orientation = CQuat::identity(); position = CVector<3>(); }

    CRotrix rotation() const { return CRotrix(orientation); }

    // [ R | t ]
    // [ 0 | 1 ]   -- each row is a rotation row padded with one translation component.
    CMatrix<4, 4> matrix() const {
        CRotrix R = rotation();
        return CMatrix<4, 4>(R[0].resized<4>(position[0]),
                             R[1].resized<4>(position[1]),
                             R[2].resized<4>(position[2]),
                             CVector<4>(0.0, 0.0, 0.0, 1.0));
    }
};

#endif
