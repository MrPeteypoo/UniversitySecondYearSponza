#include "Mesh.hpp"



// STL headers.
#include <utility>



#pragma region Constructors

Mesh::Mesh (Mesh&& move)
{
    *this = std::move (move);
}


Mesh& Mesh::operator= (Mesh&& move)
{
    // Avoid moving self to self.
    if (this != &move)
    {
        vao             = std::move (move.vao);
        vboVertices     = std::move (move.vboVertices);
        vboTransforms   = std::move (move.vboTransforms);
        vboElements     = std::move (move.vboElements);
        elementCount    = std::move (move.elementCount);
    }

    return *this;
}

#pragma endregion