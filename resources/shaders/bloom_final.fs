#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;
uniform bool bloom;
uniform float exposure;
const float offset = 1.0 / 300.0;
uniform int ind;

void main()
{
    const float gamma = 2.2;
    vec3 hdrColor = texture(scene, TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    if(bloom)
        hdrColor += bloomColor; // additive blending
    // tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // also gamma correct while we're at it
    result = pow(result, vec3(1.0 / gamma));

    if(1==ind){
        FragColor = vec4(1.0-result, 1.0);
    }
    else if(2==ind){
       float grayscale=0.2126*result.r + 0.7152*result.g+0.0722 * result.b;
       FragColor=vec4(vec3(grayscale),1.0);
    }
    else if(3==ind){

        vec2 offsets[9] = vec2[](
            vec2(-offset, offset), // top/left
            vec2(0.0f, offset), // top-center
            vec2(offset, offset),
            vec2(-offset, 0.0f),
            vec2(0.0f, 0.0f),
            vec2(0.0f, offset),
            vec2(-offset, -offset),
            vec2(0.0f, -offset),
            vec2(offset, -offset)
         );

        float kernel[9] = float[](
            1.0 / 16, 2.0/16, 1.0/ 16,
            2.0 / 16, 4.0/16, 2.0/ 16,
            1.0 / 16, 2.0/16, 1.0/ 16
        );

        vec3 sampleTex[9];
        for (int i = 0; i < 9; ++i) {
            sampleTex[i] = vec3(texture(scene, TexCoords.st + offsets[i]));
            if (bloom){
                sampleTex[i] += vec3(texture(bloomBlur, TexCoords.st + offsets[i]));
            }
        }

        vec3 col = vec3(0.0);
        for (int i = 0; i < 9; ++i) {
            col += sampleTex[i] * kernel[i];
        }

        FragColor = vec4(col, 1.0);
    }
    else if(0==ind){
           FragColor = vec4(result, 1.0);
    }

}