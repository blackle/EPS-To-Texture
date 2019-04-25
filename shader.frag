#version 420
uniform sampler2D tex;
uniform int donttouch;
out vec4 fragCol;

void main() {
		// Normalized pixel coordinates (from -1 to 1)
		vec2 uv = (gl_FragCoord.xy - vec2(960.0, 540.0))/vec2(960.0, 960.0);

		fragCol = texture(tex, vec2(uv.x, -uv.y)); //colour grading
}