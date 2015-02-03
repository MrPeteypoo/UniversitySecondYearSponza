#pragma once

#if !defined    MY_VIEW_UNIFORM_DATA_
#define         MY_VIEW_UNIFORM_DATA_
#define         MAX_LIGHTS 20


// Engine headers.
#include <glm/gtc/type_ptr.hpp>


// Personal headers.
#include <MyView/MyView.h>


// Forward declarations.
namespace SceneModel { class Light; }


// Using declarations.
using GLuint    = unsigned int;


// We'll manage the data alignment by enforcing 4-byte alignment for all types.
#pragma pack (push, 4)


/// <summary>
/// A simply enumeration of the light type to be used. Currently directional lights aren't supported by the shader.
/// </summary>
enum class LightType : int
{
    Point       = 0,    //!< Point lights create a sphere of light at a given position.
    Spot        = 1,    //!< Spot lights beam light from a given position to a given direction.
    Directional = 2     //!< Directional lights apply scene-wide lighting from a given direction.
};


/// <summary> 
/// A basic light structure with the exact layout that the shaders expect.
/// </summary>
struct Light final
{
    glm::vec3   position        { 1.f };    //!< The world position of the light in the scene.
    float       type            { 0.f };    //!< Determines how the light information is visually applied in the scene.

    glm::vec3   direction       { 1.f };    //!< The direction of the light.
    float       coneAngle       { 90.f };   //!< The cone angle for spot lights.

    glm::vec3   colour          { 1.f };    //!< The un-attenuated colour of the light.
    float       concentration   { 7.f };    //!< How concentrated beam of a spot light is.

    float       aConstant       { 1.f };    //!< The constant co-efficient for the attenutation formula.
    float       aLinear         { 0.f };    //!< The linear co-efficient for the attenutation formula.
    float       aQuadratic      { 1.f };    //!< The quadratic co-efficient for the attenuation formula.
    int         emitWireframe   { 0 };      //!< Indicates whether the light should emit a wireframe onto surfaces.

    Light()                                 = default;
    Light (const Light& copy)               = default;
    Light& operator= (const Light& copy)    = default;
    ~Light()                                = default;

    Light (Light&& move);
    Light& operator= (Light&& move);

    /// <summary> Set the light type using the type-safe enumeration. </summary>
    void setType (const LightType lightType)    { type = static_cast<float> (lightType); }
};


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
        glm::vec3 getCameraPosition() const                     { return glm::vec3 (m_cameraPosition); }
        glm::vec3 getAmbientColour() const                      { return glm::vec3 (m_ambience); }
        int getLightCount() const                               { return m_numLights; }
        
        /// <summary> Sets the projection transformation matrix. </summary>
        void setProjectionMatrix (const glm::mat4& projection)  { m_projection = projection; }
        
        /// <summary> Sets the view transformation matrix. </summary>
        void setViewMatrix (const glm::mat4& view)              { m_view = view; }
        
        /// <summary> Sets the camera position. </summary>
        /// <param name="position"> The position should be in world space. </param>
        void setCameraPosition (const glm::vec3& position)      { m_cameraPosition = glm::vec4 (position, 0.f); }
        
        /// <summary> Sets the ambient scene colour to be used during shading. </summary>
        /// <param name="colour"> RGB values should range from 0 to 1. </param>
        void setAmbientColour (const glm::vec3& colour)         { m_ambience = glm::vec4 (colour, 1.f); }
        
        /// <summary> Sets the number of lights to render. </summary>
        /// <param name="count"> Lights 0 to (count - 1) will be rendered. </param>
        void setLightCount (const int count);
        
        /// <summary> Converts the desired SceneModel::Light into a shader-ready format. </summary>
        void setLight (const int index, const SceneModel::Light& sceneLight, const LightType type);

        /// <summary> Sets the light at the given index. </summary>
        void setLight (const int index, const Light& light);

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
        static GLuint lightingSize()    { return sizeof (Light) * MAX_LIGHTS + sizeof (glm::vec4); }

        #pragma endregion

    private:
    
        #pragma region Implementation data

        glm::mat4   m_projection            { 1.f };    //!< The project matrix used during the rendering of the current frame.
        glm::mat4   m_view                  { 1.f };    //!< The view matrix from the current cameras position and direction.

        glm::vec4   m_cameraPosition        { 0.f };    //!< The world-space position of the camera. The W value is padding required by the shader.
        glm::vec4   m_ambience              { 1.f };    //!< The ambient colour of the scene. The alpha value is padding required by the shader.

        float       m_unused[24];                       //!< An unused array for 256-byte alignment to the binding block.
        
        int         m_numLights             { 0 };      //!< The number of lights currently in the scene.
        float       m_alignment[3];                     //!< Align UniformData to 128-bits.
        
        Light       m_lights[MAX_LIGHTS]    { };        //!< Data for each light in the scene.


        #pragma endregion
};


// Undo the alignment.
#pragma pack (pop)

#endif // MY_VIEW_UNIFORM_DATA_