#pragma once

namespace FS
{

    using GLuint = unsigned int;

    struct FrameBuffer
    {
        GLuint m_fbo      = 0;
        GLuint m_tex_id   = 0;
        GLuint m_depth_id = 0;
        int    m_width    = 0;
        int    m_height   = 0;

        FrameBuffer() : m_fbo(0), m_depth_id(0), m_tex_id(0), m_width(0), m_height(0) {}

        virtual void   createBuffers(int width, int height) = 0;
        virtual void   deleteBuffers()                      = 0;
        virtual void   bind()                               = 0;
        virtual void   unbind()                             = 0;
        virtual GLuint getTexture()                         = 0;
    };

    struct OpenGLFrameBuffer : public FrameBuffer
    {
        void   createBuffers(int width, int height) override;
        void   deleteBuffers() override;
        void   bind() override;
        void   unbind() override;
        GLuint getTexture() override;
    };

} // namespace FS