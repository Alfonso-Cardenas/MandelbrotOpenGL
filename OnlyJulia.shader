#shader vertex
#version 330 core

layout(location = 0) in
vec4 position;

void main()
{
    gl_Position = position;
};

#shader fragment
#version 330 core

const int MAXMAXIT = 5000;

uniform vec2 offset;
uniform float zoom;
uniform float maxIt;
uniform float samplesPerPixel;
uniform vec3 inputColor;
uniform vec2 a;
uniform float height;
uniform float width;
uniform vec2 c;
uniform float delta;

vec2 D[] =
{
    vec2(0.0, 0.0),
    vec2(0.0, delta),
    vec2(0.0, -delta),
    vec2(delta, 0.0),
    vec2(-delta, 0.0),
    vec2(delta, delta),
    vec2(-delta, delta),
    vec2(delta, -delta),
    vec2(-delta, -delta)
};

layout(location = 0) out
vec4 color;

layout(pixel_center_integer) in
vec4 gl_FragCoord;

vec2 pos;


vec3 calcJuliaRebanadas(vec2 plano)
{
    vec3 calculatedColor = vec3(0.0, 0.0, 0.0);
    vec2 z0 = plano;
    if (dot(z0, z0) > 4.0)
        return vec3(0.5, 0.5, 0.5);
    for (int i = 0; i < samplesPerPixel; i++)
    {
        vec2 z = z0;
        float it = 0.0;
        vec2 z2;
        float zx2 = z.x * z.x;
        float zy2 = z.y * z.y;
        while (zx2 + zy2 <= 4.0 && it++ < maxIt)
        {
            z2 = vec2(zx2 - zy2, 2.0 * z.x * z.y);
            z = z2 + vec2(a.x * z2.x + a.y * z2.y, a.y * z2.x - a.x * z2.y) / dot(z2, z2) + c + D[i];
            zx2 = z.x * z.x;
            zy2 = z.y * z.y;
        }
        if (it < maxIt)
            calculatedColor += inputColor * it / maxIt;
        else
            calculatedColor += vec3(1.0, 1.0, 1.0);

    }
    return calculatedColor / samplesPerPixel;
}

void main()
{
    pos = zoom * gl_FragCoord.xy / height + offset;
    color = vec4(calcJuliaRebanadas(pos), 1.0);
};