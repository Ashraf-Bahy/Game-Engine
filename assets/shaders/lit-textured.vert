#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 frag_position;
    vec3 frag_normal;
    vec3 view_position;
} vs_out;

uniform mat4 model;
uniform mat4 transform;

void main(){
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;
    vs_out.frag_position = vec3(model * vec4(position, 1.0));
    vs_out.frag_normal = mat3(transpose(inverse(model))) * normal;
    gl_Position = transform * vec4(position, 1.0);
}