#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 FragPos;     // World position
in vec3 Normal;      // World normal

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_emissive1; // 自发光贴图
uniform float emissiveIntensity = 0.0; // 旧：用于静态发光强度（保留兼容）

// 新增：时间与发光控制
uniform float uTime = 0.0;                 // 世界时间（秒）- 添加默认值
uniform bool  isEmissive = false;   // 是否启用 emissive 动画（仅 VMModel）
uniform vec3  emissiveColor = vec3(1.0); // 发光基色（默认白色）

// Phong lighting uniforms
uniform vec3 lightPos = vec3(10.0, 20.0, 10.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 viewPos = vec3(0.0, 0.0, 0.0);  // 添加默认值
uniform float ambientStrength = 0.2;
uniform float specularStrength = 0.5;
uniform int shininess = 32;

vec3 calculatePhongLighting(vec3 albedoColor, vec3 normal, vec3 fragPos) {
    vec3 norm = normalize(normal);
    vec3 ambient = ambientStrength * lightColor;
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;
    return (ambient + diffuse + specular) * albedoColor;
}

void main()
{    
    vec3 base = texture(texture_diffuse1, TexCoords).rgb;
    vec3 lit = calculatePhongLighting(base, Normal, FragPos);

    // 发光动画：仅在 isEmissive 为真时根据时间调制
    vec3 emissive = vec3(0.0);
    if (isEmissive) {
        // 呼吸参数
        float speed = 2.0;   // 呼吸速度
        float minI  = 0.2;   // 最暗
        float maxI  = 2.5;   // 最亮
        float t = 0.5 + 0.5 * sin(uTime * speed);
        float emissiveFactor = mix(minI, maxI, t);

        // 仅霓虹区域发光（黑色区域不发光）
        vec3 emisTex = texture(texture_emissive1, TexCoords).rgb;
        emissive = emissiveColor * emisTex * emissiveFactor;
    } else if (emissiveIntensity > 0.0) {
        // 兼容旧路径：静态发光强度（非动画）
        vec3 emisTex = texture(texture_emissive1, TexCoords).rgb;
        emissive = emisTex * emissiveIntensity;
    }

    vec3 finalColor = lit + emissive;
    FragColor = vec4(finalColor, 1.0);
}