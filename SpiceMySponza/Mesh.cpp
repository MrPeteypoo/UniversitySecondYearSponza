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
        vboPosition     = std::move (move.vboPosition);
        vboNormal       = std::move (move.vboNormal);
        vboElement      = std::move (move.vboElement);
        elementCount    = std::move (move.elementCount);
    }

    return *this;
}

#pragma endregion