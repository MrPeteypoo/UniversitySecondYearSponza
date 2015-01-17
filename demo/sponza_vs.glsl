#version 330

uniform mat4 projectionViewModel;
uniform mat4 modelTransform;

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 texturePoint;

out vec3 normalColour;
out vec2 textureOut;

void main()
{
    vec3 vertexColour = mat3 (modelTransform) * vertexNormal;
    normalColour = 0.5 + 0.5 * vertexColour;
    textureOut = texturePoint;

    gl_Position = projectionViewModel * vec4 (vertexPosition, 1.0);
}