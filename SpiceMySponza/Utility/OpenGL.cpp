#include "OpenGL.h"



// STL headers.
#include <iostream>



// Engine headers.
#include <SceneModel/Mesh.hpp>
#include <tgl/tgl.h>
#include <tygra/FileHelper.hpp>



// Personal headers.
#include <Misc/Vertex.h>



namespace util
{
    #pragma region Template instantiations

    template void fillVBO (GLuint& vbo, const std::vector<unsigned int>& data, const GLenum target, const GLenum usage);
    template void fillVBO (GLuint& vbo, const std::vector<Vertex>& data, const GLenum target, const GLenum usage);

    #pragma endregion


    #pragma region Implementations
    
    template <typename T> void fillVBO (GLuint& vbo, const std::vector<T>& data, const GLenum target, const GLenum usage)
    {
        glGenBuffers (1, &vbo);
        glBindBuffer (target, vbo);
        glBufferData (target, data.size() * sizeof (T), data.data(), usage);
        glBindBuffer (target, 0);
    }


    GLuint compileShaderFromFile (const std::string& fileLocation, const ShaderType shader)
    {
        // Read in the shader into a const char*.
        const auto  shaderString = tygra::stringFromFile (fileLocation);
        auto        shaderCode = shaderString.c_str();
    
        // Attempt to compile the shader.
        GLuint shaderID { };

        switch (shader)
        {
            case ShaderType::Vertex:
                shaderID = glCreateShader (GL_VERTEX_SHADER);
                break;

            case ShaderType::Fragment:
                shaderID = glCreateShader (GL_FRAGMENT_SHADER);
                break;
        }

        glShaderSource (shaderID, 1, static_cast<const GLchar**> (&shaderCode), NULL);
        glCompileShader (shaderID);

        // Check whether compilation was successful.
        GLint compileStatus { 0 };

        glGetShaderiv (shaderID, GL_COMPILE_STATUS, &compileStatus);
    
        if (compileStatus != GL_TRUE)
        {
            // Output error information.
            const unsigned int stringLength = 1024;
            GLchar log[stringLength] = "";

            glGetShaderInfoLog (shaderID, stringLength, NULL, log);
            std::cerr << log << std::endl;
        }

        return shaderID;
    }


    void attachShader (const GLuint program, const GLuint shader, const std::vector<GLchar*>& attributes)
    {
        // Check whether we have a valid shader ID before continuing.
        if (shader != 0)
        {
            glAttachShader (program, shader);

            // Add the given attributes to the shader.
            for (unsigned int i = 0; i < attributes.size(); ++i)
            {
                if (attributes[i] != nullptr)
                {
                    glBindAttribLocation (program, i, attributes[i]);
                }
            }

            // Flag the shader for deletion.
            glDeleteShader (shader);
        }    
    }


    bool linkProgram (const GLuint program)
    {
        // Attempt to link the program.
        glLinkProgram (program);

        // Test the status for any errors.
        GLint linkStatus  { 0 };
        glGetProgramiv (program, GL_LINK_STATUS, &linkStatus);

        if (linkStatus != GL_TRUE) 
        {
            // Output error information.
            const unsigned int stringLength = 1024;
            GLchar log[stringLength] = "";

            glGetProgramInfoLog (program, stringLength, NULL, log);
            std::cerr << log << std::endl;

            return false;
        }

        return true;
    }


    void allocateVBO (GLuint& vbo, const size_t size, const GLenum target, const GLenum usage)
    {
        glGenBuffers (1, &vbo);
        glBindBuffer (target, vbo);
        glBufferData (target, size, nullptr, usage);
        glBindBuffer (target, 0);
    }


    void createInstancedMatrix4 (const int attribLocation, const GLsizei stride, const int extraOffset, const int divisor)
    {
        // Pre-condition: A valid attribute location has been given.
        if (attribLocation >= 0)
        {
            // We need to go through each column of the matrices creating attribute pointers.
            const int matrixColumns { 4 };
            for (int i = 0; i < matrixColumns; ++i)
            {
                const int current   { attribLocation + i };

                // Enable each column and set the divisor.
                glEnableVertexAttribArray (current);
                glVertexAttribDivisor (current, divisor);

                // Calculate the offsets for each column.
                const auto offset = TGL_BUFFER_OFFSET (sizeof (glm::vec4) * i + extraOffset);

                // Create the columns attribute pointer.
                glVertexAttribPointer (current,  4, GL_FLOAT, GL_FALSE, stride, offset);
            }
        }
    }


    void generateTexture2D (GLuint& textureBuffer, const std::string& fileLocation)
    {
        // Attempt to load the image.
        tygra::Image image = tygra::imageFromPNG (fileLocation);

        if (image.containsData()) 
        {
            // Start by preparing the texture buffer.
            glGenTextures (1, &textureBuffer);
            glBindTexture (GL_TEXTURE_2D, textureBuffer);

            // Enable standard filters.
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,  GL_LINEAR);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,  GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,      GL_REPEAT);
            glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,      GL_REPEAT);

            // Enable each different pixel format.
            GLenum pixel_formats[] = { 0, GL_RED, GL_RG, GL_RGB, GL_RGBA };

            // Load the texture into OpenGL.
            glTexImage2D (  GL_TEXTURE_2D, 0, GL_RGBA,          
                      
                            // Dimensions and border.
                            image.width(), image.height(), 0,   
                      
                            // Format and type.
                            pixel_formats[image.componentsPerPixel()], image.bytesPerComponent() == 1 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT,
                      
                            // Data.
                            image.pixels());

            // Generate the mipmaps from the loaded texture and finish.
            glGenerateMipmap (GL_TEXTURE_2D);
            glBindTexture (GL_TEXTURE_2D, 0);
        }
    }


    void calculateVBOSize (const std::vector<SceneModel::Mesh>& meshes, size_t& vertexSize, size_t& elementSize)
    {
        // Create temporary accumlators.
        size_t vertices { 0 }, elements { 0 };  

        // We need to loop through each mesh adding up as we go along.
        for (const auto& mesh : meshes)
        {
            vertices += mesh.getPositionArray().size();
            elements += mesh.getElementArray().size();
        }

        // Calculate the final values.
        vertexSize = vertices * sizeof (Vertex);
        elementSize = elements * sizeof (unsigned int);
    }

    #pragma endregion
}