/// <summary> The fragment shader used in SpiceMy Sponza. The shader implements the Phong reflection model. </summary>
/// <namespace> GLSL::FRAGMENT </namespace>

#version 330
#define MAX_LIGHTS 20


/// <summary>
/// A structure containing information regarding to a light source in the scene. Because of the std140 layout rules of being 128-bit aligned
/// we need to be creative and combine vec3's with an additional attribute to save memory.
/// </summary>
struct Light
{
    vec4    positionType;           //!< The world position of the light in the scene. Alpha is the type of light; 0 means point light, 1 means spot light and 2 means directional light.
    vec4    directionAngle;         //!< The direction of the light. Alpha is the angle of the light in degrees.
    vec4    colourConcentration;    //!< The un-attenuated colour of the light. Alpha is the concentration of the light beam.

    float   aConstant;              //!< The constant co-efficient for the attenutation formula.
    float   aLinear;                //!< The linear co-efficient for the attenuation formula.
    float   aQuadratic;             //!< The quadratic co-efficient for the attenuation formula.
    int     emitWireframe;          //!< Determines whether the light should emit a wireframe onto surfaces.
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
    int     numLights;			//!< The number of lights in currently in use.
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

/// <summary> Calculates the lighting from a given light. </summary>
/// <param name="light"> The light to use for the calculations. </param>
/// <param name="Q"> The world position of the surface. </param>
/// <param name="N"> The world normal direction of the surface. </param>
/// <param name="V"> The direction of the surface to the viewer. </param>
/// <returns> Attenuated lighting calculated by the material and light properties. </returns>
vec3 processLight (const Light light, const vec3 Q, const vec3 N, const vec3 V);

/// <summary> Calculates the attenuation value of a point light based on the given distance. </summary>
/// <param name="light"> The light to obtain attenuation information from. </param>
/// <param name="distance"> The distance of the fragment from the light. </param>
/// <returns> An attenuation value ranging from 0 to 1. </returns>
float pointLightAttenuation (const Light light, const float distance);

/// <summary> Calculates the luminance attenuation value of a spot light based on the given distance. </summary>
/// <param name="light"> The light to obtain attenuation information from. </param>
/// <param name="L"> The surface-to-light unit direction vector required for the attenuation formula. </param>
/// <param name="distance"> The distance of the fragment from the light. </param>
/// <returns> An attenuation value ranging from 0 to 1. </returns>
float spotLightLuminanceAttenuation (const Light light, const vec3 L, const float distance);

/// <summary> Calculates the cone attenuation value of a spot light using angle information of a given light. </summary>
/// <param name="light"> The light to obtain directional and angular information from. </param>
/// <param name="L"> The surface-to-light unit direction vector required for the attenuation formula. </param>
/// <returns> An attenuation value ranging from 0 to 1. </returns>
float spotLightConeAttenuation (const Light light, const vec3 L);

/// <summary> Calculates the diffuse and specular lighting to be applied based on the given colour. </summary>
/// <param name="L"> The direction pointing from the surface to the light. </param>
/// <param name="N"> The normal pointing away from the surface. </param>
/// <param name="V"> The direction pointing from the surface to the viewer. </param>
/// <param name="colour"> The colour to modify the lighting with. </param>
/// <param name="lambertian"> The luminance value to apply to the diffuse colour. </param>
/// <returns> The calculated diffuse and specular lighting with the given colour applied. </returns>
vec3 calculateLighting (const vec3 L, const vec3 N, const vec3 V, const vec3 colour, const float lambertian);

vec3 wireframe();


// Phong reflection model: I = Ia Ka + sum[0-n] Il,n (Kd (Ln.N) + Ks (Ln.Rv))
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
    vec3 lighting = vec3 (0.0);

    // Run through each spotlight, accumlating the diffuse and specular from the fragment.
    for (int i = 0; i < numLights; ++i)
    {
        lighting += processLight (lights[i], Q, N, V);
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
    diffuse = diffusePart.rgb;

    // The alpha of the diffuse part represents the texture to use for the ambient map. -1 == no texture.
    if (diffusePart.a >= 0.0)
    {
        textureColour = texture (textureArray, vec3 (texturePoint, diffusePart.a)).rgb;
        ambientMap = textureColour;
    }

    else
    {
        // Use the diffuse colour for the ambient map and don't apply an extra texture colour.
        ambientMap = diffuse;
    }
    
    // The RGB values of the specular part is the specular colour.    
    specular = specularPart.rgb;
    
    // The alpha value of the specular part is the shininess value.
    shininess = specularPart.a;
}


vec3 processLight (const Light light, const vec3 Q, const vec3 N, const vec3 V)
{
    // Prepare our accumulator.
    vec3 lighting   = vec3 (0.0);

    // Calculate L and the lambertian value to check if we need to actually add anything.
    float distance  = length (light.positionType.xyz - Q);
    vec3 L          = (light.positionType.xyz - Q) / distance;

    // If the lambertian is zero we don't need to add any lighting.
    float lambertian = max (dot (L, N), 0);

    if (lambertian > 0)
    {
        // We need to calculate the attenuation now.
        float attenuation = 1.0;

        // For the light type 0 means point, 1 means spot and 2 means directional.
        switch (int (light.positionType.w))
        {
            // Point light.
            case 0:
                attenuation *= pointLightAttenuation (light, distance);
                break;

            // Spot light.
            case 1:
                attenuation *= spotLightLuminanceAttenuation (light, L, distance);
                attenuation *= spotLightConeAttenuation (light, L);
                break;

            // Directional lights don't have attenuation.
            default:
                break;
        }

        // Check if any light still exists.
        if (attenuation > 0)
        {
            // Booleans don't seem to translate to the UBO accurately.
            if (light.emitWireframe != 1)
            {
                // Calculate the final colour of the light. 
                vec3 attenuatedColour = light.colourConcentration.rgb * attenuation;

                // Increase the lighting to apply to the current fragment.
                lighting += calculateLighting (L, N, V, attenuatedColour, lambertian);
            }

            // Enable the wireframe view!
            else
            {
                // The materials should look emissive by using a white, unattenuated light.
                vec3 emissiveLighting = calculateLighting (L, N, V, vec3 (1), lambertian);
                
                // Blend the wireframe and emissive light together and smoothstep with the calculated attenuation.
                lighting += (emissiveLighting + wireframe()) * smoothstep (0.0, 1.0, attenuation);
            }
        }
    }

    // Return the accumulated lighting.
    return lighting;
}


float pointLightAttenuation (const Light light, const float distance)
{
    // We need to construct Ci *= 1 / (Kc + Kl * d + Kq * d * d).
    float attenuation = 1.0 / (light.aConstant + light.aLinear * distance + light.aQuadratic * distance * distance);

    return attenuation;    
}


float spotLightLuminanceAttenuation (const Light light, const vec3 L, const float distance)
{    
    // We need to construct Ci *= (pow (max {-R.L, 0}), p) / (Kc + kl * d + Kq * d * d).
    float lighting      = max (dot (-light.directionAngle.xyz, L), 0);

    float numerator     = pow (lighting, light.colourConcentration.a);

    float denominator   = light.aConstant + light.aLinear * distance + light.aQuadratic * distance * distance;
    
    // Return the final calculation.
    return numerator / denominator;
}


float spotLightConeAttenuation (const Light light, const vec3 L)
{
    // Cone attenuation is: fs := (S.D) > cos (c). S = light to surface direction, D = light direction.
    const vec3 surface  = -L;
    float coneFactor    = max (dot (surface, light.directionAngle.xyz), 0);

    // Determine the cosine of the half angle.
    float halfAngle     = cos (light.directionAngle.w / 2);

    // Attenuate using smoothstep.
    float attenuation   = smoothstep (0.0, 1.0, (coneFactor - halfAngle) / halfAngle);
    
    // Return the calculated attenuation factor.
    return attenuation;
}


vec3 calculateLighting (const vec3 L, const vec3 N, const vec3 V, const vec3 colour, const float lambertian)
{ 
    // Create the variables we'll be modifying.
    vec3 diffuseLighting    = vec3 (0.0);
    vec3 specularLighting   = vec3 (0.0);

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

vec3 wireframe()
{
    // This code is taken from a very useful blog post. Credit to Florian Boesch for such simple code.
    // Boesch, F. (2012) Easy wireframe display with barycentric coordinates. 
    // Available at: http://codeflow.org/entries/2012/aug/02/easy-wireframe-display-with-barycentric-coordinates/ (Accessed: 01/02/2015).
    
    // Determine how much of an edge exists at the interpolated barycentric point.
    vec3 d              = fwidth (baryPoint);
    vec3 a3             = smoothstep (vec3 (0.0), d * 1.5, baryPoint);
    float edgeFactor    = min (min (a3.x, a3.y), a3.z);

    // Return the desired wireframe addition.
    return mix (vec3 (3.0), vec3 (0.0), edgeFactor);
}