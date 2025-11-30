#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;     // World position
in vec3 Normal;      // World normal

uniform sampler2D texture_diffuse1;
uniform float alphaThreshold = 0.05; // base threshold
uniform int materialType = 0;        // 0=default, 1=hair, 7=transparent (eyelash etc.)
uniform bool useAdvancedAlpha = true;

// Phong lighting uniforms
uniform vec3 lightPos = vec3(10.0, 20.0, 10.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 viewPos = vec3(0.0, 0.0, 0.0);  // 添加默认值
uniform float ambientStrength = 0.35;  // stronger ambient
uniform float specularStrength = 0.6;
uniform int shininess = 32;

vec3 calculatePhongLighting(vec3 albedoColor, vec3 normal, vec3 fragPos) {
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    vec3 viewDir  = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 ambient  = ambientStrength * lightColor;
    vec3 diffuse  = diff * lightColor;
    vec3 specular = specularStrength * spec * lightColor;

    return (ambient + diffuse + specular) * albedoColor;
}

void main() {
    vec4 albedo = texture(texture_diffuse1, TexCoords);

    // 基础材质分类：头发和透明材质使用更宽松的阈值和平滑 alpha
    float threshold = alphaThreshold;
    if (materialType == 1) {          // hair
        threshold *= 0.5;             // 更宽松
    } else if (materialType == 7) {   // transparent (eyelash etc.)
        threshold *= 0.8;
    }

    // Cutout（硬剔除）最低限制：极低 alpha 直接丢弃
    if (albedo.a < threshold) {
        discard;
    }

    // 平滑 alpha 过渡：使用 smoothstep 让边缘柔和
    if (useAdvancedAlpha) {
        float smoothRange = 0.15; // 过渡范围
        albedo.a = smoothstep(threshold, threshold + smoothRange, albedo.a);
    }

    vec3 litColor = calculatePhongLighting(albedo.rgb, Normal, FragPos);
    FragColor = vec4(litColor, albedo.a);
}