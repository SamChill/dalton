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
    radius_(1.0f)
{
    // Setup Widgets.
    FormHelper *gui = new FormHelper(this);
    ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "Controls");
    FloatBox<float> *radiusBox = gui->addVariable("radius", radius_);
    radiusBox->setSpinnable(true);

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

    //MatrixXu indices(3, 2);
    //indices.col(0) << 0, 1, 3;
    //indices.col(1) << 1, 2, 3;

    Eigen::MatrixXf positions = 0.5*Eigen::MatrixXf::Random(3, 10);

    MatrixXu indices = MatrixXu::Zero(positions.cols(), 1);
    for (size_t i=0; i<positions.cols(); i++) {
        indices(i,0) = i;
    }

    shader_.bind();
    shader_.uploadIndices(indices);
    shader_.uploadAttrib("position", positions);
}

GUI::~GUI() {
    shader_.free();
}

bool GUI::scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel) {
    if (!nanogui::Screen::scrollEvent(p, rel)) {
        std::cout << p << rel << std::endl;
    }
}

bool GUI::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    if (!nanogui::Screen::mouseButtonEvent(p, button, down, modifiers)) {
        arcball_.button(p, down);
    }
}

void GUI::drawContents() {
    shader_.bind();

    arcball_.setSize(mSize);
    arcball_.motion(mousePos());
    // Make perspective matrix.
    float aspectRatio = float(mSize.x()) / float(mSize.y());
    Matrix4f pmat = ortho(-aspectRatio, aspectRatio, -1.0, 1.0, -1.0, 1.0);
    shader_.setUniform("pmat", pmat);

    Matrix4f view = arcball_.matrix();
    shader_.setUniform("view", view);

    // Update uniforms.
    shader_.setUniform("radius", radius_/10.0);

    // Draw points.
    shader_.drawIndexed(GL_POINTS, 0, 10);
}
