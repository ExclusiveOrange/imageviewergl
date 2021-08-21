#version 410

layout(location = 0) out vec2 uv;

void main()
{
  const vec2 xys[] = vec2[](
  vec2(-1.0, -1.0),
  vec2(1.0, -1.0),
  vec2(-1.0, 1.0),
  vec2(1.0, 1.0)
  );

  const vec2 uvs[4] = vec2[](
  vec2(0.0, 0.0),
  vec2(1.0, 0.0),
  vec2(0.0, 1.0),
  vec2(1.0, 1.0)
  );

  gl_Position = vec4(xys[gl_VertexID], 0.0, 1.0);
  uv = uvs[gl_VertexID];
}
