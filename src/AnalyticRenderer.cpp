#include "AnalyticRenderer.h"

#include <Resource.h>

AnalyticRenderer::AnalyticRenderer(Eigen::Vector2i &screen_size)
:sphere_texture_(0)
,neighbor_texture_(0)
,atoms_(Atoms())
{
    {
        // Load and initialzer the shaders.
        Resource vertex_source = LOAD_RESOURCE(analytic_vert_glsl);
        Resource geometry_source = LOAD_RESOURCE(analytic_geom_glsl);
        Resource fragment_source = LOAD_RESOURCE(analytic_frag_glsl);
        analytic_shader_.init(
            "Analytic AO",
            std::string(vertex_source.data(),   vertex_source.size()),
            std::string(fragment_source.data(), fragment_source.size()),
            std::string(geometry_source.data(), geometry_source.size())
        );
    }
    {
        // Load and initialzer the shaders.
        Resource vertex_source = LOAD_RESOURCE(background_vert_glsl);
        Resource geometry_source = LOAD_RESOURCE(background_geom_glsl);
        Resource fragment_source = LOAD_RESOURCE(background_frag_glsl);
        background_shader_.init(
            "Background",
            std::string(vertex_source.data(),   vertex_source.size()),
            std::string(fragment_source.data(), fragment_source.size())
        );
    }
}

void AnalyticRenderer::setAtoms(const Atoms atoms)
{
    if (atoms.size() == 0) {
        return;
    }
    analytic_shader_.bind();
    glDisable(GL_BLEND);

    atoms_ = atoms;
    AtomMatrix coordinates = atoms_.coordinates();
    coordinates.rowwise() -= (coordinates.colwise().sum() / ((float) atoms_.size())).eval();
    atoms_.setCoordinates(coordinates);

    // Compute the side of a cube that contains all atoms.
    box_size_ = (coordinates.colwise().maxCoeff() - coordinates.colwise().minCoeff()).maxCoeff();
    box_size_ *= 1.5;

    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
        sphere_data(atoms_.size(), 4);
    sphere_data.block(0, 0, atoms_.size(), 3) = atoms_.coordinates();
    sphere_data.col(3) = atoms_.radii();

    // Textures.
    glDeleteTextures(1, &sphere_texture_);
    glGenTextures(1, &sphere_texture_);
    glBindTexture(GL_TEXTURE_1D, sphere_texture_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RGBA32F,
        atoms_.size(),
        0,
        GL_RGBA,
        GL_FLOAT,
        sphere_data.data()
    );

    // Attributes.
    analytic_shader_.uploadAttrib("sphere_center", atoms_.coordinates().transpose());
    analytic_shader_.uploadAttrib("radius", atoms_.radii().transpose());
    analytic_shader_.uploadAttrib("sphere_color", atoms_.colors().transpose());

    // Default the maximum number of neighbors we can store in the texture.
    GLint max_texture_size;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture_size);
    max_neighbor_count_ = std::min(max_texture_size, (int)atoms_.size()-1);
    NeighborList neighbor_list = atoms_.neighborList(max_neighbor_count_);

    glDeleteTextures(1, &neighbor_texture_);
    glGenTextures(1, &neighbor_texture_);
    glBindTexture(GL_TEXTURE_2D, neighbor_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_R32I,
        neighbor_list.cols(),
        neighbor_list.rows(),
        0,
        GL_RED_INTEGER,
        GL_INT,
        neighbor_list.data()
    );

    background_shader_.bind();
    Eigen::Matrix<GLuint, 3, 2> indices(3, 2);
    indices.col(0) << 0, 1, 2;
    indices.col(1) << 2, 3, 0;

    Eigen::MatrixXf quad(2, 4);
    quad.col(0) << -1, -1;
    quad.col(1) <<  1, -1;
    quad.col(2) <<  1,  1;
    quad.col(3) << -1,  1;

    background_shader_.uploadIndices(indices);
    background_shader_.uploadAttrib("quad", quad);
}

void AnalyticRenderer::render(Eigen::Matrix4f &projection_matrix,
                              Eigen::Matrix4f &view_matrix,
                              float radius_scale,
                              float saturation,
                              Eigen::Vector4f &background_color,
                              float outline_thickness,
                              float outline_depth,
                              int   neighbor_count,
                              float ao_strength,
                              float ao_decay)
{
    background_shader_.bind();
    background_shader_.setUniform("background_color", background_color);
    background_shader_.drawIndexed(GL_TRIANGLES, 0, 2);

    analytic_shader_.bind();

    analytic_shader_.setUniform("projection", projection_matrix);
    analytic_shader_.setUniform("view", view_matrix);
    analytic_shader_.setUniform("radius_scale", radius_scale);
    analytic_shader_.setUniform("saturation", saturation);
    analytic_shader_.setUniform("box_size", box_size_);
    analytic_shader_.setUniform("outline", outline_thickness);
    analytic_shader_.setUniform("eta", std::exp(outline_depth));
    analytic_shader_.setUniform("neighbor_count", std::min(neighbor_count, max_neighbor_count_));
    analytic_shader_.setUniform("ambient_occlusion", ao_strength);
    analytic_shader_.setUniform("decay", ao_decay);

    analytic_shader_.setUniform("sphere_texture", sphere_texture_unit_);
    analytic_shader_.setUniform("neighbor_texture", neighbor_texture_unit_);

    glActiveTexture(GL_TEXTURE0 + sphere_texture_unit_);
    glBindTexture(GL_TEXTURE_1D, sphere_texture_);
    glActiveTexture(GL_TEXTURE0 + neighbor_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, neighbor_texture_);

    glEnable(GL_DEPTH_TEST);
    analytic_shader_.drawArray(GL_POINTS, 0, atoms_.size());
    glDisable(GL_DEPTH_TEST);
}
