#pragma once

class PerformanceMonitor {
public:
    PerformanceMonitor(double resolution)
    : t0_(glfwGetTime()),
      resolution_(resolution),
      render_time_(0.0),
      frames_(0) {}

    void update() {
        frames_ += 1;
        double t1 = glfwGetTime();
        double dt = t1 - t0_;
        if (dt > resolution_) {
            render_time_ = dt / ((float) frames_);
            t0_ = t1;
            frames_ = 0;
        }
    }

    double renderTime() {
        return 1000.0 * render_time_;
    }

    double fps() {
        return 1.0 / render_time_;
    }

private:
    double t0_;
    double resolution_;
    double render_time_;
    int frames_;
};
