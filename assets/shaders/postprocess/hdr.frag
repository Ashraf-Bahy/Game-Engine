#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds
in vec2 tex_coord;
out vec4 frag_color;

// HDR tonemapping parameters
uniform float exposure = 1.0;    // Controls brightness
uniform float gamma = 2.2;       // Standard gamma correction value

void main() {
    // Sample the scene color
    vec4 hdrColor = texture(tex, tex_coord);
    
    // Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor.rgb * exposure);
    
    // Gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
    
    // Preserve alpha
    frag_color = vec4(mapped, hdrColor.a);
}
