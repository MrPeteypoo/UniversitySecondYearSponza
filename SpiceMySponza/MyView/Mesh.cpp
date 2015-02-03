#include "Mesh.h"



// STL headers.
#include <utility>



#pragma region Constructors

MyView::Mesh::Mesh (Mesh&& move)
{
    *this = std::move (move);
}


MyView::Mesh& MyView::Mesh::operator= (Mesh&& move)
{
    // Avoid moving self to self.
    if (this != &move)
    {
        verticesIndex       = move.verticesIndex;
        elementsOffset      = std::move (move.elementsOffset);
        elementCount        = move.elementCount;

        // Reset primitives.
        move.verticesIndex  = 0;
        move.elementCount   = 0;
    }

    return *this;
}

#pragma endregion