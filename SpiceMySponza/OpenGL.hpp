#pragma once

#if !defined    _UTIL_OPEN_GL_
#define         _UTIL_OPEN_GL_


// STL headers.
#include <vector>


// Forward declarations.
namespace SceneModel { class Mesh; }
using GLchar    = char;
using GLenum    = unsigned int;
using GLsizei   = int;
using GLuint    = unsigned int;


namespace util
{
    /// <summary>
    /// A basic enumeration to indicate the type of shader to process/create/use/etc.
    /// </summary>
    enum class ShaderType : int
    {   
        Vertex = 0,
        Fragment = 1
    };


    /// <summary> Compiles a shader from a file located on the machine. </summary>
    /// <returns> Returns the OpenGL ID of the compiled shader, 0 means an error occurred. </returns>
    /// <param name="fileLocation"> The location of the shader file. </param>
    /// <param name="shader"> The type of shader to compile. </param>
    GLuint compileShaderFromFile (const std::string& fileLocation, const ShaderType shader);


    /// <summary> Attaches a shader to the given program. It will also fill the shader with the attributes specified. </summary>
    /// <param name="program"> The ID of the OpenGL program to attach the shader to. </param>
    /// <param name="shader"> The ID of the OpenGL shader we will be attaching. </param>
    /// <param name="attributes"> An array of attributes to bind to the shader. </param>
    void attachShader (const GLuint program, const GLuint shader, const std::vector<GLchar*>& attributes);


    /// <summary> Links all attached shaders together ready for use. </summary>
    /// <returns> Returns whether the linking process was successful or not. </returns>
    /// <param name="program"> The ID of the OpenGL program which we will be linking together. </param>
    bool linkProgram (const GLuint program);


    /// <summary> Generates and allocates the desired amount of memory for a given VBO. </summary>
    /// <param name="vbo"> The empty VBO which will reflect the value of newly bound buffer. </param>
    /// <param name="size"> The total size in bytes to allocate to the VBO.. </param>
    /// <param name="target"> The target buffer type, e.g. GL_ARRAY_BUFFER/GL_ELEMENT_ARRAY_BUFFER. </param>
    /// <param name="usage"> The usage parameter of the buffered data, e.g. GL_STATIC_DRAW. </param>
    void allocateVBO (GLuint& vbo, const size_t size, const GLenum target, const GLenum usage);


    /// <summary> Generates and fills a VBO with the given data. </summary>
    /// <param name="vbo"> The empty VBO which will reflect the value of newly bound buffer. </param>
    /// <param name="data"> An array of data to fill the vbo with. </param>
    /// <param name="target"> The target buffer type, e.g. GL_ARRAY_BUFFER/GL_ELEMENT_ARRAY_BUFFER. </param>
    /// <param name="usage"> The usage parameter of the buffered data, e.g. GL_STATIC_DRAW. </param>
    template <typename T> void fillVBO (GLuint& vbo, const std::vector<T>& data, const GLenum target, const GLenum usage);


    /// <summary> Creates an instanced glm::mat4 attribute on the currently bound VAO. This loops through each column enabling each glm::vec4 pointer. </summary>
    /// <param name="attribLocation"> The attribute location to start at, four locations will be used and invalid values will be ignore. </param>
    /// <param name="stride"> How many bytes are between consecutive attributes. </param>
    /// <param name="extraOffset"> Any additional offset which will be added to the matrix. </param>
    /// <param name="divisor"> How frequently the attribute should be updated between instances. </param>
    void createInstancedMatrix4 (const int attribLocation, const GLsizei stride, const int extraOffset = 0, const int divisor = 1);


    /// <summary> Generates a texture buffer from the given file location. </summary>
    /// <param name="textureBuffer"> The buffer to fill with the texture data. </param>
    /// <param name="fileLocation> The location of the texture file to load. </param>
    void generateTexture2D (GLuint& textureBuffer, const std::string& fileLocation);


    /// <summary> Iterates through each SceneMode::Mesh in meshes calculating the total buffer size required for a vertex VBO and element VBO. </summary>
    /// <param name="meshes"> A container of all meshes which will exist in a VBO. </param>
    /// <param name="vertexSize"> The calculated size that a vertex array buffer needs to be. </param>
    /// <param name="elementSize"> The calculated size that an element array buffer needs to be. </param>
    void calculateVBOSize (const std::vector<SceneModel::Mesh>& meshes, size_t& vertexSize, size_t& elementSize);
}

#endif // _UTIL_OPEN_GL