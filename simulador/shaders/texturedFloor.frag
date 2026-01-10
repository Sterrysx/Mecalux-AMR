#version 330 core

in vec4 vertexSCO;
in vec3 normalSCO;
in vec2 fragTexCoord;
in vec3 fmatamb;
in vec3 fmatdiff;
in vec3 fmatspec;
in float fmatshin;

out vec4 FragColor;

vec3 llumAmbient = vec3(0.2, 0.2, 0.2);
vec3 colFocus = vec3(1, 1, 1);

uniform mat4 proj;
uniform mat4 view;
uniform mat4 TG;

uniform vec3 posFocus;
uniform sampler2D floorTexture;

vec3 Lambert (vec3 NormSCO, vec3 L) 
{
    vec3 colRes = llumAmbient * fmatamb;

    if (dot (L, NormSCO) > 0)
      colRes = colRes + colFocus * fmatdiff * dot (L, NormSCO);
    return (colRes);
}

vec3 Phong (vec3 NormSCO, vec3 L, vec4 vertSCO) 
{
    vec3 colRes = Lambert (NormSCO, L);

    if (dot(NormSCO,L) < 0)
      return colRes;

    vec3 R = reflect(-L, NormSCO);
    vec3 V = normalize(-vertSCO.xyz);

    if ((dot(R, V) < 0) || (fmatshin == 0))
      return colRes;
    
    float shine = pow(max(0.0, dot(R, V)), fmatshin);
    return (colRes + fmatspec * colFocus * shine); 
}

void main()
{	
    vec3 L = normalize(posFocus-vertexSCO.xyz);
    vec3 norm = normalize(normalSCO);
    vec3 fcolor = Phong(norm,L,vertexSCO);
    
    // Sample and blend texture
    vec3 texColor = texture(floorTexture, fragTexCoord).rgb;
    fcolor = fcolor * texColor;
    
    FragColor = vec4(fcolor,1);
}
