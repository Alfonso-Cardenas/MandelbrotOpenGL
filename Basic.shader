#shader vertex
#version 330 core

layout(location = 0) in vec4 position;

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
uniform vec2 delta;
uniform float samplesPerPixel;
uniform vec3 inputColor;
uniform vec2 a;
uniform vec2 z0input;
uniform vec2 w0input;
uniform float height;
uniform float width;
uniform vec2 center;
uniform mat3 rotMat;
uniform float deltaJ;
uniform vec2 offsetJ;
uniform float zoomJ;
uniform vec3 inputColorJ;

float a1;


vec2 D[] =
{
    vec2(0.0, 0.0),
    vec2(0.0, delta.y),
    vec2(0.0, -delta.y),
    vec2(delta.x, 0.0),
    vec2(-delta.x, 0.0),
    vec2(delta.x, delta.y),
    vec2(-delta.x, delta.y),
    vec2(delta.x, -delta.y),
    vec2(-delta.x, -delta.y)
};

vec2 DJ[] =
{
    vec2(0.0, 0.0),
    vec2(0.0, deltaJ),
    vec2(0.0, -deltaJ),
    vec2(deltaJ, 0.0),
    vec2(-deltaJ, 0.0),
    vec2(deltaJ, deltaJ),
    vec2(-deltaJ, deltaJ),
    vec2(deltaJ, -deltaJ),
    vec2(-deltaJ, -deltaJ)
};

layout(location = 0) out vec4 color;

layout(pixel_center_integer) in vec4 gl_FragCoord;

vec2 pos;

vec3 colorz = vec3(inputColor.x, inputColor.y * 0.5, 0.0);
vec3 colorw = vec3(0.0, inputColor.y * 0.5, inputColor.z);

vec3 calcMandelbrot(vec2 c)
{
    vec3 calculatedColor = vec3(0.0, 0.0, 0.0);
    vec2 z0 = z0input + c;
    vec2 w0 = w0input + c;
    for (int i = 0; i < samplesPerPixel; i++)
    {
        vec2 z = z0;
        vec2 w = w0;
        float it = 0.0;
        vec2 z2;
        float zx2 = z.x * z.x;
        float zy2 = z.y * z.y;
        vec2 w2;
        float wx2 = w.x * w.x;
        float wy2 = w.y * w.y;
        while (zx2 + zy2 <= 4.0 && it++ < maxIt)
        {
            z2 = vec2(zx2 - zy2, 2.0 * z.x * z.y);
            z = z2 + vec2(a.x * z2.x + a.y * z2.y, a.y * z2.x - a.x * z2.y) / dot(z2,z2) + c + D[i];
            zx2 = z.x * z.x;
            zy2 = z.y * z.y;
        }
        calculatedColor += colorz * it / maxIt;
        
        it = 0.0;
        while (wx2 + wy2 <= 4.0 && it++ < maxIt)
        {
            w2 = vec2(wx2 - wy2, 2.0 * w.x * w.y);
            w = w2 + vec2(a.x * w2.x + a.y * w2.y, a.y * w2.x - a.x * w2.y) / dot(w2, w2) + c + D[i];
            wx2 = w.x * w.x;
            wy2 = w.y * w.y;
        }
        calculatedColor += colorw * it / maxIt;
    }
    return calculatedColor / samplesPerPixel;
}

const float sqrt2 = sqrt(2);
vec3 calcMandelbrot3(vec2 plano)
{
    vec2 c = plano;
    float abs2 = dot(c, c) + a1 * a1;
    if (abs2 > 1)
        return vec3(0.5, 0.5, 0.5);
    float a2 = sqrt(1 - abs2);
    vec3 rotatedVector = vec3(a2, c);
    rotatedVector = rotMat * rotatedVector;
    a2 = rotatedVector.x;
    c = vec2(rotatedVector.y, rotatedVector.z);
    vec2 a = vec2(a1, a2);
    float r = sqrt(dot(a, a));
    vec2 ar = vec2(r, 0) + a;
    vec2 sqrtaX2 = 2 * sqrt(r) * ar / sqrt(dot(ar, ar));
    vec3 calculatedColor = vec3(0.0, 0.0, 0.0);
    vec2 z0;
    vec2 w0;
    
    z0 = sqrtaX2 + c;
    w0 = -sqrtaX2 + c;
    
    for (int i = 0; i < samplesPerPixel; i++)
    {
        vec2 z = z0;
        vec2 w = w0;
        float it = 0.0;
        vec2 z2;
        float zx2 = z.x * z.x;
        float zy2 = z.y * z.y;
        vec2 w2;
        float wx2 = w.x * w.x;
        float wy2 = w.y * w.y;
        while (zx2 + zy2 <= 4.0 && it++ < maxIt)
        {
            z2 = vec2(zx2 - zy2, 2.0 * z.x * z.y);
            z = z2 + vec2(a.x * z2.x + a.y * z2.y, a.y * z2.x - a.x * z2.y) / dot(z2, z2) + c + D[i];
            zx2 = z.x * z.x;
            zy2 = z.y * z.y;
        }
        calculatedColor += colorz * it / maxIt;
        
        it = 0.0;
        while (wx2 + wy2 <= 4.0 && it++ < maxIt)
        {
            w2 = vec2(wx2 - wy2, 2.0 * w.x * w.y);
            w = w2 + vec2(a.x * w2.x + a.y * w2.y, a.y * w2.x - a.x * w2.y) / dot(w2, w2) + c + D[i];
            wx2 = w.x * w.x;
            wy2 = w.y * w.y;
        }
        calculatedColor += colorw * it / maxIt;
    }
    return calculatedColor / samplesPerPixel;
}

vec3 calcMandelbrot2(vec2 plano)
{
    vec2 c = plano;
    float absc2 = dot(c, c);
    if (absc2 > 1)
        return vec3(0.5, 0.5, 0.5);
    float a = sqrt(1 - absc2);
    vec3 rotatedVector = vec3(a, c);
    rotatedVector = rotMat * rotatedVector;
    a = rotatedVector.x;
    c = vec2(rotatedVector.y, rotatedVector.z);
    float sqrtaX2 = sqrt2 * sqrt(abs(a));
    vec3 calculatedColor = vec3(0.0, 0.0, 0.0);
    vec2 z0;
    vec2 w0;
    
    if (a > 0)
    {
        z0 = vec2(sqrtaX2, sqrtaX2) + c;
        w0 = vec2(-sqrtaX2, -sqrtaX2) + c;
    }
    else
    {
        z0 = vec2(sqrtaX2, -sqrtaX2) + c;
        w0 = vec2(-sqrtaX2, sqrtaX2) + c;
    }
    for (int i = 0; i < samplesPerPixel; i++)
    {
        vec2 z = z0;
        vec2 w = w0;
        float it = 0.0;
        vec2 z2;
        float zx2 = z.x * z.x;
        float zy2 = z.y * z.y;
        vec2 w2;
        float wx2 = w.x * w.x;
        float wy2 = w.y * w.y;
        while (zx2 + zy2 <= 4.0 && it++ < maxIt)
        {
            z2 = vec2(zx2 - zy2, 2.0 * z.x * z.y);
            z = z2 + vec2(a * z2.y, a * z2.x) / dot(z2, z2) + c + D[i];
            zx2 = z.x * z.x;
            zy2 = z.y * z.y;
        }
        calculatedColor += colorz * it / maxIt;
        
        it = 0.0;
        while (wx2 + wy2 <= 4.0 && it++ < maxIt)
        {
            w2 = vec2(wx2 - wy2, 2.0 * w.x * w.y);
            w = w2 + vec2(a * w2.y, a * w2.x) / dot(w2, w2) + c + D[i];
            wx2 = w.x * w.x;
            wy2 = w.y * w.y;
        }
        calculatedColor += colorw * it / maxIt;
    }
    return calculatedColor / samplesPerPixel;
}

vec3 calcJulia(vec2 plano)
{
    vec2 c = center;
    float absc2 = dot(c, c);
    if (absc2 > 1)
        return vec3(0.5, 0.5, 0.5);
    float a = sqrt(1 - absc2);
    vec3 rotatedVector = vec3(a, c);
    rotatedVector = rotMat * rotatedVector;
    a = rotatedVector.x;
    c = vec2(rotatedVector.y, rotatedVector.z);
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
            z = z2 + vec2(a * z2.y, a * z2.x) / dot(z2, z2) + vec2(0, c) + D[i];
            zx2 = z.x * z.x;
            zy2 = z.y * z.y;
        }
        if(it < maxIt)
            calculatedColor += vec3(0.0, 0.0, 1.0) * it / maxIt;
        else
            calculatedColor += vec3(1.0, 1.0, 1.0);

    }
    return calculatedColor / samplesPerPixel;
}

vec3 calcJuliaRebanadas(vec2 plano)
{
    vec2 c = center;
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
            z = z2 + vec2(a.x * z2.x + a.y * z2.y, a.y * z2.x - a.x * z2.y) / dot(z2, z2) + c + DJ[i];
            zx2 = z.x * z.x;
            zy2 = z.y * z.y;
        }
        if (it < maxIt)
            calculatedColor += inputColorJ * vec3(1.0, 1.0, 1.0) * it / maxIt;
        else
            calculatedColor += vec3(1.0, 1.0, 1.0);

    }
    return calculatedColor / samplesPerPixel;
}

void main()
{
    if (gl_FragCoord.x >= width)
    {
        pos = zoomJ * (gl_FragCoord.xy - vec2(width, 0.0)) / height + offsetJ;
        color = vec4(calcJuliaRebanadas(pos), 1.0);
        //color = vec4(pos, 0, 0);
        //color = vec4(1.0, 0, 0, 0);
    }
    else
    {
        pos = zoom * (gl_FragCoord.xy) / height + offset;
        //float check = dot(pos - center, pos - center);
        //if(check > .001 && check < .0015)
        //{
        //    color = vec4(1.0, 0.0, 0.0, 1.0);
        //}
        //else
        //{
        color = vec4(calcMandelbrot(pos), 1.0);
        //}
    }
};