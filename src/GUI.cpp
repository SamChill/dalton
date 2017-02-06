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

GUI::GUI() :
    nanogui::Screen(Eigen::Vector2i(1024, 768), "Dalton"),
    arcball_(2.0f),
    radius_(0.56123f),
    zoom_(0.0f),
    num_atoms_(13),
    render_time_(glfwGetTime()),
    gradient_(0.5),
    frame_(0)
{
    // Setup Widgets.
    FormHelper *gui = new FormHelper(this);
    ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "Controls");

    FloatBox<float> *radius_box = gui->addVariable("radius", radius_);
    radius_box->setSpinnable(true);
    radius_box->setMinValue(0.001);
    radius_box->setValueIncrement(0.02);

    FloatBox<float> *gradient_box = gui->addVariable("gradient", gradient_);
    gradient_box->setSpinnable(true);
    gradient_box->setMinValue(0.00);
    gradient_box->setValueIncrement(0.05);

    IntBox<int> *num_atoms_box = gui->addVariable("number of atoms", num_atoms_);
    num_atoms_box->setSpinnable(true);
    num_atoms_box->setValueIncrement(10);
    num_atoms_box->setMinValue(1);

    fps_label_ = new Label(window, "0.0");
    gui->addWidget("fps", fps_label_);

    energy_label_ = new Label(window, "0.0");
    gui->addWidget("energy", energy_label_);

    force_label_ = new Label(window, "0.0");
    gui->addWidget("force", force_label_);

    gui->addButton("Generate", [this]() { updatePositions(); });

    gui->addButton("Save", [this]() { atoms_.xyz("/home/chill/dalton.xyz"); });

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
    updatePositions();
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

void GUI::updatePositions()
{
    atoms_.setCoordinates(2*AtomMatrix::Random(num_atoms_, 3));
}

void GUI::drawContents() {
    shader_.bind();

    AtomMatrix sphere_centers = atoms_.coordinates();

    shader_.uploadAttrib("sphere_center", sphere_centers.transpose());

    // Make perspective matrix.
    float aspect_ratio = float(mSize.x()) / float(mSize.y());
    float zoom = std::exp(zoom_);
    Matrix4f projection = ortho(-10.0*zoom*aspect_ratio, 10.0*zoom*aspect_ratio, -10.0*zoom, 10.0*zoom, -10.0, 10.0);
    shader_.setUniform("projection", projection);

    // Setup camera matrix.
    arcball_.setSize(mSize);
    arcball_.motion(mousePos());
    Matrix4f view = arcball_.matrix();
    //Matrix4f zoom = scale(Eigen::Vector3f(std::exp(zoom_), std::exp(zoom_), 1));
    shader_.setUniform("view", view);

    // Update uniforms.
    shader_.setUniform("radius", radius_);
    shader_.setUniform("gradient", gradient_);

    // Draw points.
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader_.drawArray(GL_POINTS, 0, sphere_centers.rows());
    glDisable(GL_DEPTH_TEST);


    {
        float alpha = 1/1000.0;
        AtomMatrix step = alpha * atoms_.forces();
        if (step.norm() > 0.2) {
            step = step.normalized() * 0.2;
        }
        AtomMatrix new_sphere_centers = sphere_centers + step;
        atoms_.setCoordinates(new_sphere_centers);
    }

    energy_label_->setCaption(std::to_string(atoms_.energy()).substr(0,7));
    force_label_->setCaption(std::to_string(atoms_.forces().norm()).substr(0,7));

    frame_ += 1;
    if (glfwGetTime() - render_time_ > 1.0) {
        double fps = frame_ / (glfwGetTime() - render_time_);
        render_time_ = glfwGetTime();
        fps_label_->setCaption(std::to_string(fps).substr(0,5));
        frame_ = 0;
    }
}
