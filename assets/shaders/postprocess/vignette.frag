#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;

out vec4 frag_color;

// Vignette is a postprocessing effect that darkens the corners of the screen
// to grab the attention of the viewer towards the center of the screen

void main(){
    //TODO: Modify this shader to apply vignette
    // To apply vignette, divide the scene color
    // by 1 + the squared length of the 2D pixel location the NDC space
    // Hint: remember that the NDC space ranges from -1 to 1
    // while the texture coordinate space ranges from 0 to 1
    // We have the pixel's texture coordinate, how can we compute its location in the NDC space?

    // Convert texture coordinates (0 to 1) to NDC space (-1 to 1)
    vec2 ndc_coord = tex_coord * 2.0 - 1.0;

    // Compute the squared length of the NDC coordinates
    float vignette_factor = 1.0 + dot(ndc_coord, ndc_coord);

    // Apply vignette effect by dividing the scene color by the vignette factor
    frag_color = texture(tex, tex_coord) / vignette_factor;
}