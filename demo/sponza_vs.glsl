/// <summary> The vertex shader used in Spice My Sponza. Prepares the information required for the fragment shader. </summary>
/// <namespace> GLSL::VERTEX </namespace>

#version 330


/// <summary> The uniform buffer scene specific information. </summary>
layout (std140) uniform scene
{
    mat4    projection;         //!< The projection transform which establishes the perspective of the vertex.
    mat4    view;               //!< The view transform representing where the camera is looking.

    vec3    cameraPosition;     //!< Contains the position of the camera in world space.
    vec3    ambience;           //!< The ambient lighting in the scene.
};


layout (location = 0)   in      vec3    position;       //!< The local position of the current vertex.
layout (location = 1)   in      vec3    normal;         //!< The local normal vector of the current vertex.
layout (location = 2)   in      vec2    textureCoord;   //!< The texture co-ordinates for the vertex, used for mapping a texture to the object.

layout (location = 3)   in      mat4    model;          //!< The model transform representing the position and rotation of the object in world space.
layout (location = 7)   in      mat4    pvm;            //!< A combined matrix of the project, view and model transforms.


                        out     vec3    worldPosition;  //!< The world position to be interpolated for the fragment shader.
                        out     vec3    worldNormal;    //!< The world normal to be interpolated for the fragment shader.
                        out     vec2    texturePoint;   //!< The texture co-ordinate for the fragment to use for texture mapping.
flat                    out     int     instanceID;     //!< Allows the fragment shader to fetch the correct colour data.


void main()
{
    // Deal with the outputs first.
    worldPosition = mat4x3 (model) * vec4 (position, 1.0);
    worldNormal = mat3 (model) * normal;
    texturePoint = textureCoord;
    instanceID = gl_InstanceID;

    // Place the vertex in the correct position on-screen.
    gl_Position = pvm * vec4 (position, 1.0);
}