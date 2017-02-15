#pragma once
#include "Atoms.h"

#include <nanogui/glutil.h>
#include <Eigen/Dense>

class Scene
{
public:
    Scene();
    void setAtoms(const Atoms &atoms);
    int maxNeighbors() {
        return max_neighbor_count_;
    }
    void render(const float aspect_ratio,
                const float zoom,
                const Eigen::Matrix4f &view_matrix,
                const float radius_scale,
                const float saturation,
                const float outline_thickness,
                const float outline_depth,
                const int   neighbor_count,
                const float ao_strength,
                const float ao_decay);

private:
    Atoms atoms_;
    nanogui::GLShader shader_;
    GLuint sphere_texture_, radius_texture_, neighbor_texture_;
    static const int sphere_texture_unit_=0, radius_texture_unit_=1, neighbor_texture_unit_=2;
    int max_neighbor_count_;
    float box_size_;
};
