#version 330 core
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

void main()
{
    FragColor = vec4(7.0,7.0,7.0,1.0); // set alle 4 vector values to 1.0
    float brightness = dot(FragColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
       BrightColor = vec4(FragColor.rgb, 1.0);
    else
       BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}