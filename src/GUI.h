#include <nanogui/screen.h>
#include <nanogui/glutil.h>
#include <nanogui/label.h>
#include "Atoms.h"
#include "Utils.h"

class GUI : public nanogui::Screen {
public:
    GUI(std::string filename);
    ~GUI();

    virtual void drawContents();
    virtual bool scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel);
    virtual bool mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers);

private:
    Atoms atoms_;
    float ambient_occlusion_;
    float box_size_;
    float decay_;
    float radius_scale_;
    float saturation_;
    float zoom_;
    float outline_;
    float eta_;
    int neighbor_count_;
    nanogui::Arcball arcball_;
    nanogui::GLShader shader_;
    nanogui::Label *fps_label_;
    nanogui::Label *render_time_label_;
    PerformanceMonitor performance_monitor_;
};
