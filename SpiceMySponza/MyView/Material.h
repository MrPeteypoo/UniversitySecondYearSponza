#pragma once

#if !defined    _MY_VIEW_MATERIAL_
#define         _MY_VIEW_MATERIAL_


// Engine headers.
#include <glm/gtc/type_ptr.hpp>


// Personal headers.
#include <MyView/MyView.h>


/// <summary> 
/// A basic material structure which stores the diffuse and specular properties of an instance as stored in a texture buffer.
/// </summary>
struct MyView::Material final
{
    #pragma region Implementation data

    glm::vec3   diffuseColour   { 0, 0, 0 };    //!< The diffuse colour of the material.
    float       unused          { 1.f };        //!< An unused 4-byte buffer. This is because the material will be stored in vec4 segments by texture buffers.
    glm::vec3   specularColour  { 1, 1, 1 };    //!< The specular colour of the material.
    float       shininess       { 0.f };        //!< The shininess factor of the specular colour.

    #pragma endregion

    #pragma region Constructors and destructor

    Material()                                  = default;
    Material (const Material& copy)             = default;
    Material& operator= (const Material& copy)  = default;
    ~Material()                                 = default;

    Material (Material&& move);
    Material& operator= (Material&& move);

    #pragma endregion
};

#endif // _MY_VIEW_MATERIAL_