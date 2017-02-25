#pragma once
#include "Atoms.h"

#include <nanogui/glutil.h>
#include <Eigen/Dense>

class AnalyticRenderer
{
public:
    AnalyticRenderer(Eigen::Vector2i &screen_size);
    void setAtoms(const Atoms atoms);
    void resize(const Eigen::Vector2i &size) {}
    void clear() {}
    void render(Eigen::Matrix4f &projection_matrix,
                Eigen::Matrix4f &view_matrix,
                float radius_scale,
                float saturation,
                Eigen::Vector4f &background_color,
                float outline_thickness,
                float outline_depth,
                int   neighbor_count,
                float ao_strength,
                float ao_decay);

private:
    Atoms atoms_;
    nanogui::GLShader analytic_shader_, background_shader_;
    GLuint sphere_texture_, neighbor_texture_;
    static const int sphere_texture_unit_=0, neighbor_texture_unit_=1;
    int max_neighbor_count_;
    float box_size_;
};
