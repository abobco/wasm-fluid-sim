precision mediump float;

varying vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int numParticles;
uniform vec2 translations[MAX_FRAGMENT_UNIFORM_VECTORS - 32];

float particleRadius = 0.05;

#define MERGE_RADIUS 0.05

float circleSDF(vec2 point, float radius) {
  point =
      (vec2(point.x * SCREEN_HEIGHT / SCREEN_WIDTH, point.y) + vec2(1.0, 1.0)) *
      0.5;
  float line_resolition = 0.005; // controls density of the field lines
  float scaled_radius =
      line_resolition * radius; // scale down the radius to fit the

  vec2 conv_factor = vec2(SCREEN_WIDTH, SCREEN_HEIGHT) * line_resolition;
  // point.x *= SCREEN_HEIGHT / SCREEN_WIDTH;
  // vec2 tc = vec2(TexCoords.x * SCREEN_HEIGHT / SCREEN_HEIGHT, TexCoords.y);
  vec2 position = (TexCoords - point) * conv_factor;

  return length(position) - scaled_radius;
  return length(point) - radius;
}

// rounds off edges between edges of shapes
float round_merge(float shape1, float shape2, float radius) {
  vec2 intersection_space = vec2(shape1 - radius, shape2 - radius);
  intersection_space = min(intersection_space, 0.0);

  float insideDistance = -length(intersection_space);
  float simpleUnion = min(shape1, shape2);
  float outsideDistance = max(simpleUnion, radius);

  return insideDistance + outsideDistance;
}

// returns minimum distance from texture coordinate to a surface
float sdf_min() {
  float d = 1000000.0;
  int collision_count = 0;
  int collision_max = 5;
  // const int MAX_ITER = numParticles;

  for (int i = 0; i < MAX_FRAGMENT_UNIFORM_VECTORS - 32; i += 2) {
    if (translations[i].x == 0.0 && translations[i].y == 0.0)
      break;
    float d1 = circleSDF(translations[i], particleRadius);
    float d2 = circleSDF(translations[i + 1], particleRadius);

    // return closest distance
    float td = round_merge(d1, d2, MERGE_RADIUS);

    // d = min(td, d);
    d = round_merge(d, td, MERGE_RADIUS);
    // if (d < 0.0)
    //   collision_count++;
    // if (collision_count >= collision_max)
    //   break;
  }

  return d;
}

void main() {
  vec4 texColor = texture2D(screenTexture, TexCoords);
  float d = sdf_min();
  vec3 col = vec3(1.0) - sign(d) * vec3(0.1, 0.4, 0.7);
  col *= 1.0 - exp(-3.0 * abs(d));
  col *= 0.8 + 0.2 * cos(150.0 * d);
  col = mix(col, vec3(1.0), 1.0 - smoothstep(0.0, 0.01, abs(d)));
  gl_FragColor = mix(vec4(col, 1.0), texColor, 0.5);
}