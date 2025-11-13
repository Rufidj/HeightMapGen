#version 330 core  
  
in vec3 fragColor;  
in vec2 fragTexCoord;  
  
uniform bool useTexture;  
uniform sampler2D textureSampler;  
  
out vec4 finalColor;  
  
void main() {  
    if (useTexture) {  
        finalColor = texture(textureSampler, fragTexCoord);  
    } else {  
        finalColor = vec4(fragColor, 1.0);  
    }  
}
