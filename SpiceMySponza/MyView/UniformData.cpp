#include "UniformData.h"



// Engine headers.
#include <SceneModel/Light.hpp>



// Personal headers.
#include <Utility/Maths.h>



#pragma region Light structure

Light::Light (Light&& move)
{
    *this = std::move (move);
}


Light& Light::operator= (Light&& move)
{
    // Avoid moving self to self.
    if (this != &move)
    {
        // Move all data across.
        position        = std::move (move.position);
        type            = std::move (move.type);

        direction       = std::move (move.direction);
        coneAngle       = std::move (move.coneAngle);

        colour          = std::move (move.colour);
        concentration   = std::move (move.concentration);

        aConstant       = std::move (move.aConstant);
        aLinear         = std::move (move.aLinear);
        aQuadratic      = std::move (move.aQuadratic);
        emitWireframe   = std::move (move.emitWireframe);

        // Reset standard data type.
        move.setType (LightType::Point);
        move.coneAngle      = 0.f;
        move.concentration  = 0.f;
        move.aConstant      = 0.f;
        move.aLinear        = 0.f;
        move.aQuadratic     = 0.f;
        move.emitWireframe  = 0;
    }

    return *this;
}

#pragma endregion


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
        // Move the data across.
        m_projection        = std::move (move.m_projection);
        m_view              = std::move (move.m_view);
        
        m_cameraPosition    = std::move (move.m_cameraPosition);
        m_ambience          = std::move (move.m_ambience);
        
        m_numLights         = std::move (move.m_numLights);

        for (unsigned int i = 0; i < MAX_LIGHTS; ++i)
        {
            m_lights[i] = std::move (move.m_lights[i]);
        }

        // Reset primitive data types.
        m_numLights         = 0;
    }

    return *this;
}

#pragma endregion


#pragma region Setters

void MyView::UniformData::setLightCount (const int count)
{
    m_numLights = util::clamp (count, 0, MAX_LIGHTS);
}


void MyView::UniformData::setLight (const int index, const SceneModel::Light& light, const LightType type)
{
    // Pre-condition: Index is valid.
    if (index < MAX_LIGHTS && index >= 0)
    {
        // Cache the light to be modified.
        auto& shaderLight = m_lights[index];

        // Move the data across.
        shaderLight.setType (type);
        shaderLight.position    = light.getPosition();

        shaderLight.direction   = light.getDirection();
        shaderLight.coneAngle   = light.getConeAngleDegrees();

        shaderLight.aConstant   = light.getConstantDistanceAttenuationCoefficient();
        shaderLight.aQuadratic  = light.getQuadraticDistanceAttenuationCoefficient();
    }
}

void MyView::UniformData::setLight (const int index, const Light& light)
{
    // Pre-condition: Index is valid.
    if (index < MAX_LIGHTS && index >= 0)
    {
        m_lights[index] = light;   
    }
}

#pragma endregion