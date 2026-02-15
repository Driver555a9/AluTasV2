#include "core/rendering/RenderBuffers.h"

namespace CoreEngine
{
    //------------------------------- VBO
    VBO::VBO(const std::vector<Vertex>& vertices)
    {
        glGenBuffers(1, &m_ID);
        SetNewData(vertices);
    }

    VBO::VBO(const std::vector<GLfloat>& vertex_positions)
    {
        glGenBuffers(1, &m_ID);
        SetNewData(vertex_positions);
    }

    VBO::VBO(void* data, const GLuint size_of_data)
    {
        glGenBuffers(1, &m_ID);
        SetNewData(data, size_of_data);
    }

    VBO::VBO()
    {
        glGenBuffers(1, &m_ID);
    }

    VBO::~VBO()
    {
        Delete();
    }

    void VBO::SetNewData(const std::vector<Vertex>& vertices)
    {
        Bind();
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_DYNAMIC_DRAW);
        Unbind();
    }

    void VBO::SetNewData(const std::vector<GLfloat>& vertex_positions)
    {
        Bind();
        glBufferData(GL_ARRAY_BUFFER, vertex_positions.size() * sizeof(GLfloat), vertex_positions.data(), GL_DYNAMIC_DRAW);
        Unbind();
    }

    void VBO::SetNewData(const void* data, const GLuint size_of_data)
    {
        Bind();
        glBufferData(GL_ARRAY_BUFFER, size_of_data, data, GL_DYNAMIC_DRAW);
        Unbind();
    }

    void VBO::Bind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_ID);
    }

    void VBO::Unbind()
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void VBO::Delete()
    {
        if(m_ID)
        {
        glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint VBO::GetID() const
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
    EBO::EBO(const std::vector<GLuint>& indices)
    {
        glGenBuffers(1, &m_ID);
        SetNewData(indices);
    }

    EBO::EBO()
    {
        glGenBuffers(1, &m_ID);
    }

    EBO::~EBO()
    {
        Delete();
    }

    void EBO::SetNewData(const std::vector<GLuint>& indices)
    {
        Bind();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        Unbind();
    }

    void EBO::Bind()
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
    }

    void EBO::Unbind()
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void EBO::Delete()
    {
        if(m_ID)
        {
        glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint EBO::GetID() const
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
    SSBO::SSBO()
    {
        glGenBuffers(1, &m_ID);
    }

    SSBO::SSBO(const void* data, const GLuint size, const GLuint bindingPoint)
    {
        glGenBuffers(1, &m_ID);
        SetNewData(data, size, bindingPoint);
    }

    SSBO::~SSBO()
    {
        Delete();
    }

    void SSBO::SetNewData(const void* data, const GLuint size, const GLuint _bindingPoint) 
    {
        m_binding_point = _bindingPoint;
        Bind();
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, _bindingPoint, m_ID);
        Unbind();
    }

    void SSBO::SetSubData(const void* data, const GLuint size, const GLuint offset)
    {
        Bind();
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
        Unbind();
    }

    void SSBO::Bind() 
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ID);
    }

    void SSBO::BindBase()
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_binding_point, m_ID);
    }

    void SSBO::Unbind() 
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void SSBO::Delete() 
    {
        if(m_ID)
        {
            glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint SSBO::GetID() const
    {
        return m_ID;
    }

    GLuint SSBO::GetBindingPoint() const
    {
        return m_binding_point;
    }

    //------------------------------- Indirect Buffer
    IndirectBuffer::IndirectBuffer(const void* data, const GLsizeiptr size)
    {
        glGenBuffers(1, &m_ID);
        SetNewData(data, size);
    }

    IndirectBuffer::IndirectBuffer()
    {
        glGenBuffers(1, &m_ID);
    }

    IndirectBuffer::~IndirectBuffer()
    {
        Delete();
    }

    void IndirectBuffer::SetNewData(const void* data, const GLsizeiptr size)
    { 
        Delete();
        glGenBuffers(1, &m_ID);
        Bind();
        glBufferData(GL_DRAW_INDIRECT_BUFFER, size, data, GL_DYNAMIC_DRAW);
        Unbind();
    }

    void IndirectBuffer::SetSubData(const void* data, const GLsizeiptr size, const GLuint offset)
    {
        Bind();
        glBufferSubData(GL_DRAW_INDIRECT_BUFFER, offset, size, data);
        Unbind();
    }   

    void IndirectBuffer::Bind()
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, m_ID);
    }

    void IndirectBuffer::Unbind()
    {
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    }

    void IndirectBuffer::Delete()
    {
        if(m_ID)
        {
            glDeleteBuffers(1, &m_ID);
        }
    }

    GLuint IndirectBuffer::GetID() const
    {
        return m_ID;
    }
}