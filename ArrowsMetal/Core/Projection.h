// Camera matrices built only from CVector / CMatrix.
#ifndef ARROWS_PROJECTION_H
#define ARROWS_PROJECTION_H

#include "CMatrix.h"

struct ViewBasis {
    CVector<3> right, up, forward;
};

// Right-handed, camera looks down -Z, Metal clip depth in [0, 1].
inline CMatrix<4, 4> perspective(double fovyRadians, double aspect, double zNear, double zFar) {
    double f = 1.0 / std::tan(fovyRadians * 0.5);
    double d = zNear - zFar;
    return CMatrix<4, 4>(CVector<4>(f / aspect, 0.0, 0.0,       0.0),
                         CVector<4>(0.0,        f,   0.0,       0.0),
                         CVector<4>(0.0,        0.0, zFar / d,  zNear * zFar / d),
                         CVector<4>(0.0,        0.0, -1.0,      0.0));
}

// Each row of the view matrix is a basis vector padded with its translation:
// CVector<3>::resized<4>(pad) does exactly that in one call.
inline CMatrix<4, 4> lookAt(const CVector<3>& eye, const CVector<3>& target,
                            const CVector<3>& upHint, ViewBasis* basis = 0) {
    CVector<3> f = (target - eye).normalized();
    CVector<3> s = cross(f, upHint).normalized();
    CVector<3> u = cross(s, f);
    if (basis) { basis->right = s; basis->up = u; basis->forward = f; }
    return CMatrix<4, 4>(s.resized<4>(-s.dot(eye)),
                         u.resized<4>(-u.dot(eye)),
                         (-f).resized<4>(f.dot(eye)),
                         CVector<4>(0.0, 0.0, 0.0, 1.0));
}

// CMatrix is row-of-CVector, double; the GPU wants 16 column-major floats.
inline void toColumnMajor(const CMatrix<4, 4>& m, float out[16]) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            out[c * 4 + r] = static_cast<float>(m[r][c]);
}

#endif
