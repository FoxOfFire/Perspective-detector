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
uniform int reRand = 1;


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

uint rnd(uint r)
{	
	r += ( r << 10u );
	r ^= ( r >>  6u );
	r += ( r <<  3u );
	r ^= ( r >> 11u );
	r += ( r << 15u );
	return r;
	/*
	r = (r * 9302 + 49297)%122280;
	r = r ^ (r << 13);
	r = r ^ (r >> 17);
	r = r ^ (r << 5);
	return r;
	*/
}
vec4 pointSelect(){
	r = seed;
	r = rnd(r) ^ int(uv.x*100000.0*texSize.x);
	r = rnd(r) ^ int(uv.y*100000.0* texSize.y);
	r = rnd(r);
	vec2 size =  vec2(texSize);
	vec2 texelSize = 1.0 / size;
	int kernel_w = 2*int(kernelSize) ;
	int kernel_p = 2*kernel_w + 1;
	for(int k = 0; k < kernelSize*kernelSize*vecSize*vecSize; k++){
		r = rnd(r^(k+seed));
		int x = int(r);
		r = rnd(r^(k+seed));
		int y = int(r);
		int ofx = (x % int(size.x));
		int ofy = (y % int(size.y));
		vec2 samplePos = vec2(ofx,ofy)*texelSize;
		vec4 pixel = texture(srcImg, samplePos);
		if (pixel.x == 1){
			return vec4(samplePos,0,1);
		}
	}
	return vec4(0);
}
vec4 undupe(){
	float invVecSize = 1.0 / float(vecSize);
	vec2 invImgSize = 1.0 / vec2(texSize);
	float root2 = sqrt(2);

	int hit = 0;
	vec4 thisP = texture(srcImg,uv);
	for(int i = 0; i < vecSize; i++){
		for(int j = 0; j < vecSize; j++){
			vec2 newuv = vec2(i,j)*invVecSize;
			if(newuv.x < uv.x && newuv.y < uv.y || length(newuv - uv) < invVecSize){
				continue;
			}
			vec2 pos = abs(thisP.xy-texture(srcImg,newuv).xy) - invImgSize;
			if( pos.x < 0 && pos.y < 0){
				return vec4(0);
			}

		}
	}
	return thisP;
}
vec4 pointDisplay(){
	float invVecSize = 1.0 / float(vecSize);
	float invImgSize = 1.0 / float(min(texSize.x,texSize.y));

	int hit = 0;
	for(int i = 0; i < vecSize; i++){
		for(int j = 0; j < vecSize; j++){
			vec2 pos = texture(srcImg,vec2(i,j)*invVecSize).xy;
			if(length(uv-pos) < invImgSize){
				hit++;
			}

		}
	}
	if (hit > 7){
		return vec4(1,1,1,1);
	}
	hit = hit;
	return vec4(hit%2,(hit/2)%2,(hit/4)%2,1);
}
vec4 vecgen(){

	r = seed;
	r = rnd(r) ^ int(uv.x*100000.0*texSize.x);
	r = rnd(r) ^ int(uv.y*100000.0* texSize.y);
	float invVecSize = 1.0 / float(vecSize);
	float invImgSize = 1.0 / float(max(texSize.x,texSize.y));
	float pi = 3.14159265359;

	int hit = 0;
	int miss = 0;
	vec4 pos = texture(srcImg,uv);
	int x = int(rnd(r));
	r = rnd(r) ^ int(uv.x*100000.0*texSize.x);
	r = rnd(r) ^ int(uv.y*100000.0* texSize.y);
	int y = int(rnd(r));
	vec2 resUv = vec2(1);
	vec4 resCol = vec4(0);
	bool found = false;
	if (pos.w < 1){
		if (reRand == 0){
			return vec4(0);
		}else{
			for(int i = 0; i < vecSize; i++){
				for(int j = 0; j < vecSize; j++){
					resUv = vec2((i+x)%vecSize,(j+y)%vecSize)*invVecSize;
					pos = texture(srcImg,resUv);
					found = pos.z > 0.0;
					if(found){
						break;
					}

				}
				if(found){
					break;
				}
			}
			r = rnd(r) ^ int(uv.x*100000.0*texSize.x);
			r = rnd(r) ^ int(uv.y*100000.0* texSize.y);
			x = int(rnd(r));
			r = rnd(r) ^ int(uv.x*100000.0*texSize.x);
			r = rnd(r) ^ int(uv.y*100000.0* texSize.y);
			y = int(rnd(r));
		}
	}
	for(int i = 0; i < vecSize; i++){
		for(int j = 0; j < vecSize; j++){
			resUv = vec2((i+x)%vecSize,(j+y)%vecSize)*invVecSize;
			resCol = texture(srcImg,resUv);
			found = resCol.z > 0.0 && length(resCol.xy-pos.xy)-length(invImgSize) > 0;
			if(found){
				break;
			}

		}
		if(found){
			break;
		}
	}

	resUv = (resCol.xy - pos.xy  );

	vec2 norUv = normalize(resUv);
	float theta ;
	if ( abs(resUv.x) < 0.0001){
		theta = 0;
	}
	else{
		theta = ((atan(resUv.y,resUv.x))/pi);
		if (theta < 0){
			theta += 1;
		}
	}

	resUv = (resUv +1)/2;
	return vec4(pos.xy,theta,1);
}

vec4 linegen(int mode){
	float pi = 3.14159265359;
	float texelSize = 1.0 /vecSize;
	vec2 intTex = 1.0 / texSize ;
	float threshold = lineTh2*length(intTex);
	float invImgSize = 1.0 / float(min(texSize.x,texSize.y));

	int hit = 0;
	int miss = 0;
	vec4 currtex =texture(srcImg,uv);
	for(uint i = 0; i < vecSize; i++){
		for(uint j = 0; j < vecSize; j++){
			vec2 pos = vec2(i,j)*texelSize;
			vec4 line = texture(srcImg, pos);
			if (line.w < 1){
				miss++;
				continue;
			}
			vec2 p = (currtex.xy*2-1)-(line.xy *2 +1);
			if (mode == 1){
				p = uv-line.xy;
			}

			
			float theta = (line.z*2 - 1)*pi;
			float sdf = abs(p.x*cos(theta)+p.y*sin(theta)) ;
			if (mode == 1){
				if(line.w > 0 && sdf< invImgSize ){
					return vec4(1);
				}
			}
			else if( sdf< threshold ){
				hit++;
			}
			else{
				miss++;
			}

		}
	}
	if(mode == 1){
		return vec4(0);
	}
	if (hit+miss <1){
		return vec4(1,0,1,1);
	}
	float hitrate = float(hit)/float(hit+miss);
	hitrate = sqrt(hitrate);
	if (hitrate < lineTh){
		return vec4(0);
	}

	return texture(srcImg,uv);
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
		FragColor = undupe();
	}
	else if (mode == 12){
		FragColor = pointDisplay();
	}
	else if (mode == 13){
		FragColor = vecgen();
	}
	else if (mode == 14){
		FragColor = linegen(0);
	}
	else if (mode == 15){
		FragColor = linegen(1);
	}
	else{
		FragColor = vec4(1,0,1,1);
	}

}
