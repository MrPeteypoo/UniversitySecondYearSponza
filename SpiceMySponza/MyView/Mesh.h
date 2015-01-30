#pragma once

#if !defined    _MY_VIEW_MESH_
#define         _MY_VIEW_MESH_


// Personal headers.
#include <MyView/MyView.h>


// Using declarations.
using GLint  = int;
using GLuint = unsigned int;


/// <summary> 
/// A basic mesh structure used to hold the required data for rendering a mesh using OpenGL. 
/// </summary>
struct MyView::Mesh final
{
    #pragma region Implementation data

    GLint           verticesIndex   { 0 };  //!< The index of a VBO where the vertices for the mesh begin.
    GLuint          elementsOffset  { 0 };  //!< An offset in bytes used to draw the mesh in the scene.
    unsigned int    elementCount    { 0 };  //!< Indicates how many elements there are.

    #pragma endregion

    #pragma region Constructors and destructor

    Mesh()                              = default;
    Mesh (const Mesh& copy)             = default;
    Mesh& operator= (const Mesh& copy)  = default;
    ~Mesh()                             = default;

    Mesh (Mesh&& move);
    Mesh& operator= (Mesh&& move);

    #pragma endregion
};

#endif // _MY_VIEW_MESH_