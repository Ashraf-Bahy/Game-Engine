#version 330 core
#define MAX_LIGHTS 20
#define DIRECTIONAL_LIGHT 0
#define POINT_LIGHT 1
#define SPOT_LIGHT 2

// inputs
in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 frag_position;
    vec3 frag_normal;
} fs_in;

// outputs
out vec4 frag_color;


// uniforms
uniform int num_lights;
uniform vec3 view_position;

struct Material {
    sampler2D albedo;
    sampler2D specular;    
    float shininess;
}; 
uniform Material material;

// lights
struct Light {
    int type;
    vec3 position;
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;
};
uniform Light lights[MAX_LIGHTS];



// functions
vec3 calcDirLight(Light light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);

    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    
    // combine results
    vec3 ambient  = light.ambient  * texture(material.albedo, fs_in.tex_coord).rgb;
    vec3 diffuse  = light.diffuse  * diff * texture(material.albedo, fs_in.tex_coord).rgb;
    vec3 specular = light.specular * spec * vec3(texture(material.specular, fs_in.tex_coord));
    return (ambient + diffuse);
} 

vec3 calcPointLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);

    // diffuse shadding
    float diff = max(dot(normal, lightDir), 0.0);
    
    // specular
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 ambient  = light.ambient  * texture(material.albedo, fs_in.tex_coord).rgb;
    vec3 diffuse  = light.diffuse  * diff * texture(material.albedo, fs_in.tex_coord).rgb;
    vec3 specular = light.specular * spec * texture(material.specular, fs_in.tex_coord).rgb;

    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    ambient  *= attenuation;  
    diffuse   *= attenuation;
    specular *= attenuation;   
        
    return  (ambient + diffuse + specular);
}

vec3 calcSpotLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);

    // diffuse shadding
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shadding
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);


    vec3 ambient  = light.ambient  * texture(material.albedo, fs_in.tex_coord).rgb;
    vec3 diffuse  = light.diffuse  * diff * texture(material.albedo, fs_in.tex_coord).rgb;
    vec3 specular = light.specular * spec * texture(material.specular, fs_in.tex_coord).rgb;
    
    // spotlight (soft edges)
    float theta = dot(lightDir, normalize(-light.direction)); 
    float epsilon = (light.cutOff - light.outerCutOff);
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    diffuse  *= intensity;
    specular *= intensity;


    // attenuation
    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    diffuse   *= attenuation;
    specular *= attenuation;   
        
    return  (ambient + diffuse + specular);
}


void main(){
    // properties
    vec3 norm = normalize(fs_in.frag_normal);
    vec3 viewDir = normalize(view_position - fs_in.frag_position);
    
    // calculations
    vec3 result = vec3(0.0);
    for(int i = 0; i < num_lights; i++)
    {
        if (lights[i].type == DIRECTIONAL_LIGHT)
            result += calcDirLight(lights[i], norm, viewDir); 
        else if (lights[i].type == POINT_LIGHT) 
            result += calcPointLight(lights[i], norm, fs_in.frag_position, viewDir);
        else if (lights[i].type == SPOT_LIGHT)
            result += calcSpotLight(lights[i], norm, fs_in.frag_position, viewDir);
    }

    frag_color = vec4(result, 1.0);
}