#pragma once

//std
#include <iostream>
#include <vector>
#include <array>

//Own includes
#include "core/model/Model.h"

namespace CoreEngine
{
    struct DrawElementsIndirectCommand 
    {
        GLuint count;         // Number of indices to draw
        GLuint instanceCount; // Instance amount
        GLuint firstIndex;    // Offset in index buffer
        GLint  baseVertex;    // Offset in vertex buffer
        GLuint baseInstance;  // Mesh index (for SSBO lookup)     
    };

    class VBO
    {
        private:
            GLuint m_ID;
            void Delete();

        public:
            explicit VBO(const std::vector<Vertex>& vertices);
            explicit VBO(const std::vector<GLfloat>& vertex_positions);

            explicit VBO(void* data, const GLuint size_of_data);

            template <std::size_t N>
            explicit VBO(const std::array<GLfloat, N>& vertex_positions)
            {
                glGenBuffers(1, &m_ID);
                SetNewData(vertex_positions);
            }

            VBO();
            ~VBO();

//------------------------- Copy/Move behaviour
            VBO(const VBO&)            = delete;
            VBO& operator=(const VBO&) = delete;

            VBO(VBO&&)                 = delete;
            VBO& operator=(VBO&&)      = delete;
//-------------------------
            template <std::size_t N>
            inline void SetNewData(const std::array<GLfloat, N>& vertex_positions)
            {
                Bind();
                glBufferData(GL_ARRAY_BUFFER, vertex_positions.size() * sizeof(GLfloat), vertex_positions.data(), GL_STATIC_DRAW);
                Unbind();
            }

            void SetNewData(const std::vector<Vertex>& vertices);
            void SetNewData(const std::vector<GLfloat>& vertex_positions);

            void SetNewData(const void* data, const GLuint size_of_data);

            void Bind();
            void Unbind();

            [[nodiscard]] GLuint GetID() const;
    };

    class VAO
    {
        private:
            GLuint m_ID;
            void Delete();

        public:
            VAO();
            ~VAO();

//------------------------- Copy/Move behaviour
            VAO(const VAO&)            = delete;
            VAO& operator=(const VAO&) = delete;
            
            explicit VAO(VAO&&)        = delete;
            VAO& operator=(VAO&&)      = delete;
//-------------------------

            /// @brief Link attributes to VAO
            /// @param VBO The VBO to Bind()
            /// @param layout The layout/number to access the attribute in glsl Shader
            /// @param numComponents The amount of components that will be stored there
            /// @param type The GL_{Type} that will be stored there
            /// @param stride Amount of values that are stored before next instance of this attribute. Call sizeof(Vertex)
            /// @param offset The numComponents of attributes that are before this one
            void LinkAttribute(VBO& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset);

            /// @brief Binds this VAO
            void Bind();

            /// @brief Unbinds this VAO
            void Unbind();

            [[nodiscard]] GLuint GetID() const;
    };

    class EBO 
    {
        private:
            GLuint m_ID;
            void Delete();

        public:
            explicit EBO(const std::vector<GLuint>& indices);

            EBO();
            ~EBO();

//------------------------- Copy/Move behaviour
            EBO(const EBO&)            = delete;
            EBO& operator=(const EBO&) = delete;
            
            EBO(EBO&&)                 = delete;
            EBO& operator=(EBO&&)      = delete;
//-------------------------            

            /// @brief Set new data to the EBO & override previous
            /// You need not call Bind() and Unbind() explicitly
            /// @param indices New indices data to be stored
            void SetNewData(const std::vector<GLuint>& indices);

            /// @brief Bind this EBO
            void Bind();

            /// @brief Unbind Element Buffer Object
            void Unbind();

            [[nodiscard]] GLuint GetID() const;
    };

    class UBO 
    {
        private:
            GLuint m_ID;
            GLuint m_binding_point;
            void Delete();

        public:
            UBO();
            explicit UBO(const void* data, const GLuint size, const GLuint binding);
            ~UBO();

//------------------------- Copy/Move behaviour
            UBO(const UBO&)            = delete;
            UBO& operator=(const UBO&) = delete;
            
            UBO(UBO&&)                 = delete;
            UBO& operator=(UBO&&)      = delete;
//-------------------------

            void SetNewData(const void* data, const GLuint size, const GLuint binding);

            void SetSubData(const void* data, const GLuint size, const GLintptr offset);
        
            void Bind();
            void BindBase();
        
            void Unbind();

            [[nodiscard]] GLuint GetID() const;

            [[nodiscard]] GLuint GetBindingPoint() const;
    };

    class SSBO
    {
        private:
            GLuint m_ID;
            GLuint m_binding_point;
            void Delete();

        public:

            explicit SSBO(const void* data, const GLuint size, const GLuint bindingPoint);
            SSBO();
            ~SSBO();

//------------------------- Copy/Move behaviour
            SSBO(const SSBO&)            = delete;
            SSBO& operator=(const SSBO&) = delete;
            
            SSBO(SSBO&&)                 = delete;
            SSBO& operator=(SSBO&&)      = delete;
//------------------------- 

            void Bind();
            void BindBase();
            void Unbind();

            void SetNewData(const void* data, const GLuint size, const GLuint bindingPoint);
            void SetSubData(const void* data, const GLuint size, const GLuint offset);

            [[nodiscard]] GLuint GetID() const;

            [[nodiscard]] GLuint GetBindingPoint() const;
    };

    class IndirectBuffer
    {
        private:
            GLuint m_ID;
            void Delete();

        public:
            /// @details MAKE SURE to SetNewData() before usage
            IndirectBuffer();
            explicit IndirectBuffer(const void* data, const GLsizeiptr size);
            ~IndirectBuffer();

//------------------------- Copy/Move behaviour
            IndirectBuffer(const IndirectBuffer&)              = delete;
            IndirectBuffer& operator=(const IndirectBuffer&)   = delete;
            
            IndirectBuffer(IndirectBuffer&& other)             = delete;
            IndirectBuffer& operator=(IndirectBuffer&& other)  = delete;
//-------------------------

            void SetNewData(const void* data, const GLsizeiptr size);
            void SetSubData(const void* data, const GLsizeiptr size, const GLuint offset);

            /// @brief Bind as an GL_DRAW_INDIRECT_BUFFER
            void Bind();

            /// @brief Bind GL_DRAW_INDIRECT_BUFFER to 0
            void Unbind();

            [[nodiscard]] GLuint GetID() const;
    };
}