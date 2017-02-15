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
#include <FreeImage.h>
#include <future>

// Includes for the GLTexture class.
#include <cstdint>
#include <memory>
#include <utility>

using std::string;
using namespace nanogui;


GUI::GUI() :
    nanogui::Screen(Eigen::Vector2i(1024, 768), "Dalton"),
    performance_monitor_(0.25),
    arcball_(2.0f),
    radius_scale_(1.0),
    zoom_(0.0f),
    saturation_(0.6),
    outline_(0.0),
    eta_(0.0),
    ambient_occlusion_(1.0),
    decay_(1.5),
    neighbor_count_(0)
{
    // Setup Widgets.
    FormHelper *gui = new FormHelper(this);
    ref<Window> window = gui->addWindow(Eigen::Vector2i(10, 10), "");

    gui->addGroup("File");
    gui->addButton("Open", [this]() {
        std::string path = file_dialog({{"xyz", "xyz file format"}}, false);
        setXYZPath(path);
    });
    //gui->addButton("Save Screenshot", [this]() {
        //shader_.bind();
        //GLuint framebuffer;
        //glGenFramebuffers(1, &framebuffer);
        //glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        //GLuint texture;
        //glGenTextures(1, &texture);
        //glBindTexture(GL_TEXTURE_2D, texture);
        //glTexImage2D(
        //        GL_TEXTURE_2D, 0, GL_RGB, mSize.x(), mSize.y(), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        //GLuint depthbuffer;
        //glGenRenderbuffers(1, &depthbuffer);
        //glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
        //glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mSize.x(), mSize.y());
        //glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer);
        //glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
        //GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
        //glDrawBuffers(1, DrawBuffers);
        //glViewport(0, 0, mSize.x(), mSize.y());

        //glEnable(GL_DEPTH_TEST);
        //shader_.drawArray(GL_POINTS, 0, atoms_.size());
        //glDisable(GL_DEPTH_TEST);

        //// Create Pixel Array
        //GLubyte* pixels = new GLubyte [3 * mSize.x() * mSize.y()];

        //// Read Pixels From Screen And Buffer Into Array
        //glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        //glReadPixels(0, 0, mSize.x(), mSize.y(), GL_BGR, GL_UNSIGNED_BYTE, pixels);

        //// Convert To FreeImage And Save
        //FIBITMAP* image = FreeImage_ConvertFromRawBits(pixels, mSize.x(),
        //                                                    mSize.y(), 3 * mSize.x(), 24,
        //                                                    0x0000FF, 0xFF0000, 0x00FF00, false);

        //std::async(
        //    std::launch::async,
        //    [&image](){
        //        std::string path = file_dialog({ {"png", "Portable Network Graphics"} }, true);
        //        FreeImage_Save((FREE_IMAGE_FORMAT) FIF_PNG, image, path.c_str(), 0);
        //});

        //// Free Resources
        //FreeImage_Unload(image);
        //delete [] (pixels);
    //});


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
    neighbor_count_box_ = gui->addVariable("neighbour count", neighbor_count_);
    neighbor_count_box_->setSpinnable(true);
    neighbor_count_box_->setMinValue(0);

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
                    saturation_ = 0.6;
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
    });
    presets_box->setSelectedIndex(3);

    // Finalize widget setup.
    performLayout();
}

bool GUI::scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel) {
    if (!nanogui::Screen::scrollEvent(p, rel)) {
        zoom_ -= 0.1*rel.y();
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

void GUI::setXYZPath(std::string path)
{
    scene_.setAtoms(Atoms::readXYZ(path));
    neighbor_count_ = std::min(16, scene_.maxNeighbors());
    neighbor_count_box_->setValue(neighbor_count_);
    neighbor_count_box_->setMaxValue(scene_.maxNeighbors());
}

void GUI::drawContents() {
    float aspect_ratio = float(mSize.x()) / float(mSize.y());

    // Make view matrix.
    arcball_.setSize(mSize);
    arcball_.motion(mousePos());
    Matrix4f view = arcball_.matrix();

    // render the scene.
    scene_.render(
        aspect_ratio,
        zoom_,
        view,
        radius_scale_,
        saturation_,
        outline_,
        eta_,
        neighbor_count_,
        ambient_occlusion_,
        decay_);

    // Calculate fps.
    performance_monitor_.update();
    fps_label_->setCaption(std::to_string(performance_monitor_.fps()).substr(0,5));
    render_time_label_->setCaption(std::to_string(performance_monitor_.renderTime()).substr(0,5));
}
