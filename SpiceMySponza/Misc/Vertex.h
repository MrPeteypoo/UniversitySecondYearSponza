#pragma once

#if !defined    _VERTEX_
#define         _VERTEX_


// Engine headers.
#include <glm/gtc/type_ptr.hpp>


/// <summary> 
/// An object with a position, normal vector and texture co-ordinate to represent a single vertex.
/// </summary>
struct Vertex final
{
    #pragma region Implementation data

    glm::vec3   position        { 0 },  //!< The position vector of the vertex.
                normal          { 0 };  //!< The normal vector for the vertex.
    glm::vec2   texturePoint    { 0 };  //!< The texture co-ordinate of the vertex.

    #pragma endregion

    #pragma region Constructors and destructor

    Vertex()                                = default;
    Vertex (const Vertex& copy)             = default;
    Vertex& operator= (const Vertex& copy)  = default;
    ~Vertex()                               = default;

    Vertex (const glm::vec3& pos, const glm::vec3& norm, const glm::vec2& texture)  
        : position (pos), normal (norm), texturePoint (texture) { }
    
    Vertex (Vertex&& move);
    Vertex& operator= (Vertex&& move);

    #pragma endregion
};

#endif // _VERTEX_