#ifndef ARROWS_AXIS_FRAME_H
#define ARROWS_AXIS_FRAME_H

#include "RigidTransform.h"

class AxisFrame {
public:
    explicit AxisFrame(double scale = 1.0) : scale_(scale), parent_(0) {}

    void attachTo(const AxisFrame& parent, const CVector<3>& at) {
        parent_ = &parent;
        home_   = RigidTransform();
        home_.position = at;
        pose_   = home_;
    }

    void rotate(const CVector<3>& axis, double radians)         { pose_.rotateLocal(axis, radians); }
    void rotateInParent(const CVector<3>& axis, double radians) { pose_.rotateWorld(axis, radians); }
    void move(const CVector<3>& delta)                          { pose_.translate(pose_.rotation() * delta); }
    void moveInParent(const CVector<3>& delta)                  { pose_.translate(delta); }
    void reset()                                                { pose_ = home_; }

    const RigidTransform& pose() const { return pose_; }
    double scale() const { return scale_; }

    CMatrix<4, 4> matrix() const { return parent_ ? parent_->matrix() * pose_.matrix() : pose_.matrix(); }

    CMatrix<4, 4> drawMatrix() const {
        CMatrix<4, 4> M = matrix();
        for (int i = 0; i < 3; ++i) M.setCol(i, M.col(i) * scale_);
        return M;
    }

    CVector<3> origin() const     { return matrix().col(3).head<3>(); }
    CVector<3> axis(int i) const  { return matrix().col(i).head<3>(); }

private:
    RigidTransform pose_, home_;
    double         scale_;
    const AxisFrame* parent_;
};

#endif
