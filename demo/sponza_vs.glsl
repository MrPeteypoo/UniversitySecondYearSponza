#version 330

// Make the project and view transforms uniform across the application.
uniform mat4 projection;
uniform mat4 view;

// Use interleaved position, normal and texture co-ordinates.
layout (location = 0)   in vec3 position;
layout (location = 1)   in vec3 normal;
layout (location = 2)   in vec2 textureCoord;

// Use instance-specific model and PVM transforms.
layout (location = 3)   in mat4 model;
layout (location = 7)   in mat4 pvm;

// Output the normal colour and texture co-ordinates of the vertex to the fragment shader.
out vec3 normalColour;
out vec2 textureOut;


void main()
{
    vec3 vertexColour = mat3 (model) * normal;
    normalColour = 0.5 + 0.5 * vertexColour;
    textureOut = textureCoord;

    //gl_Position = projection * view * model * vec4 (position, 1.0);
    gl_Position = pvm * vec4 (position, 1.0);
}