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
        unused          = std::move (move.unused);
        specularColour  = std::move (move.specularColour);
        shininess       = std::move (move.shininess);
    }

    return *this;
}

#pragma endregion