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
namespace tygra { class Image; }
struct Light;
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

        #pragma region Public interface

        /// <summary> Sets the SceneModel::Context to use for rendering. </summary>
        void setScene (std::shared_ptr<const SceneModel::Context> scene);

        /// <summary> Causes the application to rebuild the shaders. </summary>
        void rebuildShaders();

        /// <summary> Enables a wireframe view near the camera. </summary>
        void toggleWireframeMode()  { m_wireframeMode = !m_wireframeMode; }

        /// <summary> Cycles through point, spot and directional wireframe mode. </summary>
        void toggleWireframeType()  { m_wireframeType = ++m_wireframeType % 3; }

        #pragma endregion

    private:

        #pragma region Scene construction

        /// <summary> Causes the object to initialise; loading and preparing all data. </summary>
        void windowViewWillStart (std::shared_ptr<tygra::Window> window) override final;

        /// <summary> Will create the program then compile, attach and link all required shaders together. </summary>
        /// <returns> Whether the program was compiled properly. </returns>
        bool buildProgram();

        /// <summary> Generates the VAO and buffers owned by the MyView class. </summary>
        void generateOpenGLObjects();

        /// <summary> Creates a mesh of every object in the scene and loads the data into VBOs. </summary>
        void buildMeshData();

        /// <summary> Creates a material for each materialID in the map, ready for rendering. </summary>
        void buildMaterialData();

        /// <summary> Constructs the VAO for the scene using an interleaved vertex VBO and instanced transform matrices. </summary>
        void constructVAO();

        /// <summary> This will allocate enough memory in m_uniformVBO, m_materialPool and m_matricesPool for modification at run-time. </summary>
        void allocateExtraBuffers();

        /// <summary> Sets up the binding of the Uniform Buffer Object used for the scene and lighting. </summary>
        void bindUniformBufferObject();

        /// <summary> Prepares the material TBO and allocates storage for the texture array. </summary>
        /// <param name="textureWidth"> The width each texture should be in the array. </param>
        /// <param name="textureHeight"> The height each texture should be in the array. </param>
        /// <param name="textureCount"> The total number of textures the array can store. </param>
        void prepareTextureData (const GLsizei textureWidth, const GLsizei textureHeight, const GLsizei textureCount);

        /// <summary> Loads every given image into the 2D texture array. </summary>
        /// <param name="images"> The images to load. </param>
        void loadTexturesIntoArray (const std::vector<std::pair<std::string, tygra::Image>>& images);

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

        /// <summary> Sets all uniform values for the scene. Avoid including GLM in MyView by passing void*. </summary>
        /// <param name="projectionMatrix"> A pointer to a glm::mat4 projection matrix for the scene. </param>
        /// <param name="viewMatrix"> A pointer to a glm::mat4 view matrix for the scene. </param>
        void setUniforms (const void* const projectionMatrix, const void* const viewMatrix);

        /// <summary> Creates a wireframe light based on the cameras position. </summary>
        /// <returns> A light ready for adding to the UBO. </returns>
        Light createWireframeLight() const;

        #pragma endregion

        #pragma region Implementation data

        struct Material;
        struct Mesh;
        class UniformData;

        // Using declarations.
        using MaterialID = int;

        /// <summary>
        /// A simple structure which contains an ID for a VBO and a TBO, the TBO should be linked to the VBO for use in a sampler buffer.
        /// </summary>
        struct SamplerBuffer final
        {
            GLuint  vbo { 0 };  //!< The buffer to contain shader accessible information.
            GLuint  tbo { 0 };  //!< The texture buffer which points to the VBO, linking them together.

            SamplerBuffer()                                         = default;
            SamplerBuffer (const SamplerBuffer& copy)               = default;
            SamplerBuffer& operator= (const SamplerBuffer& copy)    = default;
            ~SamplerBuffer()                                        = default;

            SamplerBuffer (SamplerBuffer&& move);
            SamplerBuffer& operator= (SamplerBuffer&& move);
        };        

        GLuint                                                  m_program           { 0 };          //!< The ID of the OpenGL program created and used to draw the scene.

        GLuint                                                  m_sceneVAO          { 0 };          //!< A Vertex Array Object for the entire scene.
        GLuint                                                  m_vertexVBO         { 0 };          //!< A Vertex Buffer Object which contains the interleaved vertex data of every mesh in the scene.
        GLuint                                                  m_elementVBO        { 0 };          //!< A Vertex Buffer Object with the elements data for every mesh in the scene.
        
        GLuint                                                  m_uniformUBO        { 0 };          //!< A Uniform Buffer Object which contains scenes uniform data.
        
        SamplerBuffer                                           m_materials         { };            //!< A VBO & TBO pair representing information on every material in the scene.
        GLuint                                                  m_textureArray      { 0 };          //!< The TEXTURE_2D_ARRAY which contains each texture in the scene.
        
        size_t                                                  m_instancePoolSize  { 0 };          //!< The current size of the instance pools, useful for optimising rendering.
        SamplerBuffer                                           m_poolMaterialIDs   { };            //!< A pool of material IDs for each instance, used for accessing the instance-specific material.
        GLuint                                                  m_poolTransforms    { 0 };          //!< A pool of model and PVM transformation matrices, used in instanced rendering.
        
        float                                                   m_aspectRatio       { 0.f };        //!< The calculated aspect ratio of the foreground resolution for the application.

        std::shared_ptr<const SceneModel::Context>              m_scene             { nullptr };    //!< The sponza scene containing instance and camera information.
        std::vector<std::pair<SceneModel::MeshId, Mesh*>>       m_meshes            { };            //!< A container of MeshId and Mesh pairs, used in instance-based rendering of meshes in the scene.
        std::unordered_map<SceneModel::MaterialId, MaterialID>  m_materialIDs       { };            //!< A map containing each material used for rendering.

        bool                                                    m_wireframeMode     { false };      //!< Causes the camera to show a wireframe around meshes nearby.
        unsigned int                                            m_wireframeType     { 0 };          //!< Allows the user to cycle through point, spot and directional mode.

        #pragma endregion
};

#endif // _MY_VIEW_