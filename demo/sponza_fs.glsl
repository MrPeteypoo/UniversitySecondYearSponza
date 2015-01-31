/// <summary> The fragment shader used in SpiceMy Sponza. The shader implements the Phong reflection model. </summary>
/// <namespace> GLSL::FRAGMENT </namespace>

#version 330
#define MAX_LIGHTS 20


/// <summary>
/// A structure containing information regarding to a light source in the scene.
/// </summary>
struct Light
{
    vec3    position;       //!< The world position of the light in the scene.
    vec3    direction;      //!< The direction of the light.
    vec3    colour;         //!< The un-attenuated colour of the light.

    float   concentration;  //!< How concentrated the beam of the spot light is.
    float   coneAngle;      //!< The angle of the light cone in degrees.

    float   cConstant;      //!< The constant co-efficient for the attenutation formula.
    float   cLinear;        //!< The linear co-efficient for the attenuation formula.
    float   cQuadratic;     //!< The quadratic co-efficient for the attenuation formula.
};


/// <summary> The uniform buffer scene specific information. </summary>
layout (std140) uniform scene
{
    mat4    projection;         //!< The projection transform which establishes the perspective of the vertex.
    mat4    view;               //!< The view transform representing where the camera is looking.

    vec3    cameraPosition;     //!< Contains the position of the camera in world space.
    vec3    ambience;           //!< The ambient lighting in the scene.
};


/// <summary> The uniform buffer containing lighting data. </summary>
layout (std140) uniform lighting
{
    int     numLights;           //!< The number of lights in use.
    Light   lights[MAX_LIGHTS]; //!< The lighting data of each light in the scene.
};


        uniform samplerBuffer   materialBuffer; //!< A texture buffer filled with the required diffuse and specular properties for the material.
        uniform sampler2DArray  textureArray;   //!< The desired texture to apply to the particular pixel.


        in      vec3            worldPosition;  //!< The fragments position vector in world space.
        in      vec3            worldNormal;    //!< The fragments normal vector in world space.
        in      vec3            baryPoint;      //!< The barycentric co-ordinate of the current fragment, useful for wireframe rendering.
        in      vec2            texturePoint;   //!< The interpolated co-ordinate to use for the texture sampler.
flat    in      int             instanceID;     //!< Used in fetching instance-specific data from the uniforms.


        out     vec4            fragmentColour; //!< The computed output colour of this particular pixel;


/// <summary> Updates the ambient, diffuse and specular colours from the materialTBO for this fragment. </summary>
void obtainMaterialProperties();

/// <summary> Calculates the lighting from a given spotlight. </summary>
/// <param name="light"> The light to use for the calculations. </param>
/// <param name="Q"> The world position of the surface. </param>
/// <param name="N"> The world normal of the surface. </param>
/// <param name="V"> The direction of the viewer to the surface. </param>
/// <param name="attenuate"> Whether any attenuation should be performed. </param>
vec3 spotLight (const Light light, const vec3 Q, const vec3 N, const vec3 V, const bool attenuate);

float spotLightLuminanceAttenuation (const Light light, const vec3 L, const float distance);
float spotLightConeAttenuation (const Light light, const vec3 L);


/// <summary> Calculates the diffuse and specular lighting to be applied based on the given colour. </summary>
/// <param name="L"> The direction pointing from the surface to the light. </param>
/// <param name="N"> The normal pointing away from the surface. </param>
/// <param name="V"> The normal pointing from the surface to the viewer. </param>
/// <param name="colour"> The colour to modify the lighting with. </param>
/// <param name="lambertian"> The luminance value to apply to the diffuse colour. </param>
/// <returns> The calculated diffuse and specular lighting with the given colour applied. </returns>
vec3 calculateLighting (const vec3 L, const vec3 N, const vec3 V, const vec3 colour, const float lambertian);

vec3 wireframe();


//float spotLightAttenuation (const Light light, const vec3 L, const float distance, const unsigned int concentration);

// Phong reflection model: I = Ia Ka + sum[0-n] Il,n (Kd (Li.N) + Ks (Li.Rv))
// Ia   = Ambient scene light.
// Ka   = Ambient map.
// Il,n = Current light colour.
// Kd   = Diffuse co-efficient.
// Ks   = Specular co-efficient.

vec3 ambientMap     = vec3 (1.0, 1.0, 1.0);
vec3 textureColour  = vec3 (1.0, 1.0, 1.0);
vec3 diffuse        = vec3 (1.0, 1.0, 1.0);
vec3 specular       = vec3 (1.0, 1.0, 1.0);
float shininess     = 16.0;


void main()
{
    // Ensure we're using the correct colours.
    obtainMaterialProperties();

    // Parameters.
    vec3 Q = worldPosition;
    vec3 N = normalize (worldNormal);
    vec3 V = normalize (cameraPosition - Q);

    // Shade each light.
    vec3 lighting = vec3 (0.0, 0.0, 0.0);

    // Run through each spotlight, accumlating the diffuse and specular from the fragment.
    for (int i = 0; i < numLights; ++i)
    {
        lighting += spotLight (lights[i], Q, N, V, true);
    }
    
    // Put the equation together and we get....
    vec3 phong = ambience * ambientMap + lighting;
    
    // Output the calculated fragment colour.
    fragmentColour = vec4 (phong, 1.0);
}


void obtainMaterialProperties()
{
    // We can use the instance ID to reconstruct the diffuse and specular colours from the RGBA material buffer.
    // Each instanceID is allocated 16 bytes of data for the diffuse colour and 16 bytes for the specular colour.
    vec4 diffusePart    = texelFetch (materialBuffer, instanceID * 2);
    vec4 specularPart   = texelFetch (materialBuffer, instanceID * 2 + 1);
    
    // The RGB values of the diffuse part are the diffuse colour.
    diffuse = vec3 (diffusePart.r, diffusePart.g, diffusePart.b);

    // The alpha of the diffuse part represents the texture to use for the ambient map. -1 == no texture.
    if (diffusePart.a >= 0.0)
    {
        textureColour = texture (textureArray, vec3 (texturePoint, diffusePart.a)).rgb;
        ambientMap = textureColour;
    }

    else
    {
        // Use the diffuse colour for the ambient map and don't apply an extra texture colour.
        textureColour = vec3 (1.0, 1.0, 1.0);
        ambientMap = diffuse;
    }
    
    // The RGB values of the specular part is the specular colour.    
    specular = vec3 (specularPart.r, specularPart.g, specularPart.b);
    
    // The alpha value of the specular part is the shininess value.
    shininess = specularPart.a;
}


vec3 spotLight (const Light light, const vec3 Q, const vec3 N, const vec3 V, const bool attenuate)
{
    // Prepare our accumulator.
    vec3 lighting   = vec3 (0.0, 0.0, 0.0);

    // Calculate L and the lambertian value to check if we need to actually add anything.
    float distance  = length (light.position - Q);
    vec3 L          = (light.position - Q) / distance;

    // If the lambertian isn't more than zero we don't need to add any lighting.
    float lambertian = max (dot (L, N), 0);

    if (lambertian > 0)
    {
        // Start by attenuating the light if necessary.
        vec3 attenuatedColour = vec3 (1.0, 1.0, 1.0);
        //attenuatedColour = light.colour;

        if (attenuate)
        {
            //attenuatedColour *= spotLightLuminanceAttenuation (light, L, distance);
            //attenuatedColour *= spotLightConeAttenuation (light, L);
        }

        // Check if any light still exists.
        lighting += calculateLighting (L, N, V, attenuatedColour, lambertian);
    }

    // Return the accumulated lighting.
    return lighting;
}


float spotLightLuminanceAttenuation (const Light light, const vec3 L, const float distance)
{    
    // We need to construct C = (pow (max {-R.L, 0}), p) / (Kc + kl * d + Kq * d * d).
    float lighting      = max (dot (-light.direction, L), 0);

    float numerator     = pow (lighting, light.concentration);

    float denominator   = light.cConstant + light.cLinear * distance + light.cQuadratic * distance * distance;
    
    // Return the final calculation.
    return numerator / denominator;
}


float spotLightConeAttenuation (const Light light, const vec3 L)
{
    // Cone attenuation is: fs := (S.D) > cos (c). S = -L, D = light direction.
    vec3  surface       = -L;
    float coneFactor    = max (dot (surface, light.direction), 0);
    float halfAngle     = cos (light.coneAngle / 2);

    // Only return the cone attenuation factor if it is more than the cosine of the half angle.
    return coneFactor > halfAngle ? coneFactor : 0;
}


vec3 calculateLighting (const vec3 L, const vec3 N, const vec3 V, const vec3 colour, const float lambertian)
{ 
    // Create the variables we'll be modifying.
    vec3 diffuseLighting    = vec3 (0.0, 0.0, 0.0);
    vec3 specularLighting   = vec3 (0.0, 0.0, 0.0);

    if (lambertian > 0)
    {
        // Diffuse is easy, apply the texture and base colour with the given luminance.
        diffuseLighting = diffuse * textureColour * lambertian;

        // Calculate the specular lighting.
        if (shininess > 0)
        {
            // We need to reflect the direction from the surface to the light for the specular calculations.
            vec3 R = reflect (L, N);
            
            // Finally use Kspecular = pow (V.R, shininess) for the specular formula.
            specularLighting = specular * pow (max (dot (V, -R), 0), shininess);
        }
    }

    return colour * (diffuseLighting + specularLighting);
}
/*
vec3 wireframe()
{
    // Determine how much of an edge exists at the interpolated barycentric point.
    vec3 d              = fwidth (baryPoint);
    vec3 a3             = smoothstep (vec3 (0.0), d * 1.5, baryPoint);
    float edgeFactor    = min (min (a3.x, a3.y), a3.z);

    // Return the desired wireframe addition.
    return mix (vec3 (0.1), vec3 (0.0), edgeFactor);
}

/// <summary> Can be used to colour the scene using red, green or and blue triangles. </summary>
/// <returns> A single colour based on the primitive ID of the current triangle. </returns>
vec3 primitiveColour();

/// <summary> Calculates the attenuation value of a point light based on the given distance and range. </summary>
/// <param name="distance"> The distance of the fragment from the light. </param>
/// <param name="range"> The range of the point light. Light shall not extend beyond this value. </param>
float pointLightAttenuation (const float distance, const float range, bool useSmoothstep);


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
    const float kl = 0.005;
    float kq = 1.0 / (range * range * 0.01);
    
    // Calculate the final attenuation value.
    float attenuation = 1.0 / (kc + kl * distance + kq * distance * distance);

    return attenuation;    
}*/