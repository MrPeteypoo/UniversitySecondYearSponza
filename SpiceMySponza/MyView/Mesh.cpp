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
        verticesIndex   = std::move (move.verticesIndex);
        elementsOffset  = std::move (move.elementsOffset);
        elementCount    = std::move (move.elementCount);
    }

    return *this;
}

#pragma endregion