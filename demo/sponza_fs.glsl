/// <summary> The fragment shader used in SpiceMy Sponza. The shader implements the Phong reflection model. </summary>
/// <namespace> GLSL::FRAGMENT </namespace>

#version 330


#define MAX_LIGHTS 100


/// <summary>
/// A structure containing information regarding to a light source in the scene.
/// </summary>
struct Light
{
    vec3    position;   //!< The world position of the light in the scene.
    vec3    direction;  //!< The direction of the light.
    vec3    colour;     //!< The un-attenuated colour of the light.
    float   coneAngle;  //!< The angle of the light cone in degrees.
    float   cConstant;  //!< The constant co-efficient for the attenutation formula.
    float   cQuadratic; //!< The quadratic co-efficient for the attenuation formula.
};


/// <summary> The uniform buffer for each shader. </summary>
layout (packed, std140) uniform ubo
{
    mat4    projection;         //!< The projection transform which establishes the perspective of the vertex.
    mat4    view;               //!< The view transform representing where the camera is looking.

    vec3    cameraPosition;     //!< Contains the position of the camera in world space.
    vec3    ambience;           //!< The ambient lighting in the scene.
    
    int     numLights;          //!< The number of lights in use.
    Light   lights[MAX_LIGHTS]; //!< The lighting data of each light in the scene.
};


        uniform samplerBuffer   materialBuffer; //!< A texture buffer filled with the required diffuse and specular properties for the material.
        uniform sampler2D       textureSampler; //!< The desired texture to apply to the particular pixel.


        in      vec3            worldPosition;  //!< The fragments position vector in world space.
        in      vec3            worldNormal;    //!< The fragments normal vector in world space.
        in      vec2            texturePoint;   //!< The interpolated co-ordinate to use for the texture sampler.
flat    in      int             instanceID;     //!< Used in fetching instance-specific data from the uniforms.


        out     vec4            fragmentColour; //!< The computed output colour of this particular pixel;


/// <summary> Updates the ambient, diffuse and specular colours from the materialTBO for this fragment. </summary>
void obtainMaterialProperties();

/// <summary> Can be used to colour the scene using red, green or and blue triangles. </summary>
/// <returns> A single colour based on the primitive ID of the current triangle. </returns>
vec3 primitiveColour();

/// <summary> Calculates the attenuation value of a point light based on the given distance and range. </summary>
/// <param name="distance"> The distance of the fragment from the light. </param>
/// <param name="range"> The range of the point light. Light shall not extend beyond this value. </param>
float pointLightAttenuation (const float distance, const float range, bool useSmoothstep);

vec3 cameraPointLight();


//float spotLightAttenuation (const Light light, const vec3 L, const float distance, const unsigned int concentration);

const float ka  = 0.2;
const float kd  = 0.8;
const float ks  = 0.0;

vec3 ambient    = ambience * ka;
vec3 diffuse    = vec3 (1.0, 1.0, 1.0) * kd;
vec3 specular   = vec3 (1.0, 1.0, 1.0) * ks;
float shininess = 16.0;


void main()
{
    // Ensure we're using the correct colours.
    obtainMaterialProperties();

    // Parameters.
    vec3 Q = worldPosition;
    vec3 N = normalize (worldNormal);
    vec3 V = normalize (cameraPosition - Q);

    // Shade each light.
    vec3 lighting = cameraPointLight();
    

    /*// Perform point light calculations for the ten small lights and point light calculations for the first.
    for (int i = 0; i < lights.length(); ++i)
    {
        float distance = length (lights[i].position - Q);
        vec3 L = (lights[i].position - Q) / distance;
        vec3 R = reflect (L, N);
        
        float lambertian = max (dot (L, N), 0);
        
        //if (lambertian > 0)
        {
            vec3 attenuatedColour = vec3 (0, 0, 0);

            // The first light should be a point light, the rest are spot lights.
            if (i == 0)
            {
                attenuatedColour = colours[i] * spotLightAttenuation (lights[i], L, distance, 10);
            }
            
            else
            {
                attenuatedColour = colours[i] * pointLightAttenuation (distance, lights[i].range);
            }
            
            // Calculate the lighting to add.
            vec3 diffuseLighting = kd * lambertian;
            vec3 specularLighting = ks * pow (max (dot (V, R), 0), shininess);
            
            lighting += attenuatedColour * (diffuseLighting + specularLighting);
        }
    }*/
    
    // Outcome.
    fragmentColour = vec4 (ambient + lighting, 1.0);
}


void obtainMaterialProperties()
{
    // We can use the instance ID to reconstruct the diffuse and specular colours from the RGBA material buffer.
    // Each instanceID is allocated 16 bytes of data for the diffuse colour and 16 bytes for the specular colour.
    vec4 diffusePart    = texelFetch (materialBuffer, instanceID * 2);
    vec4 specularPart   = texelFetch (materialBuffer, instanceID * 2 + 1);
    
    // The alpha value of the diffuse is unused.
    diffuse = vec3 (diffusePart.r, diffusePart.g, diffusePart.b) * kd;
    
    // The alpha value of the specular is the shininess value.
    specular = vec3 (specularPart.r, specularPart.g, specularPart.b) * ks;
    shininess = specularPart.a; 
}


vec3 cameraPointLight()
{
    vec3 Q = worldPosition;
    vec3 N = normalize (worldNormal);
    
    float distance = length (cameraPosition - Q);
    vec3 L = (cameraPosition - Q) / distance;

    vec3 light = vec3 (1.0, 1.0, 1.0) * pointLightAttenuation (distance, 50.0, false);
    
    float lambertian = max (dot (L, N), 0);

    return light * (diffuse * lambertian);
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
    const float kl = 0.005;
    float kq = 1.0 / (range * range * 0.01);
    
    // Calculate the final attenuation value.
    float attenuation = 1.0 / (kc + kl * distance + kq * distance * distance);

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