#version 460

layout(location = 0) in float vcolor;

layout(location = 0) out float4 frag_color;

void main() {
  frag_color = float4(color, color, color, 1.0);
}