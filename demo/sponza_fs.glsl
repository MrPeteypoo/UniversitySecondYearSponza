#version 330

#define MAX_LIGHTS 20


/// A structure containing information regarding to a light source in the scene. Because of the std140 layout rules of being 128-bit aligned
/// we need to be creative and combine vec3's with an additional attribute to save memory.
struct Light
{
    vec3    position;       //!< The world position of the light in the scene.
    float   type;           //!< The type of lighting algorithm to use. 0 means point light, 1 means spot light and 2 means directional light.
    
    vec3    direction;      //!< The direction of the light.
    float   coneAngle;      //!< The angle of the light in degrees.
    
    vec3    colour;         //!< The un-attenuated colour of the light. 
    float   concentration;  //!< How concentrated spot lights are, this effects distance attenuation.

    float   aConstant;      //!< The constant co-efficient for the attenutation formula.
    float   aLinear;        //!< The linear co-efficient for the attenuation formula.
    float   aQuadratic;     //!< The quadratic co-efficient for the attenuation formula.
    bool    emitWireframe;  //!< Determines whether the light should emit a wireframe onto surfaces.
};


/// The uniform buffer scene specific information.
layout (std140) uniform scene
{
    mat4    projection;     //!< The projection transform which establishes the perspective of the vertex.
    mat4    view;           //!< The view transform representing where the camera is looking.

    vec3    cameraPosition; //!< Contains the position of the camera in world space.
    vec3    ambience;       //!< The ambient lighting in the scene.
};


/// The uniform buffer containing lighting data.
layout (std140) uniform lighting
{
    int     numLights;			//!< The number of lights in currently in use.
    Light   lights[MAX_LIGHTS]; //!< The lighting data of each light in the scene.
};


        uniform sampler2DArray  textures;       //!< The array of textures in the scene.
        uniform samplerBuffer   materials;      //!< A texture buffer filled with the required diffuse and specular properties for the material.
        uniform isamplerBuffer  materialIDs;    //!< A buffer containing the ID of the material for the instance to fetch from the materials buffer.

        in      vec3            worldPosition;  //!< The fragments position vector in world space.
        in      vec3            worldNormal;    //!< The fragments normal vector in world space.
        in      vec3            baryPoint;      //!< The barycentric co-ordinate of the current fragment, useful for wireframe rendering.
        in      vec2            texturePoint;   //!< The interpolated co-ordinate to use for the texture sampler.
flat    in      int             instanceID;     //!< Used in fetching instance-specific data from the uniforms.


        out     vec4            fragmentColour; //!< The computed output colour of this particular pixel;


/// Updates the ambient, diffuse and specular colours from the materialTBO for this fragment.
void obtainMaterialProperties();

/// Calculates the lighting from a given light. Q should be the world position of the surface. N should be the world normal direction of the surface.
/// V should be the direction of the surface to the viewer.
/// Returns the attenuated lighting calculated by the material and light properties.
vec3 processLight (const Light light, const vec3 Q, const vec3 N, const vec3 V);

/// Calculates the attenuation value of a point light based on the given distance. The distance of the fragment from the light is represented by dist.
/// Returns an attenuation value ranging from 0 to 1.
float pointLightAttenuation (const Light light, const float dist);

/// Calculates the luminance attenuation value of a spot light based on the given distance. L is the surface-to-light unit direction vector.
/// The distance of the fragment from the light is represented by dist.
/// Returns an attenuation value ranging from 0 to 1.
float spotLightLuminanceAttenuation (const Light light, const vec3 L, const float dist);

/// Calculates the cone attenuation value of a spot light using angle information of a given light. L is the s
/// Returns an attenuation value ranging from 0 to 1.
float spotLightConeAttenuation (const Light light, const vec3 L);

/// Calculates the diffuse and specular lighting to be applied based on the given colour. L is the surface-to-light unit direction vector.
/// N should be the world normal direction of the surface. V is the surface-to-view unit direction vector. The luminance of the lighting
/// is represented by the lambertian value.
/// Returns the calculated diffuse and specular lighting with the given colour applied.
vec3 calculateLighting (const vec3 L, const vec3 N, const vec3 V, const vec3 colour, const float lambertian);


/// Using interpolated barycentric co-ordinates passed through by the vertex shader this function calculates the wireframe
/// colour of the current fragment. The base colour of the wireframe is specified with wireColour.
/// Returns a colour intensity to represent a line on the wireframe, black if the fragment isn't part of a line.
vec3 wireframe (const vec3 wireColour);


// Phong reflection model: I = Ia Ka + sum[0-n] Il,n (Kd (Ln.N) + Ks pow ((Rn.V), p))
// Ia   = Ambient scene light.
// Ka   = Ambient map.
// Il,n = Current light intensity.
// Kd   = Diffuse colour.
// Ks   = Specular colour.
// Ln.N = Dot product of direction to current light and normal.
// Rn.V = Dot product of the reflected light direction from the normal and the direction to the viewer.
// p    = The specular shininess factor.

/// Contains the properties of the material to be applied to the current fragment.
struct Material
{
    vec3 ambientMap;    //!< The ambient colour to apply to the fragment.
    vec3 texture;       //!< The texture colour, if any to apply.
    vec3 diffuse;       //!< The standard diffuse colour.
    vec3 specular;      //!< The specular colour.
    float shininess;    //!< How shiny the surface is.
} material;


void main()
{
    // Ensure we're using the correct colours.
    obtainMaterialProperties();

    // Calculate the required static shading vectors once per fragment instead of once per light.
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
    vec3 phong = ambience * material.ambientMap + lighting;
    
    // Output the calculated fragment colour.
    fragmentColour = vec4 (phong, 1.0);
}


int obtainMaterialID()
{
    /// This really isn't a nice way to do this, in future I will avoid this and just make the material ID an instance-specific 
    /// vertex attribute. However the alternative method is used here which features a samplerBuffer object and we then obtain
    /// the correct attribute using texelFetch().

    // Each instance is allocated 4-bytes of data for the material ID. We calculate the row by dividing the instance ID
    // by 4 and then the column by calculating the remainder.
    ivec4 idRow = texelFetch (materialIDs, instanceID / 4);

    switch (instanceID % 4)
    {
        case 0:
            return idRow.r;

        case 1:
            return idRow.g;

        case 2:
            return idRow.b;

        case 3:
            return idRow.a;
    }
}

void obtainMaterialProperties()
{
    // We can use the instance ID to reconstruct the diffuse and specular colours from the RGBA material buffer.
    int materialID      = obtainMaterialID();

    // Each material is allocated 16 bytes of data for the diffuse colour and 16 bytes for the specular colour.
    vec4 diffusePart    = texelFetch (materials, materialID);
    vec4 specularPart   = texelFetch (materials, materialID + 1);
    
    // The RGB values of the diffuse part are the diffuse colour.
    material.diffuse    = diffusePart.rgb;

    // The alpha of the diffuse part represents the texture to use for the ambient map. -1 == no texture.
    if (diffusePart.a >= 0.0)
    {
        material.texture    = texture (textures, vec3 (texturePoint, diffusePart.a)).rgb;
        material.ambientMap = material.texture;
    }

    // Use the diffuse colour for the ambient map and don't apply an extra texture colour.
    else
    {
        material.texture    = vec3 (1.0);
        material.ambientMap = material.diffuse;
    }
    
    // The RGB values of the specular part is the specular colour.    
    material.specular   = specularPart.rgb;
    
    // The alpha value of the specular part is the shininess value.
    material.shininess  = specularPart.a;
}


vec3 processLight (const Light light, const vec3 Q, const vec3 N, const vec3 V)
{
    // Prepare our accumulator.
    vec3 lighting   = vec3 (0.0);

    // Calculate L and the lambertian value to check if we need to actually add anything.
    float dist      = length (light.position - Q);
    vec3 L          = (light.position - Q) / dist;

    // If the lambertian is zero we don't need to add any lighting.
    float lambertian = max (dot (L, N), 0);

    if (lambertian > 0)
    {
        // We need to calculate the attenuation now.
        float attenuation = 1.0;

        // For the light type 0 means point, 1 means spot and 2 means directional.
        switch (int (light.type))
        {
            // Point light.
            case 0:
                attenuation *= pointLightAttenuation (light, dist);
                break;

            // Spot light.
            case 1:
                attenuation *= spotLightLuminanceAttenuation (light, L, dist);
                attenuation *= spotLightConeAttenuation (light, L);
                break;

            // Directional lights don't have attenuation.
            default:
                break;
        }
        
        if (attenuation > 0.0)
        {		
            // Booleans don't seem to translate to the UBO accurately.
            if (!light.emitWireframe)
            {
                // Calculate the final colour of the light. 
                vec3 attenuatedColour = light.colour * attenuation;

                // Increase the lighting to apply to the current fragment.
                lighting += calculateLighting (L, N, V, attenuatedColour, lambertian);
            }

            // Enable the wireframe view!
            else
            {
                // Create the wireframe and smoothstep with the calculated attenuation. This creates a really 
				// smooth transition between the standard Phong shading and the wireframe emissive shading!
                lighting += wireframe (light.colour) * smoothstep (0.0, 1.0, attenuation);
            }
        }
    }

    // Return the accumulated lighting.
    return lighting;
}


float pointLightAttenuation (const Light light, const float dist)
{
    // We need to construct Ci *= 1 / (Kc + Kl * d + Kq * d * d).
    float attenuation = 1.0 / (light.aConstant + light.aLinear * dist + light.aQuadratic * dist * dist);

    return attenuation;    
}


float spotLightLuminanceAttenuation (const Light light, const vec3 L, const float dist)
{    
    // We need to construct Ci *= (pow (max {-R.L, 0}), p) / (Kc + kl * d + Kq * d * d).
    float lighting      = max (dot (-light.direction, L), 0);

    float numerator     = pow (lighting, light.concentration);

    float denominator   = light.aConstant + light.aLinear * dist + light.aQuadratic * dist * dist;
    
    // Return the final calculation.
    return numerator / denominator;
}


float spotLightConeAttenuation (const Light light, const vec3 L)
{
    // Cone attenuation is: fs := aocs ((S.D)) > angle / 2. S = light to surface direction, D = light direction.
    const vec3 surface  = -L;
    float lightAngle    = degrees (acos (max (dot (surface, light.direction), 0)));

    // Determine the half angle.
    float halfAngle     = light.coneAngle / 2;

    // Attenuate using smoothstep. Don't cut off at zero, maintains spotlight look.
    float attenuation   = lightAngle <= halfAngle ? smoothstep (1.0, 0.75, lightAngle / halfAngle) : 0;
    
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
        diffuseLighting = material.diffuse * material.texture * lambertian;

        // Calculate the specular lighting.
        if (material.shininess > 0)
        {
            // We need to reflect the direction from the surface to the light for the specular calculations.
            vec3 R = reflect (L, N);
            
            // Finally use Kspecular = S * pow (-R.V, shininess) for the specular formula.
            specularLighting = material.specular * pow (max (dot (-R, V), 0), material.shininess);
        }
    }

    return colour * (diffuseLighting + specularLighting);
}


vec3 wireframe (const vec3 wireColour)
{
    /// This code is taken from a very useful blog post. Credit to Florian Boesch for such simple code.
    /// Boesch, F. (2012) Easy wireframe display with barycentric coordinates. 
    /// Available at: http://codeflow.org/entries/2012/aug/02/easy-wireframe-display-with-barycentric-coordinates/ (Accessed: 01/02/2015).

    /// I chose to implement the wireframe in this manner because it allows me to only apply the wire colour so that the normal surface/material
    /// colour can be used to fill in the frame. This looks really nice and allows us to smoothly interpolate between wireframe and Phong shading.
    /// This gives the scene the appearance of the wireframe and emissive light slowly fading out.
        
    // Determine how much of an edge exists at the interpolated barycentric point.
    vec3 d              = fwidth (baryPoint);
    vec3 a3             = smoothstep (vec3 (0.0), d * 1.5, baryPoint);
    float edgeFactor    = min (min (a3.x, a3.y), a3.z);

    // Mix an intense white and black colour based on how much of an edge exists.
    return wireColour * mix (vec3 (1.0), vec3 (0.0), edgeFactor);
}