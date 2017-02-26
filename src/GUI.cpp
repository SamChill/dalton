#include "GUI.h"
#include <algorithm>
#include <unistd.h>
#include <nanogui/screen.h>
#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/nanogui.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <iostream>
#include <string>
#include <Resource.h>
#include <future>

// Includes for the GLTexture class.
#include <cstdint>
#include <memory>
#include <utility>

using std::string;
using namespace nanogui;


GUI::GUI() :
    nanogui::Screen(Eigen::Vector2i(1024, 768), "Dalton"),
    performance_monitor_(1.00),
    arcball_(2.0f),
    radius_scale_(1.0),
    zoom_(0.0f),
    saturation_(0.65),
    outline_(0.0),
    eta_(0.0),
    ambient_occlusion_(1.0),
    decay_(2.0),
    neighbor_count_(0),
    playing_(false),
    frame_(0),
    render_method_(Analytic),
    path_tracing_renderer_(mSize),
    analytic_renderer_(mSize),
    clear_before_render_(false),
    max_bounces_(1),
    shininess_(0.0),
    background_color_(.4, .4, .4, 1.0),
    focal_distance_(0.0),
    focal_strength_(0.0),
    ambient_light_(1.0),
    direct_light_(0.0)
{
    // hack to support high-dpi displays.
    resizeEvent(mSize);
    trajectory_ = {Atoms()};
    analytic_renderer_.setAtoms(trajectory_[0]);
    path_tracing_renderer_.setAtoms(trajectory_[0]);

    // Setup Widgets.
    FormHelper *gui = new FormHelper(this);
    ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "");

    gui->addGroup("File");
    gui->addButton("Open", [this]() {
        std::string path = file_dialog({{"xyz", "xyz file format"}}, false);
        setXYZPath(path);
    });

    gui->addGroup("Rendering Method");
    render_method_box_ = new ComboBox(window, {"Analytic", "Path Tracing"});
    render_method_box_->setFontSize(16);
    gui->addWidget("", render_method_box_);

    //gui->addGroup("Movie Controls");
    //frame_label_ = new Label(window, "1/1");
    //gui->addWidget("", frame_label_);
    //movie_slider_ = new Slider(window);
    //movie_slider_->setRange({0.0, 0.0});
    //movie_slider_->setCallback(
    //    [this](float frame) {
    //        frame_ = int(frame);
    //    }
    //);
    //gui->addWidget("", movie_slider_);
    //Widget *tools = new Widget(window);
    //tools->setLayout(new BoxLayout(Orientation::Horizontal,
    //                               Alignment::Middle, 0, 6));

    //auto b = new Button(tools, "", ENTYPO_ICON_FB);
    //b->setCallback(
    //    [this, b]() {
    //        playing_ = false;
    //        frame_ = 0;
    //        play_button_->setPushed(false);
    //    }
    //);

    //play_button_ = new Button(tools, "", ENTYPO_ICON_PLAY);
    //play_button_->setChangeCallback(
    //    [this, b](bool enabled) {
    //        playing_ = enabled;
    //    }
    //);
    //play_button_->setFlags(nanogui::Button::Flags::ToggleButton);

    //b = new Button(tools, "", ENTYPO_ICON_FF);
    //b->setCallback(
    //    [this]() {
    //        playing_ = false;
    //        frame_ = scene_.trajectoryLength();
    //        play_button_->setPushed(false);
    //    }
    //);
    //gui->addWidget("", tools);

    gui->addGroup("Atom Properties");
    FloatBox<float> *radius_scale_box = gui->addVariable("radius", radius_scale_);
    radius_scale_box->setSpinnable(true);
    radius_scale_box->setMinValue(0.001);
    radius_scale_box->setValueIncrement(0.05);
    radius_scale_box->setCallback(
        [this](float r) {
            radius_scale_ = r;
            clear_before_render_ = true;
        }
    );

    FloatBox<float> *saturation_box = gui->addVariable("color saturation", saturation_);
    saturation_box->setSpinnable(true);
    saturation_box->setMinValue(0.00);
    saturation_box->setMaxValue(1.00);
    saturation_box->setValueIncrement(0.05);
    saturation_box->setCallback(
        [this](float s) {
            saturation_ = s;
            clear_before_render_ = true;
        }
    );

    ColorPicker *bg_color_picker = new ColorPicker(window, background_color_);
    setBackground(background_color_);
    bg_color_picker->setCallback(
        [this](Color c) {
            setBackground(c);
            clear_before_render_ = true;
            background_color_ = c;
        }
    );
    gui->addWidget("background", bg_color_picker);

    gui->addGroup("Analytic Options");
    ComboBox *analytic_presets_box = new ComboBox(
        window,
        {
            "2D",
            "Cell Shading",
            "Ambient Occlusion",
            "AO + Outlines",
            "B&W",
        },
        {
            "2D",
            "CS",
            "AO",
            "AO+OL",
            "B&W",
        }
    );
    analytic_presets_box->setFontSize(16);
    gui->addWidget("preset", analytic_presets_box);

    FloatBox<float> *outline_box = gui->addVariable("outline width", outline_);
    outline_box->setSpinnable(true);
    outline_box->setMinValue(0.00);
    outline_box->setMaxValue(1.00);
    outline_box->setValueIncrement(0.05);

    FloatBox<float> *eta_box = gui->addVariable("outline depth", eta_);
    eta_box->setSpinnable(true);
    eta_box->setMinValue(0.00);
    eta_box->setValueIncrement(0.2);


    neighbor_count_box_ = gui->addVariable("neighbour count", neighbor_count_);
    neighbor_count_box_->setSpinnable(true);
    neighbor_count_box_->setMinValue(0);

    FloatBox<float> *ambient_occlusion_box = gui->addVariable("ao strength", ambient_occlusion_);
    ambient_occlusion_box->setSpinnable(true);
    ambient_occlusion_box->setMinValue(0.00);
    ambient_occlusion_box->setValueIncrement(0.1);

    FloatBox<float> *decay_box = gui->addVariable("ao decay", decay_);
    decay_box->setSpinnable(true);
    decay_box->setMinValue(0.1);
    decay_box->setMaxValue(5.00);
    decay_box->setValueIncrement(0.1);

    gui->addGroup("Path Tracing Options");
    ComboBox *path_tracing_presets_box = new ComboBox(
        window,
        {
            "Ambient Occlusion",
            "Shiny",
            "Plastic",
        },
        {
            "AO",
            "Shiny",
            "Plastic",
        }
    );
    path_tracing_presets_box->setFontSize(16);
    gui->addWidget("preset", path_tracing_presets_box);

    ComboBox *resolution_factor_box = new ComboBox(
        window,
        {"High", "Medium", "Low"},
        {"High", "Med.", "Low"}
    );
    resolution_factor_box->setFontSize(16);
    gui->addWidget("quality", resolution_factor_box);
    resolution_factor_box->setCallback(
        [this](int i) {
            path_tracing_renderer_.setResolutionFactor(i+1);
        }
    );

    nanogui::IntBox<int> *max_bounces_box = gui->addVariable("max bounces", max_bounces_);
    max_bounces_box->setSpinnable(true);
    max_bounces_box->setMinValue(0);
    max_bounces_box->setCallback(
        [this](int i) {
            max_bounces_ = i;
            clear_before_render_ = true;
        }
    );

    FloatBox<float> *shininess_box = gui->addVariable("shininess", shininess_);
    shininess_box->setSpinnable(true);
    shininess_box->setMinValue(0.00);
    shininess_box->setMaxValue(1.00);
    shininess_box->setValueIncrement(0.05);
    shininess_box->setCallback(
        [this](float s) {
            shininess_ = s;
            clear_before_render_ = true;
        }
    );
    FloatBox<float> *ambient_light_box = gui->addVariable("ambient light", ambient_light_);
    ambient_light_box->setSpinnable(true);
    ambient_light_box->setMinValue(0.0);
    ambient_light_box->setMaxValue(1.0);
    ambient_light_box->setValueIncrement(0.05);
    ambient_light_box->setCallback(
        [this](float f) {
            ambient_light_ = f;
            clear_before_render_ = true;
        }
    );

    FloatBox<float> *direct_light_box = gui->addVariable("direct light", direct_light_);
    direct_light_box->setSpinnable(true);
    direct_light_box->setMinValue(0.0);
    direct_light_box->setMaxValue(10.0);
    direct_light_box->setValueIncrement(0.2);
    direct_light_box->setCallback(
        [this](float f) {
            direct_light_ = f;
            clear_before_render_ = true;
        }
    );

    FloatBox<float> *focal_distance_box = gui->addVariable("focal distance", focal_distance_);
    focal_distance_box->setSpinnable(true);
    focal_distance_box->setMinValue(-100.00);
    focal_distance_box->setMaxValue(100.00);
    focal_distance_box->setValueIncrement(0.5);
    focal_distance_box->setCallback(
        [this](float f) {
            focal_distance_ = f;
            clear_before_render_ = true;
        }
    );
    FloatBox<float> *focal_strength_box = gui->addVariable("focal blur", focal_strength_);
    focal_strength_box->setSpinnable(true);
    focal_strength_box->setMinValue(0.0);
    focal_strength_box->setMaxValue(1.00);
    focal_strength_box->setValueIncrement(0.01);
    focal_strength_box->setCallback(
        [this](float f) {
            focal_strength_ = f;
            clear_before_render_ = true;
        }
    );


    gui->addGroup("Performance");
    fps_label_ = new Label(window, "0.0");
    gui->addWidget("fps", fps_label_);

    render_time_label_ = new Label(window, "0.0");
    gui->addWidget("render time (ms)", render_time_label_);

    samples_label_ = new Label(window, "0");
    gui->addWidget("monte carlo samples", samples_label_);

    analytic_presets_box->setCallback(
        [=](int i) {
            // 2D
            switch (i) {
                case 0: //2D
                    saturation_ = 1.0;
                    outline_ = 0.25;
                    eta_ = 1.8;
                    ambient_occlusion_ = 0.0;
                    decay_ = 2.0;
                    break;
                case 1: //Cell shading
                    saturation_ = 0.6;
                    outline_ = 0.25;
                    eta_ = 2.6;
                    ambient_occlusion_ = 0.2;
                    decay_ = 0.5;
                    break;
                case 2: // AO
                    saturation_ = 0.6;
                    outline_ = 0.0;
                    eta_ = 0.0;
                    ambient_occlusion_ = 1.0;
                    decay_ = 2.0;
                    break;
                case 3: // AO + depth
                    saturation_ = 0.6;
                    outline_ = 0.3;
                    eta_ = 3.5;
                    ambient_occlusion_ = 1.0;
                    decay_ = 1.5;
                    break;
                case 4: // B&W
                    saturation_ = 0.0;
                    outline_ = 0.3;
                    eta_ = 4.3;
                    ambient_occlusion_ = 1.1;
                    decay_ = 1.7;
                    break;
            }
            saturation_box->setValue(saturation_);
            outline_box->setValue(outline_);
            eta_box->setValue(eta_);
            ambient_occlusion_box->setValue(ambient_occlusion_);
            decay_box->setValue(decay_);
    });
    analytic_presets_box->setSelectedIndex(2);

    path_tracing_presets_box->setCallback(
        [=](int i) {
            switch (i) {
                case 0: //AO
                    max_bounces_ = 1;
                    shininess_ = 0.0;
                    ambient_light_ = 1.0;
                    direct_light_ = 0.0;
                    break;
                case 1: //Shiny
                    max_bounces_ = 4;
                    shininess_ = 0.55;
                    ambient_light_ = 0.05;
                    direct_light_ = 2.0;
                    break;
                case 2: //Plastic
                    max_bounces_ = 2;
                    shininess_ = 0.6;
                    ambient_light_ = 0.85;
                    direct_light_ = 1.2;
                    break;
            }
            clear_before_render_ = true;
            max_bounces_box->setValue(max_bounces_);
            shininess_box->setValue(shininess_);
            ambient_light_box->setValue(ambient_light_);
            direct_light_box->setValue(direct_light_);
        }
    );

    render_method_box_->setCallback(
        [=](int i) {
            RenderMethod render_method = (RenderMethod) i;
            bool analytic = render_method == Analytic;
            bool path_tracing = render_method == PathTracing;

            analytic_presets_box->setEnabled(analytic);
            neighbor_count_box_->setEnabled(analytic);
            outline_box->setEnabled(analytic);
            eta_box->setEnabled(analytic);
            ambient_occlusion_box->setEnabled(analytic);
            decay_box->setEnabled(analytic);

            path_tracing_presets_box->setEnabled(path_tracing);
            resolution_factor_box->setEnabled(path_tracing);
            max_bounces_box->setEnabled(path_tracing);
            shininess_box->setEnabled(path_tracing);
            focal_distance_box->setEnabled(path_tracing);
            focal_strength_box->setEnabled(path_tracing);
            ambient_light_box->setEnabled(path_tracing);
            direct_light_box->setEnabled(path_tracing);

            setRenderMethod(render_method);
        }
    );
    path_tracing_presets_box->setEnabled(false);
    resolution_factor_box->setEnabled(false);
    max_bounces_box->setEnabled(false);
    shininess_box->setEnabled(false);
    focal_distance_box->setEnabled(false);
    focal_strength_box->setEnabled(false);
    ambient_light_box->setEnabled(false);
    direct_light_box->setEnabled(false);


    // Finalize widget setup.
    performLayout();
}

void GUI::setRenderMethod(RenderMethod render_method) {
    if (render_method_ == render_method) {
        return;
    }
    render_method_ = render_method;
    path_tracing_renderer_.setAtoms(trajectory_[0]);
    analytic_renderer_.setAtoms(trajectory_[0]);
}


bool GUI::resizeEvent(const Eigen::Vector2i &size) {
    path_tracing_renderer_.resize(size*mPixelRatio);
}

bool GUI::scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel) {
    if (!nanogui::Screen::scrollEvent(p, rel)) {
        zoom_ -= 0.1*rel.y();
        clear_before_render_ = true;
        return true;
    }
    return false;
}

bool GUI::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    if (!nanogui::Screen::mouseButtonEvent(p, button, down, modifiers)) {
        if (button == 0) {
            arcball_.button(p, down);
            return true;
        }
    }
    return false;
}

bool GUI::mouseMotionEvent(const Eigen::Vector2i &p,
                           const Eigen::Vector2i &rel,
                           int button,
                           int modifiers)
{
    if (button == 1 && (rel(0) !=0 || rel(1) != 0)) {
        clear_before_render_ = true;
    }
}

void GUI::setXYZPath(std::string path)
{
    trajectory_ = Atoms::readXYZ(path);
    analytic_renderer_.setAtoms(trajectory_[0]);
    path_tracing_renderer_.setAtoms(trajectory_[0]);
    int max_neighbors = trajectory_[0].size()-1;
    neighbor_count_box_->setMaxValue(max_neighbors);
    neighbor_count_ = std::min(32, max_neighbors);
    neighbor_count_box_->setValue(neighbor_count_);
    //movie_slider_->setRange({0, scene_.trajectoryLength()-1});
}

void GUI::drawContents() {
    //if (playing_) {
    //    frame_ += 1;
    //}
    //if (frame_ >= scene_.trajectoryLength()) {
    //    playing_ = false;
    //    frame_ = scene_.trajectoryLength() - 1;
    //    play_button_->setPushed(false);
    //}
    //frame_label_->setCaption(
    //    "Frame " + std::to_string(frame_ + 1) + "/" + std::to_string(scene_.trajectoryLength()));
    //scene_.setTrajectoryIndex(frame_);
    //movie_slider_->setValue(scene_.trajectoryIndex());
    //neighbor_count_box_->setMaxValue(scene_.maxNeighbors());

    // Make projection matrix.
    float aspect_ratio = float(mSize.x()) / float(mSize.y());
    float exp_zoom = std::exp(zoom_);
    Eigen::Matrix4f projection = nanogui::ortho(
        -exp_zoom*aspect_ratio, exp_zoom*aspect_ratio,
        -exp_zoom, exp_zoom,
        -100.0, 100.0);

    // Make view matrix.
    arcball_.setSize(mSize);
    arcball_.motion(mousePos());
    Matrix4f view = arcball_.matrix();

    // render the scene.
    if (render_method_ == Analytic || mMouseState == 2) {
        int nc = neighbor_count_;
        if (mMouseState == 2) nc = 200;
        analytic_renderer_.render(
            projection,
            view,
            radius_scale_,
            saturation_,
            background_color_,
            outline_,
            eta_,
            nc,
            ambient_occlusion_,
            decay_);
    }else{
        if (clear_before_render_) {
            path_tracing_renderer_.clear();
        }
        clear_before_render_ = false;

        path_tracing_renderer_.render(
            projection,
            view,
            radius_scale_,
            saturation_,
            background_color_,
            shininess_,
            max_bounces_,
            focal_distance_,
            focal_strength_,
            ambient_light_,
            direct_light_,
            &analytic_renderer_
        );
    }

    // Calculate fps.
    performance_monitor_.update();
    fps_label_->setCaption(std::to_string(performance_monitor_.fps()).substr(0,5));
    render_time_label_->setCaption(std::to_string(performance_monitor_.renderTime()).substr(0,5));
    samples_label_->setCaption(std::to_string(path_tracing_renderer_.samples));
}
