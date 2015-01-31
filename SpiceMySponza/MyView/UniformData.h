#pragma once

#if !defined    MY_VIEW_UNIFORM_DATA_
#define         MY_VIEW_UNIFORM_DATA_
#define         MAX_LIGHTS  (unsigned int) 20


// Engine headers.
#include <glm/gtc/type_ptr.hpp>


// Personal headers.
#include <MyView/MyView.h>


// Forward declarations.
namespace SceneModel { class Light; }


// Using declarations.
using GLuint    = unsigned int;


// We shall manage the byte alignment ourselves since it needs to be aligned properly for the GPU.
#pragma pack (push, 1)


/// <summary> 
/// A basic class used for writing to a Uniform Buffer Object which represents shader information.
/// </summary>
class MyView::UniformData final
{
    public:

        #pragma region Constructors and destructor

        UniformData()                                       = default;
        UniformData (const UniformData& copy)               = default;
        UniformData& operator= (const UniformData& copy)    = default;
        ~UniformData()                                      = default;

        UniformData (UniformData&& move);
        UniformData& operator= (UniformData&& move);

        #pragma endregion

        #pragma region Getters and setters

        const glm::mat4& getProjectionMatrix() const            { return m_projection; }
        const glm::mat4& getViewMatrix() const                  { return m_view; }
        const glm::vec3& getCameraPosition() const              { return m_cameraPosition; }
        const glm::vec3& getAmbientColour() const               { return m_ambience; }
        int getLightCount() const                               { return m_numLights; }
        
        /// <summary> Sets the projection transformation matrix. </summary>
        void setProjectionMatrix (const glm::mat4& projection)  { m_projection = projection; }
        
        /// <summary> Sets the view transformation matrix. </summary>
        void setViewMatrix (const glm::mat4& view)              { m_view = view; }
        
        /// <summary> Sets the camera position. </summary>
        /// <param name="position"> The position should be in world space. </param>
        void setCameraPosition (const glm::vec3& position)      { m_cameraPosition = position; }
        
        /// <summary> Sets the ambient scene colour to be used during shading. </summary>
        /// <param name="colour"> RGB values should range from 0 to 1. </param>
        void setAmbientColour (const glm::vec3& colour)         { m_ambience = colour; }
        
        /// <summary> Sets the number of lights to render. </summary>
        /// <param name="count"> Lights 0 to (count - 1) will be rendered. </param>
        void setLightCount (const unsigned int count);
        
        /// <summary> Converts the desired light into shader-ready format. </summary>
        void setLight (const unsigned int index, const SceneModel::Light& sceneLight);

        #pragma endregion

        #pragma region Binding/offset information        

        /// <summary> Gets the binding block index for the scene UBO. </summary>
        static GLuint sceneBlock()      { return 0; }

        /// <summary> Calculate the offset for the scene UBO in bytes. </summary>
        static GLuint sceneOffset()     { return 0; }

        /// <summary> Calculates the size of the scene UBO in bytes. </summary>
        static GLuint sceneSize()       { return lightingOffset(); }

        /// <summary> Gets the binding block index for the lighting UBO. </summary>
        static GLuint lightingBlock()   { return 1; }

        /// <summary> Calculates the offset for the lighting UBO in bytes. </summary>
        static GLuint lightingOffset()  { return sizeof (UniformData) - lightingSize(); }

        /// <summary> Calculates the size of the lighting UBO in bytes. </summary>
        static GLuint lightingSize()    { return sizeof (Light) * MAX_LIGHTS + sizeof (unsigned int); }

        #pragma endregion

    private:

        #pragma region Light structure

        /// <summary> 
        /// A basic light structure as expected by shaders.
        /// </summary>
        struct Light final
        {
            glm::vec3   position        { 1.f, 1.f, 1.f };  //!< The world position of the light in the scene.
            glm::vec3   direction       { 1.f, 1.f, 1.f };  //!< The direction of the light.
            glm::vec3   colour          { 1.f, 1.f, 1.f };  //!< The un-attenuated colour of the light.
            
            float       concentration   { 1.f };            //!< How concentrated the luminance of the spot light is.
            float       coneAngle       { 45.f };           //!< The angle of the light cone in degrees.

            float       cConstant       { 1.f };            //!< The constant co-efficient for the attenutation formula.
            float       cLinear         { 0.f };            //!< The linear co-efficient for the attenutation formula.
            float       cQuadratic      { 1.f };            //!< The quadratic co-efficient for the attenuation formula.

            Light()                                 = default;
            Light (const Light& copy)               = default;
            Light& operator= (const Light& copy)    = default;
            ~Light()                                = default;

            Light (Light&& move);
            Light& operator= (Light&& move);
        };

        #pragma endregion
    
        #pragma region Implementation data

        glm::mat4       m_projection            { 1.f };            //!< The project matrix used during the rendering of the current frame.
        glm::mat4       m_view                  { 1.f };            //!< The view matrix from the current cameras position and direction.

        glm::vec3       m_cameraPosition        { 0.f, 0.f, 0.f };  //!< The world-space position of the camera.
        glm::vec3       m_ambience              { 1.f, 1.f, 1.f };  //!< The ambient colour of the scene.

        float           unused[26];                                 //!< An unused block for 256-byte alignment.

        unsigned int    m_numLights             { 0 };              //!< The number of lights currently in the scene.
        Light           m_lights[MAX_LIGHTS]    { };                //!< Data for each light in the scene.

        #pragma endregion
};

// Undo the alignment.
#pragma pack (pop)

#endif // MY_VIEW_UNIFORM_DATA_