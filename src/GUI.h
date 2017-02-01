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

private:
    nanogui::GLShader shader_;
    float radius_;
    nanogui::Arcball arcball_;
};
