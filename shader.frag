#version 420
uniform sampler2D tex;
uniform float iTime;
out vec4 fragCol;

vec4 alphablend(vec4 bottom, vec4 top) {
	float outalpha = (1.0-top.w) + bottom.w * top.w;
  vec3 outcol = (bottom.xyz*bottom.w*top.w + top.xyz*(1.0-top.w))/outalpha;
  return vec4(outcol, outalpha);
}

vec4 OSD(vec2 uv, int idx) {

	return texture(tex, vec2(uv.x, -uv.y)).xxyy;
}

void main() {
		// Normalized pixel coordinates (from -1 to 1)
		vec2 uv = (gl_FragCoord.xy - vec2(960.0, 540.0))/vec2(960.0, 960.0);

		fragCol = alphablend(vec4(1.0, sin(uv*10.0 + sin(iTime)*5.0), 1.0), OSD(uv/2.0, 0));
}