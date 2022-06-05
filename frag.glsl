#version 330 core

out vec4 FragColor;

uniform vec2 lowerLeft;
uniform vec2 upperRight;
uniform vec2 viewportDimensions;
uniform int maxIterations;
uniform vec4 palette[64];
uniform float paletteOffset;
uniform int numColors;
uniform bool applySmooth;

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 getColor(float x) {
  int i = 0;
  while (x > palette[i + 1].x)
    ++i;
  return vec4(mix(palette[i].yzw, palette[i + 1].yzw, (x - palette[i].x) / (palette[i + 1].x - palette[i].x)), 1);
}

void main() {
  FragColor = vec4(0, 0, 0, 0);
  vec2 c = mix(lowerLeft, upperRight, gl_FragCoord.xy / viewportDimensions.xy);
  //vec2 c = mix(lowerLeft, upperRight, gl_FragCoord.xy / viewportDimensions.xy);
  float x0, y0, x, y, x2, y2;
  x = y = x2 = y2 = 0;
  x0 = c.x, y0 = c.y;

  //cardioid checking - check if point is inside the biggest 2 bulbs (which are in the set)
  float q = (x0 - 0.25) * (x0 - 0.25) + y0 * y0;
  if (q * (q + (x0 - 0.25)) <= 0.25 * y0 * y0) {
    FragColor += vec4(0, 0, 0, 1);
    return;
  }

  //this is an optimized algorithm for computing iterations,
  //minimizing the number of multiplications made,
  //sacrificing clarity for speed

  //this slightly modified version of the original algorithm
  //uses a larger bailout radius, namely 2^8
  float iteration = 0;
  while (x2 + y2 <= (1 << 16) && iteration < maxIterations) {
    y = (x + x) * y + y0;
    x = x2 - y2 + x0;
    x2 = x * x;
    y2 = y * y;
    ++iteration;
  }
  if (iteration < maxIterations) {
    //the coloring function used here is explained over here https://www.math.univ-toulouse.fr/~cheritat/wiki-draw/index.php/Mandelbrot_set,
    //though the following implementation (inspired by Wikipedia) differs a bit from the link above, while the
    //mentioned link provides intuition for the formula
    if (applySmooth)
      iteration -= (log(log(x2 + y2) / 2.0) - log(log(exp2(8)))) / log(2.0);
    FragColor = getColor(fract((iteration + paletteOffset) / numColors));
  } else {
    FragColor = vec4(0, 0, 0, 1);
  }
}