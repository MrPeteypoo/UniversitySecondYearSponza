#include "Material.h"



// STL headers.
#include <utility>



#pragma region Constructors

MyView::Material::Material (Material&& move)
{
    *this = std::move (move);
}


MyView::Material& MyView::Material::operator= (Material&& move)
{
    // Avoid moving self to self.
    if (this != &move)
    {
        diffuseColour   = std::move (move.diffuseColour);
        textureID       = move.textureID;
        specularColour  = std::move (move.specularColour);
        shininess       = move.shininess;

        // Reset primitives.
        move.textureID  = 0.f;
        move.shininess  = 0.f;
    }

    return *this;
}

#pragma endregion