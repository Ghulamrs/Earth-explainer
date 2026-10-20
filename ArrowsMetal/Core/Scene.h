// Everything the app knows about the world, in plain C++ (no Cocoa, no Metal),
// so it can be unit-tested with any compiler.
#ifndef ARROWS_SCENE_H
#define ARROWS_SCENE_H

#include "Projection.h"
#include "RigidTransform.h"

enum Control {
    RotXPos, RotXNeg, RotYPos, RotYNeg, RotZPos, RotZNeg,
    MoveXPos, MoveXNeg, MoveYPos, MoveYNeg, MoveZPos, MoveZNeg,
    ControlCount
};

class Scene {
public:
    static const double kRotateSpeed;    // rad/s while a rotate key is held
    static const double kMoveSpeed;      // units/s while a move key is held
    static const double kMorphSeconds;   // wire <-> solid transition time

    Scene();

    // Input
    void setControl(Control c, bool down) { controls_[c] = down; }
    void clearControls();
    void toggleSolid()      { solid_ = !solid_; }
    void toggleAxesFrame()  { local_ = !local_; }
    void reset();
    void zoom(double factor);
    void dragRotate(double dxPixels, double dyUpPixels);
    void setAspect(double aspect);

    // Advances by dt seconds. Returns true while more frames are needed
    // (a key is held or the wire/solid morph is still running).
    bool update(double dt);
    bool needsFrames() const;

    // Output
    CMatrix<4, 4>        modelMatrix() const { return model_.matrix(); }
    const CMatrix<4, 4>& viewProjection() const { return viewProj_; }
    const CVector<3>&    eye() const { return eye_; }
    double solidAlpha() const { return morph_; }
    double wireAlpha()  const { return 1.0 - morph_; }
    bool   isSolid()    const { return solid_; }
    bool   isLocal()    const { return local_; }
    const RigidTransform& transform() const { return model_; }

private:
    RigidTransform model_;
    bool           controls_[ControlCount];
    bool           solid_, local_;
    double         morph_;          // 0 = wireframe, 1 = solid metal
    double         distance_, aspect_;
    CVector<3>     viewDir_;        // unit, from the origin towards the camera
    CVector<3>     eye_;
    ViewBasis      basis_;
    CMatrix<4, 4>  viewProj_;       // cached; rebuilt only on zoom / resize

    void rebuildCamera();
    static double axisInput(bool pos, bool neg) { return (pos ? 1.0 : 0.0) - (neg ? 1.0 : 0.0); }
};

#endif
