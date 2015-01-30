#pragma once

#if !defined    _MY_VIEW_
#define         _MY_VIEW_


// STL headers.
#include <memory>
#include <unordered_map>


// Engine headers.
#include <SceneModel/SceneModel_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>
#include <OpenGL.hpp>


// Forward declarations.
struct Vertex;


/// <summary>
/// Used in creating and rendering of a scene using the Sponza graphics data.
/// </summary>
class MyView final : public tygra::WindowViewDelegate
{
    public:
    
        #pragma region Constructors and destructor

        MyView()                                = default;
        ~MyView();

        MyView (MyView&& move);
        MyView& operator= (MyView&& move);

        MyView (const MyView& copy)             = delete;
        MyView& operator= (const MyView& copy)  = delete;

        #pragma endregion

        #pragma region Getters and setters

        void setScene (std::shared_ptr<const SceneModel::Context> scene);

        #pragma endregion

    private:

        struct Material;
        struct Mesh;

        #pragma region Window functions

        void windowViewWillStart (std::shared_ptr<tygra::Window> window) override final;
        void windowViewDidReset (std::shared_ptr<tygra::Window> window, int width, int height) override final;
        void windowViewDidStop (std::shared_ptr<tygra::Window> window) override final;
        void windowViewRender (std::shared_ptr<tygra::Window> window) override final;

        #pragma endregion

        #pragma region Scene construction

        /// <summary> Will create the program then compile, attach and link all required shaders together. </summary>
        void buildProgram();

        /// <summary> Retrieves all VAO and VBO ready for the rendering of the scene. </summary>
        void buildMeshData();

        /// <summary> Creates a material for each materialID in the map, ready for rendering. </summary>
        void buildMaterialData();

        /// <summary> </summary>
        void buildTextureData();

        /// <summary> Fills a given vector with vertex information which is obtained from the given mesh. </summary>
        /// <param name="vertices"> An array to be filled with Vertex information. </param>
        /// <param name="mesh"> The mesh to retrieve Vertex data from. </param>
        void assembleVertices (std::vector<Vertex>& vertices, const SceneModel::Mesh& mesh);

        /// <summary> This will go through each mesh in the scene and buffer enough memory for the highest instance count in sponza. </summary>
        void allocateInstancePool();

        /// <summary> Constructs the VAO for the scene using an interleaved vertex VBO and instanced transform matrices. </summary>
        void constructVAO();

        /// <summary> Deletes all Mesh and Material objects and clears the containers. </summary>
        void cleanMeshMaterials();

        #pragma endregion

        #pragma region Implementation data
        
        GLuint                                                  m_program       { 0 };          //!< The ID of the OpenGL program created and used to draw Sponza.
        float                                                   m_aspectRatio   { 0.f };        //!< The calculated aspect ratio of the foreground resolution for the application.

        GLuint                                                  m_sceneVAO      { 0 };          //!< A Vertex Array Object for the entire scene.
        GLuint                                                  m_vertexVBO     { 0 };          //!< A Vertex Buffer Object which contains the data of every mesh in the scene.
        GLuint                                                  m_elementVBO    { 0 };          //!< A Vertex Buffer Object with the elements data for every mesh in the scene.

        GLuint                                                  m_matricesPool  { 0 };          //!< A pool of model and PVM matrices used in speeding up instanced rendering.
        GLuint                                                  m_materialPool  { 0 };          //!< A pool of material diffuse and specular colours, enables instanced rendering.
        size_t                                                  m_poolSize      { 0 };          //!< The current size of the pool, useful for optimising rendering.

        GLuint                                                  m_materialTBO   { 0 };          //!< The ID of the materials texture which points to the material pool buffer.
        GLuint                                                  m_hexTexture    { 0 };          //!< The ID of the hex texture to be drawn on Sponza.

        std::shared_ptr<const SceneModel::Context>              m_scene         { nullptr };    //!< The sponza scene containing instance and camera information.
        std::vector<std::pair<SceneModel::MeshId, Mesh*>>       m_meshes        { };            //!< A container of MeshId and Mesh pairs, used in instance-based rendering of meshes in the scene.
        std::unordered_map<SceneModel::MaterialId, Material*>   m_materials     { };            //!< A map containing each material used for rendering.

        #pragma endregion
};

#endif // _MY_VIEW_