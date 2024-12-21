#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D texture1;
uniform int numRenderedFrames;

// Output fragment color
out vec4 finalColor;

void main()
{
    vec4 newRender = texture(texture0, fragTexCoord);
    vec4 oldRender = texture(texture1, fragTexCoord);

    float weight = 1.0 / (2 + 1);
    vec4 accumulatedAverage = oldRender * (1 - weight) + newRender * weight;

    finalColor = newRender;
}