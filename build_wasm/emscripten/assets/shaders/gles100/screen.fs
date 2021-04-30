precision mediump float;
// out vec4 FragColor;

varying vec2 TexCoords;

uniform sampler2D screenTexture;

void main() { gl_FragColor = texture2D(screenTexture, TexCoords); }