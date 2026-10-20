#include "Scene.h"

const double Scene::kRotateSpeed  = 1.6;
const double Scene::kMoveSpeed    = 1.5;
const double Scene::kMorphSeconds = 0.6;

Scene::Scene()
    : solid_(false), local_(false), morph_(0.0),
      distance_(6.5), aspect_(1.0),
      viewDir_(CVector<3>(1.0, 0.75, 1.3).normalized()) {
    clearControls();
    rebuildCamera();
}

void Scene::clearControls() {
    for (int i = 0; i < ControlCount; ++i) controls_[i] = false;
}

void Scene::reset() {
    model_.reset();
    distance_ = 6.5;
    rebuildCamera();
}

void Scene::zoom(double factor) {
    distance_ *= factor;
    if (distance_ < 2.5)  distance_ = 2.5;
    if (distance_ > 40.0) distance_ = 40.0;
    rebuildCamera();
}

void Scene::setAspect(double aspect) {
    if (aspect <= 0.0 || aspect == aspect_) return;
    aspect_ = aspect;
    rebuildCamera();
}

void Scene::rebuildCamera() {
    eye_ = viewDir_ * distance_;
    CMatrix<4, 4> V = lookAt(eye_, CVector<3>(), CVector<3>(0, 1, 0), &basis_);
    CMatrix<4, 4> P = perspective(45.0 * 3.14159265358979323846 / 180.0, aspect_, 0.1, 100.0);
    viewProj_ = P * V;
}

// Mouse drag = rotation about an arbitrary axis lying in the screen plane,
// perpendicular to the drag direction (a virtual trackball).
void Scene::dragRotate(double dx, double dyUp) {
    CVector<3> axis = basis_.right * (-dyUp) + basis_.up * dx;
    double len = axis.length();
    if (len == 0.0) return;
    model_.rotateWorld(axis, len * 0.008);
}

bool Scene::update(double dt) {
    // Any combination of held rotate keys yields one axis: e.g. X+Y -> (1,1,0).
    CVector<3> spin(axisInput(controls_[RotXPos], controls_[RotXNeg]),
                    axisInput(controls_[RotYPos], controls_[RotYNeg]),
                    axisInput(controls_[RotZPos], controls_[RotZNeg]));
    CVector<3> move(axisInput(controls_[MoveXPos], controls_[MoveXNeg]),
                    axisInput(controls_[MoveYPos], controls_[MoveYNeg]),
                    axisInput(controls_[MoveZPos], controls_[MoveZNeg]));

    if (!spin.isZero()) {
        double angle = kRotateSpeed * dt;
        if (local_) model_.rotateLocal(spin, angle);
        else        model_.rotateWorld(spin, angle);
    }
    if (!move.isZero()) {
        CVector<3> step = move.normalized() * (kMoveSpeed * dt);
        model_.translate(local_ ? model_.rotation() * step : step);
    }

    double target = solid_ ? 1.0 : 0.0;
    double rate = dt / kMorphSeconds;
    if (morph_ < target)      morph_ = (morph_ + rate > target) ? target : morph_ + rate;
    else if (morph_ > target) morph_ = (morph_ - rate < target) ? target : morph_ - rate;

    return needsFrames();
}

bool Scene::needsFrames() const {
    for (int i = 0; i < ControlCount; ++i)
        if (controls_[i]) return true;
    return morph_ != (solid_ ? 1.0 : 0.0);
}
