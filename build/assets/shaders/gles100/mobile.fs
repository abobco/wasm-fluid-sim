precision mediump float;
// out vec4 FragColor;
uniform vec4 color;
void main() {
  // FragColor = vec4(0.2f, 0.2f, 0.2f, 1.0f);
  gl_FragColor = color;
}