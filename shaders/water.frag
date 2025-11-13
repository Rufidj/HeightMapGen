#version 330 core  
  
in vec3 fragColor;  
in vec2 fragTexCoord;  
  
uniform float waterAlpha;  
uniform bool useWaterTexture;  // AGREGAR  
uniform sampler2D waterTextureSampler;  // AGREGAR  
  
out vec4 finalColor;  
  
void main() {  
    gl_FragDepth = gl_FragCoord.z - 0.00001;  
      
    if (useWaterTexture) {  
        // Mezclar textura con color base  
        vec4 texColor = texture(waterTextureSampler, fragTexCoord);  
        finalColor = vec4(mix(fragColor, texColor.rgb, 0.7), waterAlpha);  
    } else {  
        finalColor = vec4(fragColor, waterAlpha);  
    }  
}
