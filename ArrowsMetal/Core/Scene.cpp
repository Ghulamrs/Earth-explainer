#include "Scene.h"
#include "ArrowMesh.h"

const double Scene::kRotateSpeed  = 1.6;
const double Scene::kMoveSpeed    = 1.5;
const double Scene::kMorphSeconds = 0.6;
const double Scene::kEarthRadius   = 1.0;
const double Scene::kLabelDistance = ArrowShape().totalLength() + 0.22;
const double Scene::kFrame2Scale   = 0.4;
const double Scene::kTurnSeconds   = 1.2;

namespace {
enum TurnMode { OwnAxis, ParentAxisThroughOrigin };

struct Turn {
    int        frame;
    TurnMode   mode;
    CVector<3> axis;
    double     degrees;
};

const Turn kFrame2Turns[Scene::kFrame2TurnCount] = {
    { 1, OwnAxis,                 CVector<3>(1, 0, 0),  74.0 },
    { 2, ParentAxisThroughOrigin, CVector<3>(0, 1, 0), -34.0 },
    { 2, OwnAxis,                 CVector<3>(0, 0, 1),  35.0 },
};

double radiansOf(const Turn& t) { return t.degrees * 3.14159265358979323846 / 180.0; }
}


static const CVector<3> kViewerUp(1, 0, 0);

Scene::Scene()
    : frame2_(kFrame2Scale), solid_(false), local_(true), morph_(0.0),
      distance_(7.5), aspect_(1.0),
      viewDir_(CVector<3>(0.35, -0.40, -1.0).normalized()),
      turnsStarted_(0), turning_(false), turnProgress_(0.0), turnEased_(0.0) {
    frame2_.attachTo(frame1_, CVector<3>(0.0, 0.0, -kEarthRadius));
    clearControls();
    rebuildCamera();
}

void Scene::clearControls() {
    for (int i = 0; i < ControlCount; ++i) controls_[i] = false;
}

void Scene::reset() {
    if (turning_) advanceTurn(kTurnSeconds);
    frame1_.reset();
    for (int k = 0; k < turnsStarted_; ++k)
        if (kFrame2Turns[k].frame == 1) frame1_.rotate(kFrame2Turns[k].axis, radiansOf(kFrame2Turns[k]));
    distance_ = 7.5;
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
    CMatrix<4, 4> V = lookAt(eye_, CVector<3>(), kViewerUp, &basis_);
    CMatrix<4, 4> P = perspective(45.0 * 3.14159265358979323846 / 180.0, aspect_, 0.1, 100.0);
    viewProj_ = P * V;
}

void Scene::dragRotate(double dx, double dyUp) {
    CVector<3> axis = basis_.right * (-dyUp) + basis_.up * dx;
    double len = axis.length();
    if (len == 0.0) return;
    frame1_.rotateInParent(axis, len * 0.008);
}

void Scene::motion(CVector<3>& spin, CVector<3>& move) const {
    spin = CVector<3>(axisInput(controls_[RotXPos], controls_[RotXNeg]),
                      axisInput(controls_[RotYPos], controls_[RotYNeg]),
                      axisInput(controls_[RotZPos], controls_[RotZNeg]));
    move = CVector<3>(axisInput(controls_[MoveXPos], controls_[MoveXNeg]),
                      axisInput(controls_[MoveYPos], controls_[MoveYNeg]),
                      axisInput(controls_[MoveZPos], controls_[MoveZNeg]));

    CVector<3> selected(controls_[SelectX] ? 1.0 : 0.0,
                        controls_[SelectY] ? 1.0 : 0.0,
                        controls_[SelectZ] ? 1.0 : 0.0);
    if (!selected.isZero()) {
        spin += selected * move[0];
        move = CVector<3>();
    }
}

bool Scene::turnFrame2() {
    if (turning_ || turnsStarted_ == kFrame2TurnCount) return false;
    turnEased_    = 0.0;
    turnProgress_ = 0.0;
    turning_      = true;
    ++turnsStarted_;
    return true;
}

void Scene::advanceTurn(double dt) {
    if (!turning_) return;
    turnProgress_ += dt / kTurnSeconds;
    if (turnProgress_ >= 1.0) { turnProgress_ = 1.0; turning_ = false; }
    double e = turnProgress_ * turnProgress_ * (3.0 - 2.0 * turnProgress_);
    const Turn& t = kFrame2Turns[turnsStarted_ - 1];
    double step = radiansOf(t) * (e - turnEased_);
    turnEased_ = e;
    AxisFrame& f = t.frame == 1 ? frame1_ : frame2_;
    if (t.mode == OwnAxis) f.rotate(t.axis, step);
    else                   f.orbit(t.axis, step);
}

bool Scene::update(double dt) {
    CVector<3> spin, move;
    motion(spin, move);
    advanceTurn(dt);

    if (!spin.isZero()) {
        double angle = kRotateSpeed * dt;
        if (local_) frame1_.rotate(spin, angle);
        else        frame1_.rotateInParent(spin, angle);
    }
    if (!move.isZero()) {
        CVector<3> step = move.normalized() * (kMoveSpeed * dt);
        if (local_) frame1_.move(step);
        else        frame1_.moveInParent(step);
    }

    double target = solid_ ? 1.0 : 0.0;
    double rate = dt / kMorphSeconds;
    if (morph_ < target)      morph_ = (morph_ + rate > target) ? target : morph_ + rate;
    else if (morph_ > target) morph_ = (morph_ - rate < target) ? target : morph_ - rate;

    return needsFrames();
}

bool Scene::needsFrames() const {
    CVector<3> spin, move;
    motion(spin, move);
    return turning_ || !spin.isZero() || !move.isZero() || morph_ != (solid_ ? 1.0 : 0.0);
}

AxisLabel Scene::projectAxisLabel(int axis, double w, double h, bool earthOpaque) const {
    AxisLabel out = { 0.0, 0.0, false, false };
    CMatrix<4, 4> M = frame1_.matrix();
    CVector<4> world4 = M * (CVector<3>::unit(axis) * kLabelDistance).resized<4>(1.0);
    CVector<3> world  = world4.head<3>();

    CVector<4> clip = viewProj_ * world4;
    if (clip[3] <= 1e-9) return out;
    out.visible = true;
    out.x = (clip[0] / clip[3] + 1.0) * 0.5 * w;
    out.y = (clip[1] / clip[3] + 1.0) * 0.5 * h;

    if (earthOpaque) {
        CVector<3> d = world - eye_;
        double len = d.length();
        CVector<3> dir = d / len;
        CVector<3> oc  = eye_ - frame1_.origin();
        double b    = oc.dot(dir);
        double disc = b * b - (oc.lengthSquared() - kEarthRadius * kEarthRadius);
        if (disc > 0.0) {
            double t = -b - std::sqrt(disc);
            out.occluded = t > 0.0 && t < len;
        }
    }
    return out;
}
