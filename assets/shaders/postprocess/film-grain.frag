#version 330

// The texture holding the scene pixels
uniform sampler2D tex;
uniform float time; // Time uniform for animated grain

in vec2 tex_coord;
out vec4 frag_color;

// Film grain parameters
const float GRAIN_AMOUNT = 0.1; // Adjust for more/less grain
const float GRAIN_SIZE = 1.6;   // Larger values = coarser grain

// Pseudo-random function
float rand(vec2 co) {
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main() {
    // Get the scene color
    vec4 color = texture(tex, tex_coord);
    
    // Generate noise based on screen position and time
    vec2 noiseCoord = gl_FragCoord.xy / GRAIN_SIZE;
    float noise = rand(noiseCoord + vec2(time * 0.001));
    
    // Mix the noise with the scene color
    vec3 grainColor = color.rgb;
    grainColor += (noise - 0.5) * GRAIN_AMOUNT;
    
    frag_color = vec4(grainColor, color.a);
}
