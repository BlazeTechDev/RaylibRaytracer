#version 430

#define SAMPLES 80  // HIGHER = NICER = SLOWER
#define DISTRIBUTION_BIAS 0.6 // between 0. and 1.
#define PIXEL_MULTIPLIER  1.5 // between 1. and 3. (keep low)
#define INVERSE_HUE_TOLERANCE 20.0 // (2. - 30.)

#define GOLDEN_ANGLE 2.3999632 //3PI-sqrt(5)PI

#define pow(a,b) pow(max(a,0.),b) // @morimea

mat2 sample2D = mat2(cos(GOLDEN_ANGLE), sin(GOLDEN_ANGLE), -sin(GOLDEN_ANGLE), cos(GOLDEN_ANGLE));

in vec2 fragTexCoord;

uniform sampler2D texture0;

uniform vec2 resolution;
uniform bool denoise;

out vec4 out_color;

vec3 sirBirdDenoise(sampler2D imageTexture, vec2 uv, vec2 imageResolution) {

    vec3 denoisedColor = vec3(0.);

    const float sampleRadius = sqrt(float(SAMPLES));
    const float sampleTrueRadius = 0.5 / (sampleRadius * sampleRadius);
    vec2        samplePixel = vec2(1.0 / imageResolution.x, 1.0 / imageResolution.y);
    vec3        sampleCenter = texture(imageTexture, uv).rgb;
    vec3        sampleCenterNorm = normalize(sampleCenter);
    float       sampleCenterSat = length(sampleCenter);

    float  influenceSum = 0.0;
    float brightnessSum = 0.0;

    vec2 pixelRotated = vec2(0., 1.);

    for (float x = 0.0; x <= float(SAMPLES); x++) {

        pixelRotated *= sample2D;

        vec2  pixelOffset = PIXEL_MULTIPLIER * pixelRotated * sqrt(x) * 0.5;
        float pixelInfluence = 1.0 - sampleTrueRadius * pow(dot(pixelOffset, pixelOffset), DISTRIBUTION_BIAS);
        pixelOffset *= samplePixel;

        vec3 thisDenoisedColor =
            texture(imageTexture, uv + pixelOffset).rgb;

        pixelInfluence *= pixelInfluence * pixelInfluence;
        /*
            HUE + SATURATION FILTER
        */
        pixelInfluence *=
            pow(0.5 + 0.5 * dot(sampleCenterNorm, normalize(thisDenoisedColor)), INVERSE_HUE_TOLERANCE)
            * pow(1.0 - abs(length(thisDenoisedColor) - length(sampleCenterSat)), 8.);

        influenceSum += pixelInfluence;
        denoisedColor += thisDenoisedColor * pixelInfluence;
    }

    return denoisedColor / influenceSum;

}

void main()
{
    out_color = texture(texture0, fragTexCoord);
}