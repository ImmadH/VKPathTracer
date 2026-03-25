#version 450

layout(set = 0, binding = 0, rgba16f) uniform readonly image2D accumulationImage;

layout(location = 0) out vec4 outColor;

vec3 toneMap(vec3 hdrColor)
{
  vec3 mapped = hdrColor / (hdrColor + vec3(1.0));
  return pow(mapped, vec3(1.0 / 2.2));
}

void main()
{
  ivec2 imageExtent = imageSize(accumulationImage);
  ivec2 pixelCoord = clamp(ivec2(gl_FragCoord.xy), ivec2(0), imageExtent - ivec2(1));
  vec3 hdrColor = imageLoad(accumulationImage, pixelCoord).rgb;
  outColor = vec4(toneMap(hdrColor), 1.0);
}
