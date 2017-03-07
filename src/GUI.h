#include <memory>
#include <functional>

#include <nanogui/nanogui.h>
#include <nanogui/screen.h>
#include <nanogui/glutil.h>
#include <nanogui/label.h>

#include "AnalyticRenderer.h"
#include "Atoms.h"
#include "PathTracingRenderer.h"
#include "Utils.h"


class GUI : public nanogui::Screen {
public:
    GUI();
    void setXYZPath(std::string path);
    virtual void drawContents();
    virtual bool scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel);
    virtual bool mouseButtonEvent(const Eigen::Vector2i &p, int button, bool down, int modifiers);
    virtual bool mouseMotionEvent(const Eigen::Vector2i &p,
                                  const Eigen::Vector2i &rel,
                                  int button,
                                  int modifiers);
    virtual bool resizeEvent(const Eigen::Vector2i &size);

private:
    enum RenderMethod { Analytic, PathTracing };
    void setRenderMethod(RenderMethod render_method);
    RenderMethod render_method_;
    std::vector<Atoms> trajectory_;
    float ambient_occlusion_;
    float decay_;
    float radius_scale_;
    float saturation_;
    float zoom_;
    float outline_;
    float eta_;
    int neighbor_count_;
    nanogui::Arcball arcball_;
    nanogui::Label *frame_label_;
    nanogui::Label *fps_label_;
    nanogui::Label *render_time_label_;
    nanogui::Label *samples_label_;
    nanogui::Button *play_button_;
    nanogui::ComboBox *render_method_box_;
    nanogui::IntBox<int> *neighbor_count_box_;
    nanogui::Slider *movie_slider_;
    PerformanceMonitor performance_monitor_;
    PathTracingRenderer path_tracing_renderer_;
    AnalyticRenderer analytic_renderer_;
    bool playing_;
    int frame_;
    bool clear_before_render_;
    int max_bounces_;
    float shininess_;
    Eigen::Vector4f background_color_;
    float focal_distance_;
    float focal_strength_;
    float ambient_light_;
    float direct_light_;
    Eigen::Vector2i initial_mouse_coordinates_, shift_;
};
