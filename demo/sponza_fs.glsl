/// <summary> The fragment shader used in SpiceMy Sponza. The shader implements the Phong reflection model. </summary>
/// <namespace> GLSL::FRAGMENT </namespace>

#version 330

uniform vec3        cameraPosition; //!< Contains the position of the camera in world space.
uniform sampler2D   textureSampler; //!< The desired texture to apply to the particular pixel.

in      vec3        worldPosition;  //!< The fragments position vector in world space.
in      vec3        worldNormal;    //!< The fragments normal vector in world space.
in      vec2        texturePoint;   //!< The interpolated co-ordinate to use for the texture sampler.    

out     vec4        fragmentColour; //!< The computed output colour of this particular pixel;


/// <summary> Can be used to colour the scene using red, green or and blue triangles. </summary>
/// <returns> A single colour based on the primitive ID of the current triangle. </returns>
vec3 primitiveColour();

/// <summary> Calculates the attenuation value of a point light based on the given distance and range. </summary>
/// <param name="distance"> The distance of the fragment from the light. </param>
/// <param name="range"> The range of the point light. Light shall not extend beyond this value. </param>
float pointLightAttenuation (const float distance, const float range, bool useSmoothstep);


//float spotLightAttenuation (const Light light, const vec3 L, const float distance, const unsigned int concentration);



void main()
{
    vec3 textureColour = texture (textureSampler, texturePoint).rgb;
    fragmentColour = vec4 (textureColour *  (0.5 + 0.5 * normalize (worldNormal)), 1.0);
}



vec3 primitiveColour()
{
    switch (gl_PrimitiveID % 3)
    {
        case 0:
            return vec3 (1.0, 0.0, 0.0);
        case 1:
            return vec3 (0.0, 1.0, 0.0);
        case 2:
            return vec3 (0.0, 0.0, 1.0);
    }
}




float pointLightAttenuation (const float distance, const float range, bool useSmoothstep)
{
    // Use smoothstep if desired.
    if (useSmoothstep)
    {
        return smoothstep (1.0, 0.0, distance / range);
    }

    // Start by calculating the attentuation constants.
    const float kc = 1.0;
    const float kl = 0.1;
    float kq = 1.0 / (range * range * 0.01);
    
    // Calculate the final attenuation value.
    float attenuation = 1.0 / (kc + kl * distance + kq * distance * distance);
    
    // Comment the next line for smoothstep.
    return attenuation;    
}

/*
float spotLightAttenuation (const Light light, const vec3 L, const float distance, const unsigned int concentration)
{
    const float kc = 1.0;
    const float kl = 0.1;
    float kq = 1.0 / (light.range * light.range * 1.0);
    
    float lighting = max (dot (-light.direction, L), 0);
    float numerator = pow (lighting, concentration);
    float denominator = kc + kl * distance + kq * distance * distance;
    
    return numerator / denominator;
    return numerator * smoothstep (1.0, 0.0, distance / light.range);
}*/