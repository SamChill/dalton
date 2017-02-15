#include "Scene.h"

#include <Resource.h>

Scene::Scene()
:sphere_texture_(0)
,radius_texture_(0)
,neighbor_texture_(0)
,max_neighbor_count_(0)
{
    // Load and initialzer the shader.
    Resource vertexShader   = LOAD_RESOURCE(vertex_glsl);
    Resource fragmentShader = LOAD_RESOURCE(fragment_glsl);
    Resource geometryShader = LOAD_RESOURCE(geometry_glsl);
    shader_.init(
        "Scene",
        std::string(vertexShader.data(),   vertexShader.size()),
        std::string(fragmentShader.data(), fragmentShader.size()),
        std::string(geometryShader.data(), geometryShader.size())
    );
}

void Scene::setAtoms(const Atoms &atoms)
{
    shader_.bind();

    // Make a local copy of the atoms.
    atoms_ = atoms;

    // Center the coordates at the origin.
    AtomMatrix coordinates = atoms_.coordinates();
    coordinates.rowwise() -= (coordinates.colwise().sum() / ((float) atoms_.size())).eval();
    atoms_.setCoordinates(coordinates);

    // Compute the longest size of a cube that contains all atoms.
    box_size_ = (coordinates.colwise().maxCoeff() - coordinates.colwise().minCoeff()).maxCoeff();
    box_size_ *= 1.5;
    shader_.setUniform("box_size", box_size_);

    // Attributes.
    shader_.uploadAttrib("sphere_center", atoms_.coordinates().transpose());
    shader_.uploadAttrib("radius", atoms_.radii().transpose());
    shader_.uploadAttrib("sphere_color", atoms_.colors().transpose());

    // Textures.
    glDeleteTextures(1, &sphere_texture_);
    glGenTextures(1, &sphere_texture_);
    glBindTexture(GL_TEXTURE_1D, sphere_texture_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RGB32F,
        atoms_.size(),
        0,
        GL_RGB,
        GL_FLOAT,
        atoms_.coordinates().transpose().data()
    );

    glDeleteTextures(1, &radius_texture_);
    glGenTextures(1, &radius_texture_);
    glBindTexture(GL_TEXTURE_1D, radius_texture_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F,
        atoms_.radii().rows(),
        0,
        GL_RED,
        GL_FLOAT,
        atoms_.radii().data()
    );

    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    max_neighbor_count_ = std::min(max_texture_size / atoms_.size(), atoms_.size()-1);
    NeighborList neighbor_list = atoms_.neighborList(max_neighbor_count_);
    shader_.setUniform("max_neighbor_count", max_neighbor_count_);

    glDeleteTextures(1, &neighbor_texture_);
    glGenTextures(1, &neighbor_texture_);
    glBindTexture(GL_TEXTURE_1D, neighbor_texture_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_R32F,
        neighbor_list.cols() * neighbor_list.rows(),
        0,
        GL_RED,
        GL_FLOAT,
        neighbor_list.data()
    );

    glActiveTexture(GL_TEXTURE0 + sphere_texture_unit_);
    glBindTexture(GL_TEXTURE_1D, sphere_texture_);
    shader_.setUniform("sphere_texture", sphere_texture_unit_);

    glActiveTexture(GL_TEXTURE0 + radius_texture_unit_);
    glBindTexture(GL_TEXTURE_1D, radius_texture_);
    shader_.setUniform("radius_texture", radius_texture_unit_);

    glActiveTexture(GL_TEXTURE0 + neighbor_texture_unit_);
    glBindTexture(GL_TEXTURE_1D, neighbor_texture_);
    shader_.setUniform("neighbor_texture", neighbor_texture_unit_);
}

void Scene::render(const float aspect_ratio,
                   const float zoom,
                   const Eigen::Matrix4f &view_matrix,
                   const float radius_scale,
                   const float saturation,
                   const float outline_thickness,
                   const float outline_depth,
                   const int   neighbor_count,
                   const float ao_strength,
                   const float ao_decay)
{
    shader_.bind();

    float exp_zoom = std::exp(zoom);
    Eigen::Matrix4f projection = nanogui::ortho(
        -box_size_/2.0*exp_zoom*aspect_ratio, box_size_/2.0*exp_zoom*aspect_ratio,
        -box_size_/2.0*exp_zoom, box_size_/2.0*exp_zoom,
        -box_size_/2.0, box_size_/2.0);

    shader_.setUniform("projection", projection);
    shader_.setUniform("view", view_matrix);
    shader_.setUniform("radius_scale", radius_scale);
    shader_.setUniform("saturation", saturation);
    shader_.setUniform("outline", outline_thickness);
    shader_.setUniform("eta", std::exp(outline_depth));
    shader_.setUniform("neighbor_count", std::min(neighbor_count, max_neighbor_count_));
    shader_.setUniform("ambient_occlusion", ao_strength);
    shader_.setUniform("decay", ao_decay);

    glEnable(GL_DEPTH_TEST);
    shader_.drawArray(GL_POINTS, 0, atoms_.size());
    glDisable(GL_DEPTH_TEST);
}
