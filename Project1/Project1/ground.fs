#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform vec3 groundColor;
uniform vec3 lightDir;

void main()
{
    // Simple directional lighting
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(lightDir);
    float diff = max(dot(norm, lightDirection), 0.0);
    
    vec3 ambient = 0.3 * groundColor;
    vec3 diffuse = diff * groundColor;
    
    vec3 result = ambient + diffuse;
    FragColor = vec4(result, 1.0);
}