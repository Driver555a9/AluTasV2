#include "core/rendering/RenderBuffers.h"

#include "core/utility/Assert.h"

namespace CoreEngine
{
    //------------------------------- VBO
    VBO::VBO(const std::vector<Vertex>& vertices) noexcept
    {
        glGenBuffers(1, &m_ID);
        SetNewData(vertices);
    }

    VBO::VBO(const std::vector<GLfloat>& vertex_positions) noexcept
    {
        glGenBuffers(1, &m_ID);
        SetNewData(vertex_positions);
    }

    VBO::VBO(const void* data, const GLuint size_of_data) noexcept
    {
        glGenBuffers(1, &m_ID);
        SetNewData(data, size_of_data);
    }

    VBO::VBO() noexcept
    {
        glGenBuffers(1, &m_ID);
    }
 
    VBO::~VBO() noexcept
    {
        Delete();
    }

    void VBO::SetNewData(const std::vector<Vertex>& vertices) noexcept
    {
        SetNewData(vertices.data(), vertices.size() * sizeof(Vertex));
    }

    void VBO::SetNewData(const std::vector<GLfloat>& vertex_positions) noexcept
    {
        SetNewData(vertex_positions.data(), vertex_positions.size() * sizeof(GLfloat));
    }

    void VBO::SetNewData(const void* data, const GLuint size_of_data) noexcept
    {
        m_size = size_of_data;

        Bind();
        if (m_size > m_capacity)
        {
            m_capacity = m_size * 1.5f;

            glBufferData(GL_ARRAY_BUFFER, m_capacity, nullptr, GL_DYNAMIC_DRAW);
        }

        glBufferSubData(GL_ARRAY_BUFFER, 0, m_size, data);
        Unbind();
    }

    void VBO::ShrinkToFit() noexcept
    {
        if (m_size == m_capacity)
            return;

        GLuint newID;
        glGenBuffers(1, &newID);

        glBindBuffer(GL_ARRAY_BUFFER, newID);
        glBufferData(GL_ARRAY_BUFFER, m_size, nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_COPY_READ_BUFFER, m_ID);
        glBindBuffer(GL_COPY_WRITE_BUFFER, newID);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, m_size);

        glDeleteBuffers(1, &m_ID);

        m_ID       = newID;
        m_capacity = m_size;
    }

    void VBO::Bind() noexcept
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    }

    void VBO::Unbind() noexcept
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void VBO::Delete() noexcept
    {
        if(m_ID)
        {
        glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint VBO::GetID() const noexcept
    {
        return m_ID;
    }


    //------------------------------- VAO
    VAO::VAO()
    {
        glGenVertexArrays(1, &m_ID);
    }

    VAO::~VAO()
    {
        Delete();
    }

    void VAO::LinkAttribute(VBO& VBO, GLuint layout, GLuint numComponents, GLenum type, GLsizeiptr stride, void* offset)
    {
        Bind();
        VBO.Bind();
        glVertexAttribPointer(layout, numComponents, type, GL_FALSE,  stride, offset);
        glEnableVertexAttribArray(layout);
        VBO.Unbind();
        Unbind();
    }

    void VAO::Bind()
    {
        glBindVertexArray(m_ID);
    }

    void VAO::Unbind()
    {
        glBindVertexArray(0);
    }

    void VAO::Delete()
    {
        if(m_ID)
        {
        glDeleteVertexArrays(1, &m_ID);
        }
    }

    GLuint VAO::GetID() const
    {
        return m_ID;
    }


    //------------------------------- EBO
    EBO::EBO(const std::vector<GLuint>& indices) noexcept
    {
        glGenBuffers(1, &m_ID);
        SetNewData(indices);
    }

    EBO::EBO() noexcept
    {
        glGenBuffers(1, &m_ID);
    }

    EBO::~EBO() noexcept
    {
        Delete();
    }

    void EBO::SetNewData(const std::vector<GLuint>& indices) noexcept
    {
        m_size = indices.size() * sizeof(GLuint);

        Bind();

        if (m_size > m_capacity)
        {
            m_capacity = static_cast<size_t>(m_size * 1.5f);

            glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_capacity, nullptr, GL_DYNAMIC_DRAW);
        }
        
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_size, indices.data());
        Unbind();
    }

    void EBO::ShrinkToFit() noexcept
    {
        if (m_size == m_capacity)
            return;

        GLuint newID;
        glGenBuffers(1, &newID);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_size, nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_COPY_READ_BUFFER, m_ID);
        glBindBuffer(GL_COPY_WRITE_BUFFER, newID);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, m_size);

        glDeleteBuffers(1, &m_ID);

        m_ID       = newID;
        m_capacity = m_size;
    }

    void EBO::Bind() noexcept
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
    }

    void EBO::Unbind() noexcept
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void EBO::Delete() noexcept
    {
        if(m_ID)
        {
            glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint EBO::GetID() const noexcept
    {
        return m_ID;
    }


    //------------------------------- UBO
    UBO::UBO() 
    {
        glGenBuffers(1, &m_ID);
    }

    UBO::UBO(const void* data, const GLuint size, const GLuint binding)
    {
        glGenBuffers(1, &m_ID);
        SetNewData(data, size, binding);
    }

    UBO::~UBO()
    {
        Delete();
    }
        
    void UBO::SetNewData(const void* data, const GLuint size, const GLuint _binding) 
    {
        m_binding_point = _binding;
        Bind();
        glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, _binding, m_ID);
        Unbind();
    }

    void UBO::SetSubData(const void* data, const GLuint size, const GLintptr offset)
    {
        Bind();
        glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
        Unbind();
    }
        
    void UBO::Bind() 
    {
        glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
    }

    void UBO::BindBase()
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_binding_point, m_ID);
    }
        
    void UBO::Unbind() 
    {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void UBO::Delete()
    {
        if(m_ID)
        {
        glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint UBO::GetBindingPoint() const
    {
        return m_binding_point;
    }

    //------------------------------- SSBO
    SSBO::SSBO() noexcept
    {
        glGenBuffers(1, &m_ID);
    }

    SSBO::SSBO(const void* data, const GLuint size, const GLuint bindingPoint) noexcept
    {
        glGenBuffers(1, &m_ID);
        SetNewData(data, size, bindingPoint);
    }

    SSBO::~SSBO() noexcept
    {
        Delete();
    }

    void SSBO::SetNewData(const void* data, GLuint size, GLuint bindingPoint) noexcept
    {
        m_binding_point = bindingPoint;
        m_size = size;

        Bind();
        if (size > m_capacity)
        {
            m_capacity = size * 1.5f;

            glBufferData(GL_SHADER_STORAGE_BUFFER, m_capacity, nullptr, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_binding_point, m_ID);
        }

        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
        Unbind();
    }

    void SSBO::SetSubData(const void* data, const GLuint size, const GLuint offset) noexcept
    {
        ENGINE_ASSERT(size + offset <= m_capacity && "SSBO::SetSubData(): Data is not in bounds.");
        m_size = std::max<size_t>(m_size, offset + size);
        Bind();
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
        Unbind();
    }

    void SSBO::ShrinkToFit() noexcept
    {
        if (m_size == m_capacity)
            return; 

        GLuint newID;
        glGenBuffers(1, &newID);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, newID);
        glBufferData(GL_SHADER_STORAGE_BUFFER, m_size, nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_COPY_READ_BUFFER, m_ID);
        glBindBuffer(GL_COPY_WRITE_BUFFER, newID);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, m_size);

        glDeleteBuffers(1, &m_ID);

        m_ID = newID;
        m_capacity = m_size;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_binding_point, m_ID);
    }

    void SSBO::Bind() noexcept 
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
    }

    void SSBO::BindBase() noexcept
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_binding_point, m_ID);
    }

    void SSBO::Unbind() noexcept
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void SSBO::Delete() noexcept
    {
        if(m_ID)
        {
            glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint SSBO::GetID() const noexcept
    {
        return m_ID;
    }

    GLuint SSBO::GetBindingPoint() const noexcept
    {
        return m_binding_point;
    }

    //------------------------------- Indirect Buffer
    IndirectBuffer::IndirectBuffer(const void* data, const GLsizeiptr size) noexcept
    {
        glGenBuffers(1, &m_ID);
        SetNewData(data, size);
    }

    IndirectBuffer::IndirectBuffer() noexcept
    {
        glGenBuffers(1, &m_ID);
    }

    IndirectBuffer::~IndirectBuffer() noexcept
    {
        Delete();
    }

    void IndirectBuffer::SetNewData(const void* data, const GLsizeiptr size) noexcept
    { 
        m_size = size;

        Bind();
        if (size > m_capacity)
        {
            m_capacity = size * 1.5f;

            Delete();
            glGenBuffers(1, &m_ID);
            Bind();
            glBufferData(GL_DRAW_INDIRECT_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        }

        glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, size, data);
        Unbind();
    }

    void IndirectBuffer::SetSubData(const void* data, const GLsizeiptr size, const GLuint offset) noexcept
    {
        ENGINE_ASSERT(size + offset <= m_capacity && "IndirectBuffer::SetSubData(): Data is not in bounds.");
        m_size = std::max<size_t>(m_size, offset + size);
        Bind();
        glBufferSubData(GL_DRAW_INDIRECT_BUFFER, offset, size, data);
        Unbind();
    }   

    void IndirectBuffer::ShrinkToFit() noexcept
    {
        ENGINE_ASSERT(m_size <= m_capacity && "At IndirectBuffer::ShrinkToFit(): Size must not be greater than capacity");
        if (m_size == m_capacity)
            return; 

        GLuint newID;
        glGenBuffers(1, &newID);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, newID);
        glBufferData(GL_DRAW_INDIRECT_BUFFER, m_size, nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_COPY_READ_BUFFER, m_ID);
        glBindBuffer(GL_COPY_WRITE_BUFFER, newID);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, m_size);

        glDeleteBuffers(1, &m_ID);

        m_ID = newID;
        m_capacity = m_size;
    }

    void IndirectBuffer::Bind() noexcept
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_ID);
    }

    void IndirectBuffer::Unbind() noexcept
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    }

    void IndirectBuffer::Delete() noexcept
    {
        if(m_ID)
        {
            glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint IndirectBuffer::GetID() const noexcept
    {
        return m_ID;
    }
}