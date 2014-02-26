uniform mat4 u_worldViewMatrix;
uniform vec3 u_cameraPosition;

vec4 getPW( vec4 a_position){
	return  a_position * u_worldViewMatrix;	 
}

vec3 getCameraPosition(){
   return u_cameraPosition;
}
