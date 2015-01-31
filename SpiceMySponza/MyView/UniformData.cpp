#include "UniformData.h"



// Engine headers.
#include <SceneModel/Light.hpp>



// Personal headers.
#include <Utility/Maths.h>



#pragma region Constructors

MyView::UniformData::UniformData (UniformData&& move)
{
    *this = std::move (move);
}


MyView::UniformData& MyView::UniformData::operator= (UniformData&& move)
{
    // Avoid moving self to self.
    if (this != &move)
    {
        m_projection        = std::move (move.m_projection);
        m_view              = std::move (move.m_view);
        
        m_cameraPosition    = std::move (move.m_cameraPosition);
        m_ambience          = std::move (move.m_ambience);

        m_numLights         = std::move (move.m_numLights);
        
        for (unsigned int i = 0; i < MAX_LIGHTS; ++i)
        {
            m_lights[i] = std::move (move.m_lights[i]);
        }
    }

    return *this;
}

#pragma endregion


#pragma region Setters

void MyView::UniformData::setLightCount (const unsigned int count)
{
    m_numLights = util::min (count, MAX_LIGHTS);
}


void MyView::UniformData::setLight (const unsigned int index, const SceneModel::Light& light)
{
    // Pre-condition: Index is valid.
    if (index < MAX_LIGHTS)
    {
        // Cache the light to be modified.
        auto& shaderLight = m_lights[index];

        // Move the data across.
        shaderLight.position    = light.getPosition();
        shaderLight.direction   = light.getDirection();
        shaderLight.coneAngle   = light.getConeAngleDegrees();
        shaderLight.cConstant   = light.getConstantDistanceAttenuationCoefficient();
        shaderLight.cQuadratic  = light.getQuadraticDistanceAttenuationCoefficient();
    }
}

#pragma endregion


#pragma region Light structure

MyView::UniformData::Light::Light (Light&& move)
{
    *this = std::move (move);
}


MyView::UniformData::Light& MyView::UniformData::Light::operator= (Light&& move)
{
    // Avoid moving self to self.
    if (this != &move)
    {
        position    = std::move (move.position);
        direction   = std::move (move.direction);
        colour      = std::move (move.colour);

        coneAngle   = std::move (move.coneAngle);
        cConstant   = std::move (move.cConstant);
        cLinear     = std::move (move.cLinear);
        cQuadratic  = std::move (move.cQuadratic);
    }

    return *this;
}

#pragma endregion