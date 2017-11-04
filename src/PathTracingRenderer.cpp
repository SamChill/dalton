#include "PathTracingRenderer.h"

#include <Resource.h>
#include <random>

PathTracingRenderer::PathTracingRenderer(Eigen::Vector2i &screen_size)
:screen_size_(screen_size)
,samples(0)
,atoms_(Atoms())
,resized_(false)
,sampling_weight_(1.0)
,adaptive_sampling_weight(1.0)
,resolution_factor_(1)
{
    {
        Resource vertex_source = LOAD_RESOURCE(pathtracing_vert_glsl);
        Resource fragment_source = LOAD_RESOURCE(pathtracing_frag_glsl);
        render_shader_.init(
            "Path Tracing",
            std::string(vertex_source.data(),   vertex_source.size()),
            std::string(fragment_source.data(), fragment_source.size())
        );
    }

    {
        Resource vertex_source = LOAD_RESOURCE(display_vert_glsl);
        Resource fragment_source = LOAD_RESOURCE(display_frag_glsl);
        display_shader_.init(
            "Display",
            std::string(vertex_source.data(),   vertex_source.size()),
            std::string(fragment_source.data(), fragment_source.size())
        );
    }

    {
        Resource vertex_source = LOAD_RESOURCE(adaptive_sampling_vert_glsl);
        Resource fragment_source = LOAD_RESOURCE(adaptive_sampling_frag_glsl);
        adaptive_sampling_shader_.init(
            "Adaptive sampling",
            std::string(vertex_source.data(),   vertex_source.size()),
            std::string(fragment_source.data(), fragment_source.size())
        );
    }

    std::random_device rd{};
    std::mt19937 generator{rd()};
    std::uniform_real_distribution<double> dis(-1.0, 1.0);

    int random_texture_size_ = 512;
    AtomMatrix random = AtomMatrix::Zero(random_texture_size_*random_texture_size_, 3);
    size_t i=0;
    while (i < random_texture_size_*random_texture_size_) {
        Eigen::Vector3f r = Eigen::Vector3f::Zero(3);
        r(0) = dis(generator);
        r(1) = dis(generator);
        r(2) = dis(generator);
        if (r.norm() < 1.0) {
            random.row(i) = r/r.norm();
            i += 1;
        }
    }
    glDeleteTextures(1, &random_texture_);
    glGenTextures(1, &random_texture_);
    glBindTexture(GL_TEXTURE_2D, random_texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB32F,
        random_texture_size_,
        random_texture_size_,
        0,
        GL_RGB,
        GL_FLOAT,
        random.data()
    );

    reinitialize();
}

void PathTracingRenderer::clear()
{
    // Reset the number of samples that have been collected.
    samples = 0;

    // Clear the "ping-pong" framebuffers.
    for (int i=0; i<2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, accumulator_framebuffers_[i]);
        GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }

    // Switch the framebuffer back to the screen.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PathTracingRenderer::setAtoms(const Atoms atoms)
{
    atoms_ = atoms;
    AtomMatrix coordinates = atoms_.coordinates();
    coordinates.rowwise() -= (coordinates.colwise().sum() / ((float) atoms_.size())).eval();
    atoms_.setCoordinates(coordinates);

    reinitialize();
}

void PathTracingRenderer::resize(const Eigen::Vector2i &size) {
    screen_size_ = size;
    resized_ = true;
}

void PathTracingRenderer::reinitialize()
{
    samples = 0;
    adaptive_sampling_weight = 0.1;
    render_shader_.bind();

    GLint num_atoms = atoms_.size();

    // Attributes.
    Eigen::Matrix<GLuint, 3, 2> indices(3, 2);
    indices.col(0) << 0, 1, 2;
    indices.col(1) << 2, 3, 0;

    Eigen::MatrixXf quad(2, 4);
    quad.col(0) << -1, -1;
    quad.col(1) <<  1, -1;
    quad.col(2) <<  1,  1;
    quad.col(3) << -1,  1;

    render_shader_.uploadIndices(indices);
    render_shader_.uploadAttrib("quad", quad);

    // Textures.
    Eigen::Matrix<float, Eigen::Dynamic, 4, Eigen::RowMajor> sphere_data(num_atoms, 4);
    sphere_data.block(0, 0, num_atoms, 3) = atoms_.coordinates();
    sphere_data.col(3) = atoms_.radii();

    glDeleteTextures(1, &sphere_texture_);
    glGenTextures(1, &sphere_texture_);
    glBindTexture(GL_TEXTURE_1D, sphere_texture_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RGBA32F,
        num_atoms,
        0,
        GL_RGBA,
        GL_FLOAT,
        sphere_data.data()
    );

    glDeleteTextures(1, &sphere_color_texture_);
    glGenTextures(1, &sphere_color_texture_);
    glBindTexture(GL_TEXTURE_1D, sphere_color_texture_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RGB32F,
        num_atoms,
        0,
        GL_RGB,
        GL_FLOAT,
        atoms_.colors().data()
    );

    glDeleteTextures(1, &material_texture_);
    glGenTextures(1, &material_texture_);
    glBindTexture(GL_TEXTURE_1D, material_texture_);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage1D(
        GL_TEXTURE_1D,
        0,
        GL_RGB32F,
        num_atoms,
        0,
        GL_RGB,
        GL_FLOAT,
        atoms_.materials().data()
    );

    for (int i=0; i<2; i++) {
        glDeleteFramebuffers(1, &accumulator_framebuffers_[i]);
        glGenFramebuffers(1, &accumulator_framebuffers_[i]);
        glBindFramebuffer(GL_FRAMEBUFFER, accumulator_framebuffers_[i]);

        glDeleteTextures(1, &accumulator_textures_[i]);
        glGenTextures(1, &accumulator_textures_[i]);
        glBindTexture(GL_TEXTURE_2D, accumulator_textures_[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            screen_size_(0)/resolution_factor_,
            screen_size_(1)/resolution_factor_,
            0,
            GL_RGB,
            GL_FLOAT,
            NULL
        );
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumulator_textures_[i], 0);

        glDeleteTextures(1, &statistics_textures_[i]);
        glGenTextures(1, &statistics_textures_[i]);
        glBindTexture(GL_TEXTURE_2D, statistics_textures_[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA32F,
            screen_size_(0)/resolution_factor_,
            screen_size_(1)/resolution_factor_,
            0,
            GL_RGB,
            GL_FLOAT,
            NULL
        );
        glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, statistics_textures_[i], 0);

        GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);
        float black[4] = {0.0, 0.0, 0.0, 0.0};
        glClearBufferfv(GL_COLOR, 0, &black[0]);
        glClearBufferfv(GL_COLOR, 1, &black[0]);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        glDeleteRenderbuffers(1, &renderbuffer_[i]);
        glGenRenderbuffers(1, &renderbuffer_[i]);
        glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer_[i]);
        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            screen_size_(0)/resolution_factor_,
            screen_size_(1)/resolution_factor_);
        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer_[i]);

        GLenum fb_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if(fb_status != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "error: accumulator framebuffer is not complete: " << fb_status << std::endl;
        }
    }


    // Set the framebuffer to the screen.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Setup the display shader. It renders a texture to a screen filled quad.
    display_shader_.bind();
    display_shader_.uploadIndices(indices);
    display_shader_.uploadAttrib("quad", quad);

    // Setup the adaptive sampling shader. It updates the stencil buffer.
    adaptive_sampling_shader_.bind();
    adaptive_sampling_shader_.uploadIndices(indices);
    adaptive_sampling_shader_.uploadAttrib("quad", quad);
}

void PathTracingRenderer::render(Eigen::Matrix4f &projection_matrix,
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
                                 bool adaptive_sampling_enabled)
{
    if (resized_) {
        resized_ = false;
        clear();
        reinitialize();
    }
    render_shader_.bind();
    glDisable(GL_BLEND);
    //glDisable(GL_ALPHA);
    //glDisable(GL_ALPHA_TEST);
    //glDisable(GL_DEPTH_TEST);
    //glDisable(GL_DEPTH);
    GLuint attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    /* Setup uniforms */
    render_shader_.setUniform("projection", projection_matrix);
    render_shader_.setUniform("view", view_matrix);
    render_shader_.setUniform("radius_scale", radius_scale);
    render_shader_.setUniform("max_bounces", max_bounces);
    render_shader_.setUniform("saturation", saturation);
    render_shader_.setUniform("background_color", background_color);
    render_shader_.setUniform("shininess", shininess);
    render_shader_.setUniform("sphere_count", (GLint)atoms_.size());
    render_shader_.setUniform("sphere_texture", sphere_texture_unit_);
    render_shader_.setUniform("sphere_color_texture", sphere_color_texture_unit_);
    render_shader_.setUniform("random_texture", random_texture_unit_);
    render_shader_.setUniform("accumulator_texture", accumulator_texture_unit_);
    render_shader_.setUniform("statistics_texture", statistics_texture_unit_);
    render_shader_.setUniform("material_texture", material_texture_unit_);
    Eigen::Vector2i scaled_screen_size = screen_size_;
    scaled_screen_size(0) /= resolution_factor_;
    scaled_screen_size(1) /= resolution_factor_;
    render_shader_.setUniform("screen_size", scaled_screen_size);
    render_shader_.setUniform("focal_distance", focal_distance);
    render_shader_.setUniform("focal_strength", focal_strength);
    render_shader_.setUniform("ambient_light", ambient_light);
    render_shader_.setUniform("direct_light", direct_light);
    //render_shader_.setUniform("sampling_weight", sampling_weight_);
    Eigen::Vector2f rand_offset = Eigen::Vector2f::Random(2);
    render_shader_.setUniform("rand_offset", rand_offset);

    /* Bind textures */
    size_t const read_idx = (samples+1) % 2;
    size_t const write_idx = (samples) % 2;
    glActiveTexture(GL_TEXTURE0 + sphere_texture_unit_);
    glBindTexture(GL_TEXTURE_1D, sphere_texture_);
    glActiveTexture(GL_TEXTURE0 + sphere_color_texture_unit_);
    glBindTexture(GL_TEXTURE_1D, sphere_color_texture_);
    glActiveTexture(GL_TEXTURE0 + material_texture_unit_);
    glBindTexture(GL_TEXTURE_1D, material_texture_);
    glActiveTexture(GL_TEXTURE0 + random_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, random_texture_);
    glActiveTexture(GL_TEXTURE0 + accumulator_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, accumulator_textures_[read_idx]);
    glActiveTexture(GL_TEXTURE0 + statistics_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, statistics_textures_[read_idx]);

    glBindFramebuffer(GL_FRAMEBUFFER, accumulator_framebuffers_[write_idx]);
    glViewport(0, 0, scaled_screen_size(0), scaled_screen_size(1));
    double dt;
    if (samples == -1) {
        analytic_renderer->render(
            projection_matrix,
            view_matrix,
            radius_scale,
            saturation,
            background_color,
            0.0,
            0.0,
            200,
            1.0,
            2.0);
    }else{
        /* Make a single pass of the path tracing algorithm. */
        glEnable(GL_STENCIL_TEST);
        if (samples == 0) {
            glStencilFunc(GL_EQUAL, 0, 0xFF);
            glClear(GL_COLOR_BUFFER_BIT);
        }else{
            glStencilFunc(GL_EQUAL, 1, 0xFF);
        }
        double t0 = glfwGetTime();
        render_shader_.drawIndexed(GL_TRIANGLES, 0, 2);
        glFinish();
        dt = glfwGetTime() - t0;
        glDisable(GL_STENCIL_TEST);
    }
    samples += 1;

    /* Render the accumulated image to screen */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    display_shader_.bind();
    display_shader_.setUniform("accumulator_texture", accumulator_texture_unit_);
    display_shader_.setUniform("statistics_texture", statistics_texture_unit_);
    glViewport(
        0,
        0,
        screen_size_(0),
        screen_size_(1)
    );
    glActiveTexture(GL_TEXTURE0 + accumulator_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, accumulator_textures_[write_idx]);
    glActiveTexture(GL_TEXTURE0 + statistics_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, statistics_textures_[write_idx]);

    display_shader_.drawIndexed(GL_TRIANGLES, 0, 2);

    // adaptive sampling
    glBindFramebuffer(GL_FRAMEBUFFER, accumulator_framebuffers_[read_idx]);
    adaptive_sampling_shader_.bind();
    glViewport(0, 0, scaled_screen_size(0), scaled_screen_size(1));
    adaptive_sampling_shader_.setUniform("accumulator_texture", accumulator_texture_unit_);
    adaptive_sampling_shader_.setUniform("statistics_texture", statistics_texture_unit_);
    adaptive_sampling_shader_.setUniform("random_texture", random_texture_unit_);
    // Target fps of 30.
    if (dt < 1.0/30.0) {
        // if we are rendering frames too fast, then decrease the weight, which increase the
        // probability that we include a pixel.
        adaptive_sampling_weight *= 0.9;
    }else{
        // if we are rendering frames too slow, then increase the weight, which decreases the
        // probability that we include a pixel.
        adaptive_sampling_weight *= 1.1;
    }
    adaptive_sampling_weight = std::min(0.4f, adaptive_sampling_weight);
    adaptive_sampling_weight = std::max(1e-6f, adaptive_sampling_weight);
    if (!adaptive_sampling_enabled) {
        adaptive_sampling_weight = 0.0;
    }

    adaptive_sampling_shader_.setUniform("weight", adaptive_sampling_weight);
    rand_offset = Eigen::Vector2f::Random(2);
    adaptive_sampling_shader_.setUniform("rand_offset", rand_offset);
    glActiveTexture(GL_TEXTURE0 + accumulator_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, accumulator_textures_[write_idx]);
    glActiveTexture(GL_TEXTURE0 + statistics_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, statistics_textures_[write_idx]);
    glActiveTexture(GL_TEXTURE0 + random_texture_unit_);
    glBindTexture(GL_TEXTURE_2D, random_texture_);

    glEnable(GL_STENCIL_TEST);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 0XFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilMask(0xFF);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    adaptive_sampling_shader_.drawIndexed(GL_TRIANGLES, 0, 2);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_ALPHA);
}
