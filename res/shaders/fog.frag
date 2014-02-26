varying vec3 cameraPosition;
varying vec4 positionWorldViewSpace;	 

//float fogNear=1000;
//float fogFar=2000;

vec4 applyFog(in vec4 color){
//    const vec3 fogColor = vec3(1.0,1.0,1.0);
//    float depth = gl_FragCoord.z / gl_FragCoord.w;
//    float fogFactor = smoothstep(fogNear, fogFar, depth);
//    color = mix(color, vec4(fogColor, color.w), fogFactor);

    return color;
}

