#include "Vertex.hpp"



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
        texturePoint    = std::move (move.texturePoint);
    }

    return *this;
}

#pragma endregion