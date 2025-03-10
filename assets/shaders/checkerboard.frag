#version 330 core

out vec4 frag_color;

// In this shader, we want to draw a checkboard where the size of each tile is (size x size).
// The color of the top-left most tile should be "colors[0]" and the 2 tiles adjacent to it
// should have the color "colors[1]".

//TODO: (Req 1) Finish this shader.

uniform int size = 32;
uniform vec3 colors[2];

void main(){
    // Get the current fragment's position in screen space
    ivec2 tile = ivec2(gl_FragCoord.xy) / size;

    // Compute the checkerboard pattern (alternating tiles)
    int checkerIndex = (tile.x + tile.y) % 2;

    // Set the fragment color based on the checker index
    frag_color = vec4(colors[checkerIndex], 1.0);
}