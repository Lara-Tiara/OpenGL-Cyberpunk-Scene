#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D texture_diffuse1;   // 底色贴图
uniform sampler2D texture_emissive1;   // 自发光贴图（亮=发光, 黑=不发光）
uniform float uTime;                  // 世界时间（秒）
uniform float emissiveStrength = 1.0; // 基础发光强度
uniform vec3  emissiveTint = vec3(1.0); // 发光色调
uniform float baseMix = 1.0;          // 底色显示比例(1=全部显示, 0=只显示发光)

// 生成一个呼吸 + 流动条纹 + 闪烁混合的效果
float computeNeonFactor(float t, vec2 uv)
{
    // 呼吸 (慢速平滑变亮变暗)
    float breathe = 0.5 + 0.5 * sin(t * 1.5);
    // 纵向流动条纹 (沿 V 方向滚动)
    float flow = fract(uv.y + t * 0.25);
    float flowBand = smoothstep(0.2, 0.35, flow) * (1.0 - smoothstep(0.65, 0.8, flow));
    // 闪烁 (不规则快速抖动)
    float flicker = 0.85 + 0.15 * sin(t * 30.0 + sin(t * 7.0));
    // 组合：主亮度由呼吸控制，叠加流动条与微弱闪烁
    return breathe * (0.7 + 0.3 * flowBand) * flicker;
}

void main()
{
    vec3 baseColor = texture(texture_diffuse1, TexCoords).rgb; // 底色
    vec3 emisTex   = texture(texture_emissive1, TexCoords).rgb; // 自发光贴图黑色处为0，不发光
    // 计算动画系数
    float neonFactor = computeNeonFactor(uTime, TexCoords);
    // 最终发光：贴图 * 动画 * 强度 * 色调
    vec3 emissive = emisTex * neonFactor * emissiveStrength * emissiveTint;

    // 混合底色与发光：底色先调暗，以突出霓虹
    vec3 finalColor = baseColor * baseMix * 0.4 + emissive; // 0.4 可调：底色暗化系数
    FragColor = vec4(finalColor, 1.0); // 同时显示发光与底色
}
