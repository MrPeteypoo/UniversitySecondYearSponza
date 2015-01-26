#pragma once

#if !defined    _MESH_
#define         _MESH_


// Using declarations.
using GLuint = unsigned int;


/// <summary> 
/// A basic mesh structure used to hold the required data for rendering a mesh using OpenGL. 
/// </summary>
struct Mesh final
{
    #pragma region Implementation data

    GLuint          vao             { 0 },  //!< The VertexArrayObject for the mesh.
                    vboVertices     { 0 },  //!< The interleaved VertexBufferObject with position vectors, normal vectors and texture co-ordinates.
                    vboTransforms   { 0 },  //!< A VBO with instanced model and PVM transform information.
                    vboElements     { 0 };  //!< The VertexBufferObject for the element indices.
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

#endif // _MESH_