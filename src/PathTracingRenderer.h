#pragma once
#include "AnalyticRenderer.h"
#include "Atoms.h"

#include <nanogui/glutil.h>
#include <Eigen/Dense>

class PathTracingRenderer
{
public:
    PathTracingRenderer(Eigen::Vector2i &screen_size);
    void setAtoms(const Atoms atoms);
    void resize(const Eigen::Vector2i &size);
    void clear();
    void setResolutionFactor(int factor) {
        resolution_factor_ = factor;
        reinitialize();
    }
    void render(Eigen::Matrix4f &projection_matrix,
                Eigen::Matrix4f &view_matrix,
                float radius_scale,
                float saturation,
                Eigen::Vector4f &background_color,
                float shininess,
                int max_bounces,
                float focal_distance,
                float focal_strength,
                float ambient_light,
                float direct_light,
                AnalyticRenderer *analytic_renderer,
                bool adaptive_sampling_enabled);

    GLuint samples;
    float adaptive_sampling_weight;

private:
    Atoms atoms_;
    void reinitialize();
    int resolution_factor_;
    Eigen::Vector4f background_color_;
    Eigen::Vector2i screen_size_;
    Eigen::Vector3f camera_coordinates_;
    nanogui::GLShader render_shader_, display_shader_, adaptive_sampling_shader_;
    GLuint sphere_texture_, sphere_color_texture_, random_texture_, material_texture_;
    GLuint accumulator_framebuffers_[2], accumulator_textures_[2];
    GLuint statistics_textures_[2];
    GLuint renderbuffer_[2];
    static const GLuint sphere_texture_unit_ = 0;
    static const GLuint sphere_color_texture_unit_ = 1;
    static const GLuint accumulator_texture_unit_ = 2;
    static const GLuint random_texture_unit_ = 3;
    static const GLuint material_texture_unit_ = 4;
    static const GLuint statistics_texture_unit_ = 5;
    float max_world_depth_;
    bool resized_;
    float sampling_weight_;
};
