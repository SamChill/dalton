#include "GUI.h"

#include <nanogui/nanogui.h>
#include <nanogui/screen.h>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv) {
    try {
        nanogui::init();

        {
            nanogui::ref<GUI> gui = new GUI();
            if (argc > 1) {
                gui->setXYZPath(argv[1]);
            }
            gui->drawAll();
            gui->setVisible(true);
            nanogui::mainloop(1);
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        string error_msg = string("Caught a fatal error: ") + string(e.what());
        cerr << error_msg << endl;
        return -1;
    }

    return 0;
}
