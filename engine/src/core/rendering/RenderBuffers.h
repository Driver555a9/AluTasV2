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
    public:
        explicit VBO(const std::vector<Vertex>& vertices) noexcept;
        explicit VBO(const std::vector<GLfloat>& vertex_positions) noexcept;
        explicit VBO(const void* data, const GLuint size_of_data) noexcept;

        VBO() noexcept;
        ~VBO() noexcept;

        void SetNewData(const std::vector<Vertex>& vertices) noexcept;
        void SetNewData(const std::vector<GLfloat>& vertex_positions) noexcept;
        void SetNewData(const void* data, const GLuint size_of_data) noexcept;

        void ShrinkToFit() noexcept;

        void Bind() noexcept;
        void Unbind() noexcept;

        [[nodiscard]] GLuint GetID() const noexcept;
                    
        ///////////////////////////////////
        /// Copy/Move behaviour
        ///////////////////////////////////
        VBO(const VBO&)            = delete;
        VBO& operator=(const VBO&) = delete;

        VBO(VBO&&)                 = delete;
        VBO& operator=(VBO&&)      = delete;

    private:
        GLuint m_ID;
        size_t m_capacity = 0;
        size_t m_size = 0;
        void Delete() noexcept;
    };

    class VAO
    {
    public:
        VAO();
        ~VAO();

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

            
        ///////////////////////////////////
        /// Copy/Move behaviour
        ///////////////////////////////////
        VAO(const VAO&)            = delete;
        VAO& operator=(const VAO&) = delete;
        
        explicit VAO(VAO&&)        = delete;
        VAO& operator=(VAO&&)      = delete;

    private:
        GLuint m_ID;
        void Delete();
    };

    class EBO 
    {
    public:
        explicit EBO(const std::vector<GLuint>& indices) noexcept;

        EBO() noexcept;
        ~EBO() noexcept;

        /// @brief Set new data to the EBO & override previous
        /// You need not call Bind() and Unbind() explicitly
        /// @param indices New indices data to be stored
        void SetNewData(const std::vector<GLuint>& indices) noexcept;
        void ShrinkToFit() noexcept;

        /// @brief Bind this EBO
        void Bind() noexcept;

        /// @brief Unbind Element Buffer Object
        void Unbind() noexcept;

        [[nodiscard]] GLuint GetID() const noexcept;

        ///////////////////////////////////
        /// Copy/Move behaviour
        ///////////////////////////////////
        EBO(const EBO&)            = delete;
        EBO& operator=(const EBO&) = delete;
        
        EBO(EBO&&)                 = delete;
        EBO& operator=(EBO&&)      = delete;         

    private:
        GLuint m_ID;
        size_t m_capacity = 0;
        size_t m_size = 0;
        void Delete() noexcept;

    };

    class UBO 
    {
    public:
        UBO();
        explicit UBO(const void* data, const GLuint size, const GLuint binding);
        ~UBO();

        void SetNewData(const void* data, const GLuint size, const GLuint binding);
        void SetSubData(const void* data, const GLuint size, const GLintptr offset);
    
        void Bind();
        void BindBase();
    
        void Unbind();

        [[nodiscard]] GLuint GetID() const;
        [[nodiscard]] GLuint GetBindingPoint() const;

        
        ///////////////////////////////////
        /// Copy/Move behaviour
        ///////////////////////////////////
        UBO(const UBO&)            = delete;
        UBO& operator=(const UBO&) = delete;
        
        UBO(UBO&&)                 = delete;
        UBO& operator=(UBO&&)      = delete;

    private:
        GLuint m_ID;
        GLuint m_binding_point;
        void Delete();
    };

    class SSBO
    {
    public:
        explicit SSBO(const void* data, const GLuint size, const GLuint bindingPoint) noexcept; 
        SSBO() noexcept;
        ~SSBO() noexcept;

        void Bind() noexcept;
        void BindBase() noexcept;
        void Unbind() noexcept;

        void SetNewData(const void* data, const GLuint size, const GLuint bindingPoint) noexcept;
        void SetSubData(const void* data, const GLuint size, const GLuint offset) noexcept;
        void ShrinkToFit() noexcept;

        [[nodiscard]] GLuint GetID() const noexcept;

        [[nodiscard]] GLuint GetBindingPoint() const noexcept;

        ///////////////////////////////////
        /// Copy/Move behaviour
        ///////////////////////////////////
        SSBO(const SSBO&)            = delete;
        SSBO& operator=(const SSBO&) = delete;
        
        SSBO(SSBO&&)                 = delete;
        SSBO& operator=(SSBO&&)      = delete;

    private:
        GLuint m_ID = 0;
        GLuint m_binding_point = 0;

        size_t m_capacity = 0;
        size_t m_size = 0;
        void Delete() noexcept;
    };

    class IndirectBuffer
    {
    public:
        /// @details MAKE SURE to SetNewData() before usage
        IndirectBuffer() noexcept;
        explicit IndirectBuffer(const void* data, const GLsizeiptr size) noexcept;
        ~IndirectBuffer() noexcept;

        /// Deletes buffer per regrow (for compatibility reasons), improve this as soon as possible
        void SetNewData(const void* data, const GLsizeiptr size) noexcept;
        void SetSubData(const void* data, const GLsizeiptr size, const GLuint offset) noexcept;
        void ShrinkToFit() noexcept;

        void Bind() noexcept;

        void Unbind() noexcept;

        [[nodiscard]] GLuint GetID() const noexcept;

        ////////////////////////////
        /// Copy/Move behaviour
        ////////////////////////////
        IndirectBuffer(const IndirectBuffer&)              = delete;
        IndirectBuffer& operator=(const IndirectBuffer&)   = delete;
        
        IndirectBuffer(IndirectBuffer&& other)             = delete;
        IndirectBuffer& operator=(IndirectBuffer&& other)  = delete;

    private:
        GLuint m_ID;
        size_t m_capacity = 0;
        size_t m_size = 0;
        void Delete() noexcept;
    };
}