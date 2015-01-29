#pragma once

#if !defined    _MY_VIEW_
#define         _MY_VIEW_


// STL headers.
#include <unordered_map>
#include <memory>


// Engine headers.
#include <SceneModel/SceneModel_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <Mesh.hpp>


// Forward declarations.
struct Vertex;


/// <summary>
/// A basic enumeration to indicate the type of shader to process/create/use/etc.
/// </summary>
enum class ShaderType : int
{   
    Vertex = 0,
    Fragment = 1
};


/// <summary>
/// Used in creating and rendering of a scene using the Sponza graphics data.
/// </summary>
class MyView final : public tygra::WindowViewDelegate
{
    public:
    
        #pragma region Constructors and destructor

        MyView() = default;
        ~MyView() = default;

        MyView (MyView&& move);
        MyView& operator= (MyView&& move);

        MyView (const MyView& copy) = delete;
        MyView& operator= (const MyView& copy) = delete;

        #pragma endregion

        #pragma region Getters and setters

        void setScene (std::shared_ptr<const SceneModel::Context> scene);

        #pragma endregion

    private:

        #pragma region Window functions

        void windowViewWillStart (std::shared_ptr<tygra::Window> window) override;
        void windowViewDidReset (std::shared_ptr<tygra::Window> window, int width, int height) override;
        void windowViewDidStop (std::shared_ptr<tygra::Window> window) override;    
        void windowViewRender (std::shared_ptr<tygra::Window> window) override;

        #pragma endregion

        #pragma region Scene construction

        /// <summary> Will create the program then compile, attach and link all required shaders together. </summary>
        void buildProgram();

        /// <summary> Retrieves all VAO and VBO ready for the rendering of the scene. </summary>
        void buildMeshData();

        /// <summary> Fills a given vector with vertex information which is obtained from the given mesh. </summary>
        /// <param name="vertices"> An array to be filled with Vertex information. </param>
        /// <param name="mesh"> The mesh to retrieve Vertex data from. </param>
        void assembleVertices (std::vector<Vertex>& vertices, const SceneModel::Mesh& mesh);

        /// <summary> This will go through each mesh in the scene and buffer enough memory for the highest instance count in sponza. </summary>
        void allocateInstancePool();

        /// <summary> Constructs the VAO for the scene using an interleaved vertex VBO and instanced transform matrices. </summary>
        void constructVAO();

        #pragma endregion

        #pragma region Implementation data

        GLuint                                              m_program       { 0 };          //!< The ID of the OpenGL program created and used to draw Sponza.
        float                                               m_aspectRatio   { 0.f };        //!< The calculated aspect ratio of the foreground resolution for the application.

        std::shared_ptr<const SceneModel::Context>          m_scene         { nullptr };    //!< The sponza scene.
        std::vector<std::pair<SceneModel::MeshId, Mesh>>    m_meshes        { };            //!< A container of MeshId and Mesh pairs, used in instance-based rendering of meshes in the scene.

        GLuint                                              m_sceneVAO      { 0 };          //!< A Vertex Array Object for the entire scene.
        GLuint                                              m_vertexVBO     { 0 };          //!< A Vertex Buffer Object which contains the data of every mesh in the scene.
        GLuint                                              m_elementVBO    { 0 };          //!< A Vertex Buffer Object with the elements data for every mesh in the scene.

        GLuint                                              m_instancePool  { 0 };          //!< A pool of memory used in speeding up instanced rendering.
        size_t                                              m_poolSize      { 0 };          //!< The current size of the pool, useful for optimising rendering.


        GLuint                                              m_hexTexture    { 0 };          //!< The ID of the hex texture to be drawn on Sponza.

        #pragma endregion
};

#pragma region OpenGL creation

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
template <typename T> void fillVBO (GLuint& vbo, const std::vector<T>& data, const GLenum target, const GLenum usage)
{
    glGenBuffers (1, &vbo);
    glBindBuffer (target, vbo);
    glBufferData (target, data.size() * sizeof (T), data.data(), usage);
    glBindBuffer (target, 0);
}


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

#pragma endregion

#endif // _MY_VIEW_