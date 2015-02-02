#pragma once

#if !defined    _UTIL_OPEN_GL_
#define         _UTIL_OPEN_GL_


// STL headers.
#include <vector>


// Using declarations.
using GLchar    = char;
using GLenum    = unsigned int;
using GLsizei   = int;
using GLuint    = unsigned int;


namespace util
{
    #pragma region Compilation

    /// <summary> Compiles a shader from a file located on the machine. </summary>
    /// <returns> Returns the OpenGL ID of the compiled shader, 0 means an error occurred. </returns>
    /// <param name="fileLocation"> The location of the shader file. </param>
    /// <param name="shader"> The type of shader to compile. </param>
    GLuint compileShaderFromFile (const std::string& fileLocation, const GLenum shader);


    /// <summary> Attaches a shader to the given program. It will also fill the shader with the attributes specified. </summary>
    /// <param name="program"> The ID of the OpenGL program to attach the shader to. </param>
    /// <param name="shader"> The ID of the OpenGL shader we will be attaching. </param>
    /// <param name="attributes"> An array of attributes to bind to the shader. </param>
    void attachShader (const GLuint program, const GLuint shader, const std::vector<GLchar*>& attributes);


    /// <summary> Links all attached shaders together ready for use. </summary>
    /// <returns> Returns whether the linking process was successful or not. </returns>
    /// <param name="program"> The ID of the OpenGL program which we will be linking together. </param>
    bool linkProgram (const GLuint program);

    #pragma endregion

    #pragma region Allocation

    /// <summary> Allocates the desired amount of memory for a given buffer. </summary>
    /// <param name="buffer"> The buffer to be allocated the specified amount of memory. If this is invalid then the buffer will be generated. </param>
    /// <param name="size"> The total size in bytes to allocate to the buffer. </param>
    /// <param name="target"> The target buffer type, e.g. GL_ARRAY_BUFFER/GL_ELEMENT_ARRAY_BUFFER. </param>
    /// <param name="usage"> The usage parameter of the buffered data, e.g. GL_STATIC_DRAW/GL_DYNAMIC_DRAW. </param>
    void allocateBuffer (GLuint& buffer, const size_t size, const GLenum target, const GLenum usage);


    /// <summary> Allocates and fills a given buffer with the specified data. </summary>
    /// <param name="buffer"> A buffer which will contain the given data. If this is invalid then the buffer will be generated. </param>
    /// <param name="data"> An array of data to fill the buffer with. </param>
    /// <param name="target"> The target buffer type, e.g. GL_ARRAY_BUFFER/GL_ELEMENT_ARRAY_BUFFER. </param>
    /// <param name="usage"> The usage parameter of the buffered data, e.g. GL_STATIC_DRAW/GL_DYNAMIC_DRAW. </param>
    template <typename T> void fillBuffer (GLuint& buffer, const std::vector<T>& data, const GLenum target, const GLenum usage);

    #pragma endregion

    #pragma region Miscellaneous

    /// <summary> Creates an instanced glm::mat4 attribute on the currently bound VAO. This loops through each column enabling each glm::vec4 pointer. </summary>
    /// <param name="attribLocation"> The attribute location to start at, four locations will be used and invalid values will be ignore. </param>
    /// <param name="stride"> How many bytes are between consecutive attributes. </param>
    /// <param name="extraOffset"> Any additional offset which will be added to the matrix. </param>
    /// <param name="divisor"> How frequently the attribute should be updated between instances. </param>
    void createInstancedMatrix4 (const int attribLocation, const GLsizei stride, const int extraOffset = 0, const int divisor = 1);


    /// <summary> Generates a texture buffer from the given file location. </summary>
    /// <param name="textureBuffer"> The buffer to fill with the texture data. </param>
    /// <param name="fileLocation"> The location of the texture file to load. </param>
    void generateTexture2D (GLuint& textureBuffer, const std::string& fileLocation);

    #pragma endregion
}

#endif // _UTIL_OPEN_GL