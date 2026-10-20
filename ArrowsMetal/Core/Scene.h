#ifndef ARROWS_SCENE_H
#define ARROWS_SCENE_H

#include "Projection.h"
#include "AxisFrame.h"

struct AxisLabel {
    double x, y;
    bool   visible;
    bool   occluded;
};

enum Control {
    RotXPos, RotXNeg, RotYPos, RotYNeg, RotZPos, RotZNeg,
    MoveXPos, MoveXNeg, MoveYPos, MoveYNeg, MoveZPos, MoveZNeg,
    SelectX, SelectY, SelectZ,
    ControlCount
};

class Scene {
public:
    static const double kRotateSpeed;
    static const double kMoveSpeed;
    static const double kMorphSeconds;
    static const double kEarthRadius;
    static const double kLabelDistance;

    Scene();

    void setControl(Control c, bool down) { controls_[c] = down; }
    void clearControls();
    void toggleSolid()      { solid_ = !solid_; }
    void toggleAxesFrame()  { local_ = !local_; }
    void reset();
    void zoom(double factor);
    void dragRotate(double dxPixels, double dyUpPixels);
    void setAspect(double aspect);

    bool update(double dt);
    bool needsFrames() const;

    CMatrix<4, 4>        modelMatrix() const { return frame1_.matrix(); }
    const CMatrix<4, 4>& viewProjection() const { return viewProj_; }
    const CVector<3>&    eye() const { return eye_; }
    double solidAlpha() const { return morph_; }
    double wireAlpha()  const { return 1.0 - morph_; }
    bool   isSolid()    const { return solid_; }
    bool   isLocal()    const { return local_; }
    const AxisFrame&      frame1() const { return frame1_; }
    const ViewBasis&      cameraBasis() const { return basis_; }

    AxisLabel projectAxisLabel(int axis, double viewWidth, double viewHeight, bool earthOpaque) const;

private:
    AxisFrame      frame1_;
    bool           controls_[ControlCount];
    bool           solid_, local_;
    double         morph_;
    double         distance_, aspect_;
    CVector<3>     viewDir_;
    CVector<3>     eye_;
    ViewBasis      basis_;
    CMatrix<4, 4>  viewProj_;

    void rebuildCamera();
    void motion(CVector<3>& spin, CVector<3>& move) const;
    static double axisInput(bool pos, bool neg) { return (pos ? 1.0 : 0.0) - (neg ? 1.0 : 0.0); }
};

#endif
