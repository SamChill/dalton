#include <nanogui/screen.h>
#include <nanogui/glutil.h>

class GUI : public nanogui::Screen {
public:
    GUI();
    ~GUI();

    virtual void drawContents();

private:
    nanogui::GLShader mShader;
    float scale;
};
