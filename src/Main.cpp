#include "GUI.h"

#include <nanogui/nanogui.h>
#include <nanogui/screen.h>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << argv[0] << ": missing filename" << std::endl;
        return 1;
    }
    try {
        nanogui::init();

        {
            nanogui::ref<GUI> gui = new GUI(argv[1]);
            gui->drawAll();
            gui->setVisible(true);
            nanogui::mainloop();
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        string error_msg = string("Caught a fatal error: ") + string(e.what());
        cerr << error_msg << endl;
        return -1;
    }

    return 0;
}
