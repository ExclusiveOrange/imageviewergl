#version 410

layout(location = 0) in vec2 uv;

uniform sampler2D theTexture;

layout(location = 0) out vec4 outColor;


void main()
{
  outColor = texture( theTexture, uv );
}