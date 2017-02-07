#include "GUI.h"
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
    arcball_(2.0f),
    radius_scale_(1.0),
    zoom_(0.0f),
    render_time_(glfwGetTime()),
    gradient_(0.5),
    frame_(0)
{
    // Setup Widgets.
    FormHelper *gui = new FormHelper(this);
    ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "Controls");

    FloatBox<float> *radius_scale_box = gui->addVariable("radius", radius_scale_);
    radius_scale_box->setSpinnable(true);
    radius_scale_box->setMinValue(0.001);
    radius_scale_box->setValueIncrement(0.02);

    FloatBox<float> *gradient_box = gui->addVariable("gradient", gradient_);
    gradient_box->setSpinnable(true);
    gradient_box->setMinValue(0.00);
    gradient_box->setValueIncrement(0.05);

    fps_label_ = new Label(window, "0.0");
    gui->addWidget("fps", fps_label_);

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
    shader_.setUniform("gradient", gradient_);
    shader_.setUniform("radius_scale", radius_scale_);
    shader_.setUniform("box_size", box_size_);

    // Draw points.
    glEnable(GL_DEPTH_TEST);
    shader_.drawArray(GL_POINTS, 0, sphere_centers.rows());
    glDisable(GL_DEPTH_TEST);

    // Calculate fps.
    frame_ += 1;
    if (glfwGetTime() - render_time_ > 1.0) {
        double fps = frame_ / (glfwGetTime() - render_time_);
        render_time_ = glfwGetTime();
        fps_label_->setCaption(std::to_string(fps).substr(0,5));
        frame_ = 0;
    }
}
