#pragma once

#if !defined    _MY_VIEW_
#define         _MY_VIEW_


// STL headers.
#include <memory>
#include <unordered_map>


// Engine headers.
#include <SceneModel/SceneModel_fwd.hpp>
#include <tygra/WindowViewDelegate.hpp>


// Personal headers.
#include <Utility/OpenGL.h>


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
        class UniformData;

        #pragma region Scene construction

        /// <summary> Causes the object to initialise, loading and preparing all data. </summary>
        void windowViewWillStart (std::shared_ptr<tygra::Window> window) override final;

        /// <summary> Will create the program then compile, attach and link all required shaders together. </summary>
        /// <returns> Whether the program was compiled properly. </returns>
        bool buildProgram();

        /// <summary> Generates every VAO and buffers owned by the MyView class. </summary>
        void generateOpenGLObjects();

        /// <summary> Retrieves all VAO and VBO ready for the rendering of the scene. </summary>
        void buildMeshData();

        /// <summary> Creates a material for each materialID in the map, ready for rendering. </summary>
        void buildMaterialData();

        /// <summary> Loads all textures and texture buffers required for rendering. </summary>
        void buildTextureData();

        /// <summary> Constructs the VAO for the scene using an interleaved vertex VBO and instanced transform matrices. </summary>
        void constructVAO();

        /// <summary> This will allocate enough memory in m_uniformVBO, m_materialPool and m_matricesPool for modification at run-time. </summary>
        void allocateExtraBuffers();

        /// <summary> Obtains each group of instances for each SceneModel::MeshId and determines the maximum number of instances we'll encounter. </summary>
        /// <returns> The highest instance count of each SceneModel::MeshId in the scene. </returns>
        size_t highestInstanceCount() const;
        
        #pragma endregion

        #pragma region Clean up

        /// <summary> Causes the object to free up any resources being held. </summary>
        void windowViewDidStop (std::shared_ptr<tygra::Window> window) override final;
        
        /// <summary> Deletes all Mesh and Material objects and clears the containers. </summary>
        void cleanMeshMaterials();

        /// <summary> Deletes all VAOs, VBOs, TBOs, etc. owned by the MyView class. </summary>
        void deleteOpenGLObjects();

        #pragma endregion

        #pragma region Rendering

        /// <summary> Changes the viewport, updating the aspect ratio, etc. </summary>
        void windowViewDidReset (std::shared_ptr<tygra::Window> window, int width, int height) override final;

        /// <summary> Renders the given scene, the object should be initialised before calling this. </summary>
        void windowViewRender (std::shared_ptr<tygra::Window> window) override final;

        /// <summary> Fills the Uniform Buffer Object with the correct data for this frame. </summary>
        /// <param name="data"> The data object to use, this should contain a correct projection and view matrix. </param>
        void setUniforms (UniformData& data);

        #pragma endregion

        #pragma region Implementation data
        
        GLuint                                                  m_program           { 0 };          //!< The ID of the OpenGL program created and used to draw Sponza.

        GLuint                                                  m_sceneVAO          { 0 };          //!< A Vertex Array Object for the entire scene.
        GLuint                                                  m_vertexVBO         { 0 };          //!< A Vertex Buffer Object which contains the interleaved vertex data of every mesh in the scene.
        GLuint                                                  m_elementVBO        { 0 };          //!< A Vertex Buffer Object with the elements data for every mesh in the scene.
        GLuint                                                  m_uniformUBO        { 0 };          //!< A Uniform Buffer Object which contains scenes uniform data.
        
        GLuint                                                  m_materialPool      { 0 };          //!< A pool of material diffuse and specular colours, used in instanced rendering.
        GLuint                                                  m_matricesPool      { 0 };          //!< A pool of model and PVM matrices, used in instanced rendering.
        size_t                                                  m_poolSize          { 0 };          //!< The current size of the pool, useful for optimising rendering.

        GLuint                                                  m_materialTBO       { 0 };          //!< The materials texture buffer which points to the material pool buffer.
        GLuint                                                  m_hexTBO            { 0 };          //!< The hex texture to be drawn on Sponza.
        
        float                                                   m_aspectRatio       { 0.f };        //!< The calculated aspect ratio of the foreground resolution for the application.

        std::shared_ptr<const SceneModel::Context>              m_scene             { nullptr };    //!< The sponza scene containing instance and camera information.
        std::vector<std::pair<SceneModel::MeshId, Mesh*>>       m_meshes            { };            //!< A container of MeshId and Mesh pairs, used in instance-based rendering of meshes in the scene.
        std::unordered_map<SceneModel::MaterialId, Material*>   m_materials         { };            //!< A map containing each material used for rendering.

        #pragma endregion
};

#endif // _MY_VIEW_