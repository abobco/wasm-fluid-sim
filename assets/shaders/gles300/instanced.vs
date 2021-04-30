layout(location = 0) in vec3 aPos;

uniform float particle_radius;
uniform vec2 translations[MAX_VERTEX_UNIFORM_VECTORS - 2];
void main() {
  vec2 translation = translations[gl_InstanceID];
  gl_Position = vec4(aPos * particle_radius + vec3(translation, 0.0), 1.0);
}