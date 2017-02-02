#include "GUI.h"

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
    radius_(1.00f),
    zoom_(1.0f),
    num_atoms_(10),
    positions_(Eigen::MatrixXf::Random(3, num_atoms_))
{
    glfwWindowHint(GLFW_SAMPLES, 0);
    // Setup Widgets.
    FormHelper *gui = new FormHelper(this);
    ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "Controls");

    FloatBox<float> *radiusBox = gui->addVariable("radius", radius_);
    radiusBox->setSpinnable(true);

    gui->addVariable("number of atoms", num_atoms_);

    gui->addButton("Update", [this]() { updatePositions(); });

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
}

GUI::~GUI() {
    shader_.free();
}

bool GUI::scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel) {
    if (!nanogui::Screen::scrollEvent(p, rel)) {
        zoom_ += 0.1*rel.y();
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
    positions_ = Eigen::MatrixXf::Random(3, num_atoms_);
}

void GUI::drawContents() {
    shader_.bind();

    MatrixXu indices = MatrixXu::Zero(positions_.cols(), 1);
    for (size_t i=0; i<positions_.cols(); i++) {
        indices(i,0) = i;
    }

    shader_.uploadIndices(indices);
    shader_.uploadAttrib("sphere_center", positions_);

    // Make perspective matrix.
    float aspectRatio = float(mSize.x()) / float(mSize.y());
    Matrix4f pmat = ortho(-aspectRatio, aspectRatio, -1.0, 1.0, -1.0, 1.0);
    shader_.setUniform("perspective", pmat);

    // Make model matrix.
    Matrix4f model = scale(Eigen::Vector3f(
        1.0/(positions_.row(0).maxCoeff() - positions_.row(0).minCoeff()),
        1.0/(positions_.row(1).maxCoeff() - positions_.row(1).minCoeff()),
        1.0/(positions_.row(2).maxCoeff() - positions_.row(2).minCoeff())
    ));

    shader_.setUniform("model", model);

    // Setup view matrix.
    arcball_.setSize(mSize);
    arcball_.motion(mousePos());
    Matrix4f view = arcball_.matrix();
    shader_.setUniform("view", view);

    // Set zoom.
    Matrix4f zoom = scale(Eigen::Vector3f(std::exp(zoom_ - 1.0), std::exp(zoom_ - 1.0), 1));
    shader_.setUniform("zoom", zoom);

    // Update uniforms.
    shader_.setUniform("radius", radius_/10.0);

   // Draw points.
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    shader_.drawIndexed(GL_POINTS, 0, positions_.cols());
    glDisable(GL_DEPTH_TEST);
}
