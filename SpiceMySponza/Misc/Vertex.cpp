#include "Vertex.h"



// STL headers.
#include <utility>



#pragma region Constructors

Vertex::Vertex (Vertex&& move)
{
    *this = std::move (move);
}


Vertex& Vertex::operator= (Vertex&& move)
{
    // Avoid moving self to self.
    if (this != &move)
    {
        position        = std::move (move.position);
        normal          = std::move (move.normal);
        baryPoint       = std::move (move.baryPoint);
        texturePoint    = std::move (move.texturePoint);
    }

    return *this;
}

#pragma endregion