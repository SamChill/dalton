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
    ambient_occlusion_(1.0),
    saturation_(1.0),
    frame_(0)
{
    // Setup Widgets.
    FormHelper *gui = new FormHelper(this);
    ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "Controls");

    FloatBox<float> *radius_scale_box = gui->addVariable("radius", radius_scale_);
    radius_scale_box->setSpinnable(true);
    radius_scale_box->setMinValue(0.001);
    radius_scale_box->setValueIncrement(0.1);

    FloatBox<float> *ambient_occlusion_box = gui->addVariable("ambient occlusion", ambient_occlusion_);
    ambient_occlusion_box->setSpinnable(true);
    ambient_occlusion_box->setMinValue(0.00);
    ambient_occlusion_box->setValueIncrement(0.1);

    FloatBox<float> *saturation_box = gui->addVariable("saturation", saturation_);
    saturation_box->setSpinnable(true);
    saturation_box->setMinValue(0.00);
    saturation_box->setMaxValue(1.00);
    saturation_box->setValueIncrement(0.1);

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

    // Texture.
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

    GLint sphere_texture_loc = shader_.uniform("sphere_texture");
    GLint radius_texture_loc = shader_.uniform("radius_texture");

    glUniform1i(sphere_texture_loc, 0);
    glUniform1i(radius_texture_loc, 1);

    glActiveTexture(GL_TEXTURE0 + 0);
    glBindTexture(GL_TEXTURE_1D, sphere_texture);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_1D, radius_texture);

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
