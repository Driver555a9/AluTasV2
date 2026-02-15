#pragma once

//GLAD
#include <glad/gl.h>
//glfw
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//Assimp
#include <assimp/scene.h>
//Glm
#include <glm/glm.hpp>

namespace CoreEngine
{
    class Texture
    {
        private:
            GLuint           m_ID {0};
            GLuint64         m_bindless_handle {0};

            unsigned char*   m_data_img = nullptr;
            int              m_width_img, m_height_img, m_amount_channels_img;

            void GenerateGpuTexture();

        public:

            explicit Texture(const char* file_path);
            explicit Texture(const aiTexture* embeddedTexture);
            explicit Texture(const glm::vec3& rgb_color_tex); //Solid color
            explicit Texture(Texture&& _tex);
            ~Texture();

            Texture()                          = delete;
            Texture(const Texture&)            = delete;
            Texture& operator=(const Texture&) = delete;
            Texture& operator=(Texture&&)      = delete;

            void Bind();
            void Unbind();

            [[nodiscard]] GLuint GetID() const;
            [[nodiscard]] GLuint64 GetBindlessHandle() const;
    };
}