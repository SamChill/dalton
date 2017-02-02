#include <nanogui/screen.h>
#include <nanogui/glutil.h>
#include "AtomMatrix.h"

class GUI : public nanogui::Screen {
public:
    GUI();
    ~GUI();

    virtual void drawContents();
    virtual bool scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel);
    virtual bool mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers);

    void updatePositions();

private:
    nanogui::GLShader shader_;
    float radius_;
    float zoom_;
    int num_atoms_;
    Eigen::MatrixXf positions_;
    nanogui::Arcball arcball_;
    double render_time_;
    float gradient_;
};
