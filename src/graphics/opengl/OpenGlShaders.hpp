#pragma once

static const char *VERT_SRC = R"glsl(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in vec2 aUV;
layout(location = 3) in float aUseTex;
out vec4 vColor;
out vec2 vUV;
out float vUseTex;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vColor = aColor;
    vUV = aUV;
    vUseTex = aUseTex;
}
)glsl";

static const char *FRAG_SRC = R"glsl(
#version 330 core
in  vec4 vColor;
in  vec2 vUV;
in  float vUseTex;
out vec4 FragColor;
uniform sampler2D uFont;
uniform sampler2D uTiles;
uniform vec2 uFontTexel;
uniform vec2 uResolution;

float sdRoundBox(vec2 p, vec2 b, float r)
{
    vec2 q = abs(p) - b + vec2(r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

float roundedRectAlpha(vec2 uv)
{
    vec2 p = uv * 2.0 - 1.0;
    float pad = 0.06;
    float radius = 0.22;
    float d = sdRoundBox(p, vec2(1.0 - pad), radius);
    float aa = fwidth(d) * 1.25;
    return 1.0 - smoothstep(0.0, aa, d);
}

float roundedRectBorder(vec2 uv)
{
    vec2 p = uv * 2.0 - 1.0;
    float pad = 0.06;
    float radius = 0.22;
    float d = sdRoundBox(p, vec2(1.0 - pad), radius);
    float aa = fwidth(d) * 1.25;
    float w = 0.055;
    float outer = 1.0 - smoothstep(0.0, aa, d);
    float inner = 1.0 - smoothstep(0.0, aa, d + w);
    return clamp(outer - inner, 0.0, 1.0);
}

float vignetteFactor()
{
    vec2 q = gl_FragCoord.xy / max(uResolution, vec2(1.0));
    vec2 d = q - vec2(0.5);
    float r2 = dot(d, d);
    return mix(1.0, 0.90, smoothstep(0.10, 0.35, r2));
}

void main() {
    if (vUseTex > 1.5) {
        vec3 tex = texture(uTiles, vUV).rgb;
        vec4 outC = vec4(vColor.rgb * tex, vColor.a);
        outC.rgb *= vignetteFactor();
        FragColor = outC;
    } else if (vUseTex > 0.5) {
        float mask = texture(uFont, vUV).r;
        float sh = texture(uFont, vUV + vec2(uFontTexel.x, -uFontTexel.y)).r;
        float shadow = sh * (1.0 - mask);
        vec3 rgb = vColor.rgb;
        vec4 base = vec4(rgb, vColor.a * mask);
        vec4 shade = vec4(vec3(0.0), vColor.a * 0.45 * shadow);
        vec4 outC = base + shade;
        outC.rgb *= vignetteFactor();
        FragColor = outC;
    } else {
        float a = roundedRectAlpha(vUV);
        float b = roundedRectBorder(vUV);
        vec3 border = clamp(vColor.rgb * 1.15, 0.0, 1.0);
        vec3 fill = mix(vColor.rgb, border, b * 0.35);
        vec4 outC = vec4(fill, vColor.a * a);
        outC.rgb *= vignetteFactor();
        FragColor = outC;
    }
}
)glsl";
