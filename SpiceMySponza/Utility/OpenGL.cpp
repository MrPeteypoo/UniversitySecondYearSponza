#include "OpenGL.h"



// STL headers.
#include <iostream>



// Engine headers.
#include <tgl/tgl.h>
#include <tygra/FileHelper.hpp>



// Personal headers.
#include <MyView/Material.h>



namespace util
{
    #pragma region Template instantiations
    
    // Instant the different required templates to avoid including OpenGL in the header.
    template void fillBuffer (GLuint& vbo, const std::vector<MyView::Material>& data, const GLenum target, const GLenum usage);

    #pragma endregion


    #pragma region Compilation
    
    GLuint compileShaderFromFile (const std::string& fileLocation, const GLenum shader)
    {
        // Read in the shader into a const char*.
        const auto shaderString = tygra::stringFromFile (fileLocation);
        auto       shaderCode   = shaderString.c_str();
    
        // Attempt to compile the shader.
        GLuint shaderID { };
        shaderID = glCreateShader (shader);

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

    #pragma endregion


    #pragma region Allocation

    void allocateBuffer (GLuint& buffer, const size_t size, const GLenum target, const GLenum usage)
    {
        if (buffer == 0)
        {
            glGenBuffers (1, &buffer);
        }
        
        glBindBuffer (target, buffer);
        glBufferData (target, size, nullptr, usage);
        glBindBuffer (target, 0);
    }


    template <typename T> void fillBuffer (GLuint& buffer, const std::vector<T>& data, const GLenum target, const GLenum usage)
    {
        if (buffer == 0)
        {
            glGenBuffers (1, &buffer);
        }

        glBindBuffer (target, buffer);
        glBufferData (target, data.size() * sizeof (T), data.data(), usage);
        glBindBuffer (target, 0);
    }

    #pragma endregion


    #pragma region Miscellaneous

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
            if (textureBuffer == 0)
            {
                glGenTextures (1, &textureBuffer);
            }

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
    
    #pragma endregion
}