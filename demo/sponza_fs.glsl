#version 330

uniform sampler2D textureSampler;

in vec3 normalColour;
in vec2 textureOut;

out vec4 fragmentColour;

vec3 getPrimitiveColour();


void main()
{
    //fragmentColour = vec4 (normalColour, 1.0);
    //fragmentColour = vec4 (getPrimitiveColour(), 1.0);
    vec3 textureColour = texture (textureSampler, textureOut).rgb;
    fragmentColour = vec4 (textureColour * normalColour, 1.0);
}


vec3 getPrimitiveColour()
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