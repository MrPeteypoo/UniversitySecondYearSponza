#pragma once

#if !defined    _MY_VIEW_MESH_
#define         _MY_VIEW_MESH_


// Using declarations.
using GLuint = unsigned int;


/// <summary> 
/// A basic mesh structure used to hold the required data for rendering a mesh using OpenGL. 
/// </summary>
struct Mesh final
{
    #pragma region Implementation data

    GLuint          vao             { 0 },  //!< The VertexArrayObject for the mesh.
                    vboPosition     { 0 },  //!< The VertexBufferObject for vertex positions.
                    vboNormal       { 0 },  //!< The VertexBufferObject for vertex normals.
                    vboTexture      { 0 },  //!< The VertexBufferObject for texture co-ordinates.  
                    vboElement      { 0 };  //!< The VertexBufferObject for the elements.
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