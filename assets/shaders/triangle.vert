#version 330

// This vertex shader should be used to render a triangle whose normalized device coordinates are:
// (-0.5, -0.5, 0.0), ( 0.5, -0.5, 0.0), ( 0.0,  0.5, 0.0)
// And it also should send the vertex color as a varying to the fragment shader where the colors are (in order):
// (1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)

out Varyings {
    vec3 color;
} vs_out;

// Currently, the triangle is always in the same position, but we don't want that.
// So two uniforms should be added: translation (vec2) and scale (vec2).
// Each vertex "v" should be transformed to be "scale * v + translation".
// The default value for "translation" is (0.0, 0.0) and for "scale" is (1.0, 1.0).

//TODO: (Req 1) Finish this shader

// Define uniforms for translation and scaling
uniform vec2 translation = vec2(0.0, 0.0);
uniform vec2 scale = vec2(1.0, 1.0);

void main(){
    // Define an array of vertex positions in normalized device coordinates
    const vec3 positions[3] = vec3[](
        vec3(-0.5, -0.5, 0.0), // First vertex
        vec3( 0.5, -0.5, 0.0), // Second vertex
        vec3( 0.0,  0.5, 0.0)  // Third vertex
    );

    // Define an array of vertex colors
    const vec3 colors[3] = vec3[](
        vec3(1.0, 0.0, 0.0), // Red
        vec3(0.0, 1.0, 0.0), // Green
        vec3(0.0, 0.0, 1.0)  // Blue
    );

    // Get the vertex position and color using the built-in "gl_VertexID"
    vec3 position = positions[gl_VertexID];
    vs_out.color = colors[gl_VertexID];

    // Apply scale and translation
    vec2 transformedPosition = scale * position.xy + translation;

    // Output final position in homogeneous coordinates
    gl_Position = vec4(transformedPosition, position.z, 1.0);
}