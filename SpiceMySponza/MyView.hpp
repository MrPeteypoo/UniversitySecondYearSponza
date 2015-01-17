#pragma once

#if !defined    _MY_VIEW_
#define         _MY_VIEW_


// STL headers.
#include <vector>
#include <unordered_map>
#include <memory>


// Engine headers.
#include <SceneModel/SceneModel_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <tgl/tgl.h>
#include <Mesh.hpp>


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

        #pragma region Utility functions

        /// <summary> Will create the program then compile, attach and link all required shaders together. </summary>
        void buildProgram();

        #pragma endregion

        #pragma region Implementation data

        GLuint                                          m_program       { 0 };          //!< The ID of the OpenGL program created and used to draw Sponza.
        float                                           m_aspectRatio   { 0.f };        //!< The calculated aspect ratio of the foreground resolution for the application.

        std::shared_ptr<const SceneModel::Context>      m_scene         { nullptr };    //!< The sponza scene.
        std::unordered_map<SceneModel::MeshId, Mesh>    m_meshes        { };            //!< The collection of meshes which will be used in rendering each mesh in the scene.
        GLuint                                          m_hexTexture    { 0 };          //!< The ID of the hex texture to be drawn on Sponza.

        #pragma endregion
};

#pragma region OpenGL creation

/// <summary> 
/// Compiles a shader from a file located on the machine. 
/// <returns> Returns the OpenGL ID of the compiled shader, 0 means an error occurred. </returns>
/// </summary>
GLuint compileShaderFromFile (const std::string& fileLocation, const ShaderType shader);


/// <summary> 
/// Attaches a shader to the given program. It will also fill the shader with the attributes specified. 
/// </summary>
void attachShader (const GLuint program, const GLuint vertexShader, const std::vector<GLchar*>& attributes);


/// <summary> 
/// Links all attached shaders together ready for use. 
/// <returns> Returns whether the linking process was successful or not. </returns>
/// </summary>
bool linkProgram (const GLuint program);


/// <summary>
/// Generates and fills a VBO with the given data.
/// </summary>
template <typename T> void fillVBO (GLuint& vbo, const std::vector<T>& data, const GLenum target, const GLenum usage)
{
    glGenBuffers (1, &vbo);
    glBindBuffer (target, vbo);
    glBufferData (target, data.size() * sizeof (T), data.data(), usage);
    glBindBuffer (target, 0);
}

#pragma endregion

#endif // _MY_VIEW_