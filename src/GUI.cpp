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

// Includes for the GLTexture class.
#include <cstdint>
#include <memory>
#include <utility>

using std::string;
using namespace nanogui;


GUI::GUI(std::string filename) :
    nanogui::Screen(Eigen::Vector2i(1024, 768), "Dalton"),
    performance_monitor_(0.25),
    arcball_(2.0f),
    radius_scale_(1.0),
    zoom_(0.0f),
    saturation_(0.6),
    outline_(0.0),
    eta_(0.0),
    ambient_occlusion_(1.0),
    decay_(1.5)
{
    // Setup Widgets.
    FormHelper *gui = new FormHelper(this);
    ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "");

    gui->addGroup("Geometry");
    FloatBox<float> *radius_scale_box = gui->addVariable("radius", radius_scale_);
    radius_scale_box->setSpinnable(true);
    radius_scale_box->setMinValue(0.001);
    radius_scale_box->setValueIncrement(0.1);

    gui->addGroup("Presets");
    ComboBox *presets_box = new ComboBox(
        window,
        {"Custom", "2D", "Cell Shading", "Ambient Occlusion (AO)", "AO + Outlines", "B&W"}
    );
    presets_box->setFontSize(16);
    gui->addWidget("", presets_box);

    gui->addGroup("Colors");
    FloatBox<float> *saturation_box = gui->addVariable("saturation", saturation_);
    saturation_box->setSpinnable(true);
    saturation_box->setMinValue(0.00);
    saturation_box->setMaxValue(1.00);
    saturation_box->setValueIncrement(0.1);

    Color default_bg = Color(0.841803f, 1.0f);
    setBackground(default_bg);
    ColorPicker *bg_color_picker = new ColorPicker(window, default_bg);
    bg_color_picker->setCallback(
        [this](Color c) {
            setBackground(c);
        }
    );
    gui->addWidget("background", bg_color_picker);

    gui->addGroup("Outlines");
    FloatBox<float> *outline_box = gui->addVariable("thickness", outline_);
    outline_box->setSpinnable(true);
    outline_box->setMinValue(0.00);
    outline_box->setMaxValue(1.00);
    outline_box->setValueIncrement(0.05);

    FloatBox<float> *eta_box = gui->addVariable("depth hinting", eta_);
    eta_box->setSpinnable(true);
    eta_box->setMinValue(0.00);
    eta_box->setValueIncrement(0.2);

    gui->addGroup("Ambient Occlusion");
    FloatBox<float> *ambient_occlusion_box = gui->addVariable("strength", ambient_occlusion_);
    ambient_occlusion_box->setSpinnable(true);
    ambient_occlusion_box->setMinValue(0.00);
    ambient_occlusion_box->setValueIncrement(0.1);

    FloatBox<float> *decay_box = gui->addVariable("decay", decay_);
    decay_box->setSpinnable(true);
    decay_box->setMinValue(0.1);
    decay_box->setMaxValue(5.00);
    decay_box->setValueIncrement(0.1);

    gui->addGroup("Performance");
    fps_label_ = new Label(window, "0.0");
    gui->addWidget("fps", fps_label_);

    render_time_label_ = new Label(window, "0.0");
    gui->addWidget("render time (ms)", render_time_label_);

    presets_box->setCallback(
        [this,saturation_box,outline_box,eta_box,ambient_occlusion_box,decay_box](int i) {
            // 2D
            switch (i) {
                case 0: //Custom;
                    break;
                case 1: //2D
                    saturation_ = 1.0;
                    outline_ = 0.25;
                    eta_ = 1.8;
                    ambient_occlusion_ = 0.0;
                    decay_ = 2.0;
                    break;
                case 2: //Cell shading
                    saturation_ = 0.4;
                    outline_ = 0.25;
                    eta_ = 2.6;
                    ambient_occlusion_ = 0.2;
                    decay_ = 0.5;
                    break;
                case 3: // AO
                    saturation_ = 0.6;
                    outline_ = 0.0;
                    eta_ = 0.0;
                    ambient_occlusion_ = 1.0;
                    decay_ = 1.5;
                    break;
                case 4: // AO + depth
                    saturation_ = 0.6;
                    outline_ = 0.3;
                    eta_ = 3.5;
                    ambient_occlusion_ = 1.0;
                    decay_ = 1.5;
                    break;
                case 5: // B&W
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

            saturation_box->setEnabled(i==0);
            outline_box->setEnabled(i==0);
            eta_box->setEnabled(i==0);
            ambient_occlusion_box->setEnabled(i==0);
            decay_box->setEnabled(i==0);
    });
    presets_box->setSelectedIndex(3);

    // Finalize widget setup.
    performLayout();

    // Load and initialzer shaders.
    Resource vertexShader   = LOAD_RESOURCE(vertex_glsl);
    Resource fragmentShader = LOAD_RESOURCE(fragment_glsl);
    Resource geometryShader = LOAD_RESOURCE(geometry_glsl);
    shader_.init(
        "scene",
        string(vertexShader.data(), vertexShader.size()),
        string(fragmentShader.data(), fragmentShader.size()),
        string(geometryShader.data(), geometryShader.size())
    );

    shader_.bind();

    atoms_ = Atoms::readXYZ(filename);
    AtomMatrix coordinates = atoms_.coordinates();
    coordinates.rowwise() -= (coordinates.colwise().sum() / ((float) atoms_.size())).eval();
    box_size_ = (coordinates.colwise().maxCoeff() - coordinates.colwise().minCoeff()).maxCoeff();
    box_size_ *= 1.5;
    atoms_.setCoordinates(coordinates);
    shader_.uploadAttrib("radius", atoms_.radii().transpose());
    shader_.uploadAttrib("sphere_color", atoms_.colors().transpose());

    // Texture.
    AtomMatrix sphere_centers = atoms_.coordinates();
    GLuint sphere_texture;
    glGenTextures(1, &sphere_texture);
    glBindTexture(GL_TEXTURE_1D, sphere_texture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RGB32F,
        sphere_centers.rows(),
        0,
        GL_RGB,
        GL_FLOAT,
        sphere_centers.transpose().data()
    );

    GLuint radius_texture;
    glGenTextures(1, &radius_texture);
    glBindTexture(GL_TEXTURE_1D, radius_texture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F,
        atoms_.radii().rows(),
        0,
        GL_RED,
        GL_FLOAT,
        atoms_.radii().data()
    );

    int max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    neighbor_count_ = std::min(max_texture_size / atoms_.size(), atoms_.size()-1);
    neighbor_count_ = std::min(40, neighbor_count_);

    NeighborList neighbor_list;
    if (neighbor_count_ < 8 && atoms_.size() > 9) {
        neighbor_count_ = 0;
        std::cerr << "warning: max texture size too small: ao disabled" << std::endl;
    }else{
        neighbor_list = atoms_.neighborList(neighbor_count_);
    }

    shader_.setUniform("neighbor_count", neighbor_count_);
    GLuint neighbor_texture;
    glGenTextures(1, &neighbor_texture);
    glBindTexture(GL_TEXTURE_1D, neighbor_texture);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F,
        neighbor_list.cols() * neighbor_list.rows(),
        0,
        GL_RED,
        GL_FLOAT,
        neighbor_list.transpose().data()
    );

    shader_.setUniform("sphere_texture", 0);
    shader_.setUniform("radius_texture", 1);
    shader_.setUniform("neighbor_texture", 2);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_1D, sphere_texture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_1D, radius_texture);
    glActiveTexture(GL_TEXTURE0 + 2);
    glBindTexture(GL_TEXTURE_1D, neighbor_texture);
}

GUI::~GUI() {
    shader_.free();
}

bool GUI::scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel) {
    if (!nanogui::Screen::scrollEvent(p, rel)) {
        zoom_ -= 0.1*rel.y();
    }
}

bool GUI::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    if (!nanogui::Screen::mouseButtonEvent(p, button, down, modifiers)) {
        if (button == 0) {
            arcball_.button(p, down);
        }
    }
}

void GUI::drawContents() {
    shader_.bind();

    // Get sphere centers.
    AtomMatrix sphere_centers = atoms_.coordinates();
    shader_.uploadAttrib("sphere_center", sphere_centers.transpose());

    Eigen::VectorXf sphere_numbers = Eigen::VectorXf::Zero(atoms_.size());
    for (int i=0; i<atoms_.size(); i++) {
        sphere_numbers(i) = float(i);
    }
    shader_.uploadAttrib("sphere_number", sphere_numbers.transpose());

    // Make perspective matrix.
    float aspect_ratio = float(mSize.x()) / float(mSize.y());
    float zoom = std::exp(zoom_);
    Matrix4f projection = ortho(
        -box_size_/2.0*zoom*aspect_ratio, box_size_/2.0*zoom*aspect_ratio,
        -box_size_/2.0*zoom, box_size_/2.0*zoom,
        -box_size_/2.0, box_size_/2.0);
    shader_.setUniform("projection", projection);

    // Make view matrix.
    arcball_.setSize(mSize);

    arcball_.motion(mousePos());
    Matrix4f view = arcball_.matrix();
    shader_.setUniform("view", view);

    // Update uniforms.
    shader_.setUniform("num_atoms", (int)atoms_.size());
    shader_.setUniform("ambient_occlusion", ambient_occlusion_);
    shader_.setUniform("radius_scale", radius_scale_);
    shader_.setUniform("box_size", box_size_);
    shader_.setUniform("saturation", saturation_);
    shader_.setUniform("outline", outline_);
    shader_.setUniform("decay", decay_);
    shader_.setUniform("eta", std::exp(eta_));

    // Draw points.
    glEnable(GL_DEPTH_TEST);
    shader_.drawArray(GL_POINTS, 0, sphere_centers.rows());
    glDisable(GL_DEPTH_TEST);

    // Calculate fps.
    performance_monitor_.update();
    fps_label_->setCaption(std::to_string(performance_monitor_.fps()).substr(0,5));
    render_time_label_->setCaption(std::to_string(performance_monitor_.renderTime()).substr(0,5));

}
