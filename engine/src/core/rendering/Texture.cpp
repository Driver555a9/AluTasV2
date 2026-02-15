#include "core/rendering/Texture.h"

//STB
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

//Std
#include <iostream>

namespace CoreEngine
{
    Texture::Texture(const char* file_path)
    {
        stbi_set_flip_vertically_on_load(false);
        m_data_img = stbi_load(file_path, &m_width_img, &m_height_img, &m_amount_channels_img, 0);
        
        if (! m_data_img) 
        {
            std::cout << "At Texture::Texture(): Texture failed to load: " << file_path << std::endl;
            return;
        }

        GenerateGpuTexture();

        // Free stb memory
        stbi_image_free(this->m_data_img);
    }

    Texture::Texture(const aiTexture* embeddedTexture)
    {
        m_data_img = nullptr;
        
        if (embeddedTexture->mHeight == 0) 
        { 
            m_data_img = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(embeddedTexture->pcData), 
                    embeddedTexture->mWidth, &m_width_img, &m_height_img, &m_amount_channels_img, 0);
        } 
        else 
        {
            m_data_img = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(embeddedTexture->pcData),
                    embeddedTexture->mWidth * embeddedTexture->mHeight, &m_width_img, &m_height_img, &m_amount_channels_img, 0);
        }
        
        if (! m_data_img) 
        {
            std::cout << "At Texture::Texture(): Embedded texture failed to load." << std::endl;
            return;
        }

        GenerateGpuTexture();
        
        // Free stb memory
        stbi_image_free(this->m_data_img);
    }

    Texture::Texture(const glm::vec3& rgb_solid_color)
    {
        m_width_img           = 1;
        m_height_img          = 1;
        m_amount_channels_img = 3;

        glGenTextures(1, &m_ID);
        Bind();

        unsigned char color[3] = {
            static_cast<unsigned char>(rgb_solid_color.r * 255),
            static_cast<unsigned char>(rgb_solid_color.g * 255),
            static_cast<unsigned char>(rgb_solid_color.b * 255)
        };

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, color);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        Unbind();

        m_bindless_handle = glGetTextureHandleARB(m_ID);
        glMakeTextureHandleResidentARB(m_bindless_handle);
    }

    Texture::Texture(Texture&& _tex) 
    :  m_ID(_tex.m_ID), 
        m_bindless_handle(_tex.m_bindless_handle), 
        m_data_img(_tex.m_data_img), 
        m_width_img(_tex.m_width_img), 
        m_height_img(_tex.m_height_img),
        m_amount_channels_img(_tex.m_amount_channels_img)
    {
        _tex.m_ID = 0;
        _tex.m_bindless_handle = 0;
        _tex.m_data_img = nullptr;
    }

    void Texture::GenerateGpuTexture()
    {
        glGenTextures(1, &m_ID);
        Bind();

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GLfloat maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        if (maxAniso > 1.0f) {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
        }

        GLenum format         = GL_RGBA;
        GLenum internalFormat = GL_RGBA8;
        GLenum pixelType      = GL_UNSIGNED_BYTE;

        if (m_amount_channels_img == 1) 
        {
            format = GL_RED;
            internalFormat = GL_R8;
        } 
        else if (m_amount_channels_img == 3) 
        {
            format = GL_RGB;
            internalFormat = GL_RGB8;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_width_img, m_height_img, 0, format, pixelType, m_data_img);

        glGenerateMipmap(GL_TEXTURE_2D);

        Unbind();

        m_bindless_handle = glGetTextureHandleARB(m_ID);
        glMakeTextureHandleResidentARB(m_bindless_handle);
    }

    //----------- Public methods

    Texture::~Texture() 
    {
        if (m_ID != 0) 
        {
            if (m_bindless_handle != 0) 
            {
                glMakeTextureHandleNonResidentARB(m_bindless_handle);
            }
            glDeleteTextures(1, &m_ID);
        }
    }

    void Texture::Bind()
    {
        glBindTexture(GL_TEXTURE_2D, m_ID);
    }

    void Texture::Unbind()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GLuint Texture::GetID() const
    {
        return m_ID;
    }

    GLuint64 Texture::GetBindlessHandle() const
    {
        return m_bindless_handle;
    }
}