#version 450 core

layout(location = 0) out vec4 FragColor;

uniform sampler2D srcImg;

uniform int mode = 0;
//blur
uniform int kernelSize = 3;
uniform ivec2 texSize = ivec2(100);
uniform int vecSize = 100;
uniform float sigma = 1.0;
uniform float t_lower = .3;
uniform float t_upper = .6;
uniform int last_hyst = 10;
uniform uint seed = 0;
uniform float lineTh = .1;
uniform float lineTh2 = 1;


in vec2 uv;

vec4 gaussian(){
	float pi = 3.14159265359;
	float sigsqr = 1.0/(2.0*pow(sigma,2.0));
	float normal = 1.0/(2.0*pi*pow(sigma,2.0));
	vec2 texelSize = 1.0 / vec2(texSize);

	float Col = 0.0;
	float weights = 0.0;
	for(int i = -kernelSize; i <= kernelSize; i++){
		for(int j = -kernelSize; j <= kernelSize; j++){
			float ofcol = texture(srcImg, uv + vec2(i,j) * texelSize).x;
			float w = exp(-((i*i+j*j)*sigsqr)*normal);
			Col += ofcol * w;
			weights += w;
			
		}
	}
	return vec4(vec3(Col/weights),1);
}

vec4 sobel(int mode){
	vec2 texelSize = 1.0 / vec2(texSize);

	mat3 x;
	x[0] = vec3(-1,0,1);
	x[1] = vec3(-2,0,2);
	x[2] = vec3(-1,0,1);

	mat3 y;
	y[0] = vec3( 1, 2, 1);
	y[1] = vec3( 0, 0, 0);
	y[2] = vec3(-1,-2,-1);

	float resx = 0.0;
	float resy = 0.0;
	
	for(int i = -1; i <= 1; i++){
		for(int j = -1; j <= 1; j++){
			float ofcol = texture(srcImg, uv + vec2(i,j) * texelSize).x;
			resx = resx + ofcol * x[i+1][j+1];
			resy = resy + ofcol * y[i+1][j+1];
			
		}
	}
	float epsilon = 0.0001;
	if (resx < epsilon){
		resx = sign(resy)*epsilon;
	}
	if (mode == 0){
		return vec4(vec3(sqrt(resx*resx+resy*resy)),1);
	}
	else if (mode == 1){
		return vec4(vec3(atan(resy,resx)),1);
	}
	else if (mode == 2){
		float norm = sqrt(resx*resx+resy*resy);
		return vec4(norm,(vec2(resx,resy)/norm+1)/2,1);
	}
	else{
		return vec4(sqrt(resx*resx+resy*resy),atan(resy,resx),0,1);
	}

}
vec4 non_max_supress(){
	float pi = 3.14159265359;
	float angle = texture(srcImg, uv).y;
	float val = texture(srcImg, uv).x;
	if (angle < 0) {
		angle += pi;
	}
	angle *= 180 / pi;
	float q = 1.0;
	float r = 1.0;
	vec2 texelSize = 1.0 / vec2(texSize);
	if ( 157.5 <= angle || angle < 22.5){
		q = texture(srcImg, uv + vec2(0,+1)*texelSize).x;
		r = texture(srcImg, uv + vec2(0,-1)*texelSize).x;
	}
	else if ( 22.5 <= angle && angle < 67.5){
		q = texture(srcImg, uv + vec2(-1,+1)*texelSize).x;
		r = texture(srcImg, uv + vec2(+1,-1)*texelSize).x;
	}
	else if ( 67.5 <= angle && angle < 112.5){
		q = texture(srcImg, uv + vec2(+1,0)*texelSize).x;
		r = texture(srcImg, uv + vec2(-1,0)*texelSize).x;
	}
	else if ( 112.5 <= angle && angle < 157.5){
		q = texture(srcImg, uv + vec2(-1,-1)*texelSize).x;
		r = texture(srcImg, uv + vec2(+1,+1)*texelSize).x;
	}

	if(val > q && val > r){
		return vec4(vec3(val),0);
	}
	else{
		return vec4(0,0,0,1);
	}
}

vec4 threshold(){
	float val = texture(srcImg, uv).x;
	float strong = 0;
	float weak = 0;
	if (val >= t_lower){
		weak = 1;
	}
	if (val >= t_upper){
		strong = 1;
	}
	return vec4(val,weak,strong,1);

}

vec4 hysteresis(){
	vec4 col = texture(srcImg, uv);
	if ( last_hyst <= 0){
		return vec4(vec3(col.z),1);
	}else if (col.y < 1 || col.z > 0){
		return col;
	}
	vec2 texelSize = 1.0 / vec2(texSize);
	for(int i = -1; i <= 1; i++){
		for(int j = -1; j <= 1; j++){
			if  (texture(srcImg, uv + vec2(i,j) * texelSize).z > 0){
				return vec4(col.xy,1,1);
			}
		}
	}
	return col;
}
uint r = 123412341;

uint rnd()
{
	r = (r * 9302 + 49297)%122280;
	r = r ^ (r << 13);
	r = r ^ (r >> 17);
	r = r ^ (r << 5);
	return r;
}
vec4 pointSelect(){

	r = int(round((uv.x+uv.y)*5134123))+seed;
	vec2 size =  vec2(texSize);
	vec2 texelSize = 1.0 / size;
	uint kernel_w = 4*kernelSize +1;
	for(int k = 0; k < kernelSize; k++){
		uint x = rnd() % int(size.x);
		uint y = rnd() % int(size.y);
		uint sx = rnd()%kernel_w;
		uint sy = rnd()%kernel_w;
		for(uint i = 0; i < kernel_w; i++){
			for(uint j = 0; j < kernel_w; j++){
				uint ofx = x + ((sx + i) % kernel_w);
				uint ofy = y + ((sy + j) % kernel_w);
				vec4 pixel = texture(srcImg, uv + vec2(ofx,ofy)*texelSize);
				if (pixel.x == 1){
					return vec4(ofx*texelSize.x,ofy*texelSize.y,.1,1);
				}

			}
		}
	}
	return vec4(0,0,0,1);
}
vec4 vecgen(int mode){
	vec3 pos = texture(srcImg, uv).xyz;
	if (pos.z == 0) {
		return vec4(0);
	}
	r = int(round((uv.x+uv.y)*5000))+seed;
	vec2 texelSize = 1.0 /vec2(vecSize);
	
	uint sx = rnd();
	uint sy = rnd();
	bool got_line = false;
	int hits = 0;
	int total= 0;
	vec3 line_data = vec3(0);
	for(uint i = 0; i < vecSize; i++){
		for(uint j = 0; j < vecSize; j++){
			uint ofx =  (sx + i) % vecSize;
			uint ofy =  (sy + j) % vecSize;
			vec4 pixel = texture(srcImg, uv + vec2(ofx,ofy)*texelSize);
			if (pixel.z == 0){
				continue;
			}
			if (!got_line){ 
				vec2 dir = pixel.xy - pos.xy;
				float norm = sqrt(dir.x*dir.x + dir.y*dir.y);
				if (norm < 0.0000001){
					hits++;
					total++;
					continue;
				}
				vec2 nordir =  vec2(dir.y*norm,dir.x*norm);
				line_data = vec3(nordir,-nordir.x*pos.x-nordir.y*pos.y);
				got_line = true;
			}
			float linedist = abs(line_data.x *pos.x + line_data.y * pos.y +line_data.z);
			if (linedist < lineTh2){
				hits++;
			}
			total++;

		}
	}
	float rate = float(hits)/float(total);
	if (rate < lineTh){
		return vec4(0);
	}
	else if (mode == 1){
		return vec4(vec3(rate),1);
	}
	else if (mode == 2){
		return vec4(line_data,1);
	}
	else{
		return vec4(line_data,rate);
	}

}

vec4 linegen(){
	vec2 texelSize = 1.0 /vec2(vecSize);
	
	for(uint i = 0; i < vecSize; i++){
		for(uint j = 0; j < vecSize; j++){
			vec4 line = texture(srcImg, vec2(i,j)*texelSize);
			if (abs(line.w) + abs(line.z) + abs(line.y) + abs(line.x) < 0.001){
				continue;
			}
			float linedist = abs(line.x *uv.x + line.y * uv.y + line.z);
			if (linedist < lineTh2){
				return vec4(1);
			}

		}
	}
	return vec4(0,0,0,1);
}



void main(){
	if(mode == 0){
		FragColor = texture(srcImg, uv);
	}
	else if( mode == 1){
		vec4 texCol = texture(srcImg, uv);
		FragColor =  vec4(vec3((texCol.x + texCol.y + texCol.z)/3.0),1);
	}
	else if (mode == 2){
		FragColor = gaussian();
	}
	else if (mode == 3){
		FragColor =  sobel(0);
	}
	else if (mode == 4){
		FragColor =  sobel(1);
	}
	else if (mode == 5){
		FragColor =  sobel(2);
	}
	else if (mode == 6){
		FragColor =  sobel(3);
	}
	else if (mode == 7){
		FragColor =  non_max_supress();
	}
	else if (mode == 8){
		FragColor =  threshold();
	}
	else if (mode == 9){
		FragColor =  hysteresis();
	}
	else if (mode == 10){
		FragColor = pointSelect();
	}
	else if (mode == 11){
		FragColor = vecgen(1);
	}
	else if (mode == 12){
		FragColor = vecgen(2);
	}
	else if (mode == 13){
		FragColor = vecgen(0);
	}
	else if (mode == 14){
		FragColor = linegen();
	}
	else{
		FragColor = vec4(1,0,1,1);
	}

}
