#include "ImGuiContext/OpenGLFrameBuffer.hpp"

#include <glad/glad.h>

namespace FS
{

    void OpenGLFrameBuffer::createBuffers(int width, int height)
    {
        // spdlog::info(">>> OpenGLFrameBuffer::create_buffers - before");
        m_width  = width;
        m_height = height;

        if (m_fbo)
        {
            deleteBuffers();
        }

        glGenFramebuffers(1, &m_fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

        // depth texture
        glGenTextures(1, &m_depth_id);
        glBindTexture(GL_TEXTURE_2D, m_depth_id);
        glTexImage2D(
            GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_width, m_height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_depth_id, 0);

        // color texture
        glGenTextures(1, &m_tex_id);
        glBindTexture(GL_TEXTURE_2D, m_tex_id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex_id, 0);

        GLenum buffers[4] = {GL_COLOR_ATTACHMENT0};
        glDrawBuffers(m_tex_id, buffers);

        unbind();

        // spdlog::info(">>> OpenGLFrameBuffer::create_buffers - ok");
    }

    void OpenGLFrameBuffer::deleteBuffers()
    {
        if (m_fbo)
        {
            glDeleteFramebuffers(GL_FRAMEBUFFER, &m_fbo);
            glDeleteTextures(1, &m_tex_id);
            glDeleteTextures(1, &m_depth_id);
            m_tex_id   = 0;
            m_depth_id = 0;
        }
    }

    void OpenGLFrameBuffer::bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        glViewport(0, 0, m_width, m_height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void OpenGLFrameBuffer::unbind() { glBindFramebuffer(GL_FRAMEBUFFER, 0); }

    GLuint OpenGLFrameBuffer::getTexture() { return m_tex_id; }

} // namespace FS