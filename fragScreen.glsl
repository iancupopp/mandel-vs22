#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 viewportDimensions;

void main() {
  float x_offset = 1.0 / viewportDimensions.x;
  float y_offset = 1.0 / viewportDimensions.y;
  float weights[9] = float[](
    -1, 0, 1,
    -2, 0, 2, 
    -1, 0, 1
  );
  float weights2[9] = float[](
    -1, -2, -1,
    -2, 0, 2, 
    -1, 2, 1
  );
  int k = 0;
  FragColor = vec4(0, 0, 0, 0);
  for (int i = -1; i <= 1; ++i)
    for (int j = -1; j <= 1; ++j)
      FragColor += weights[k++]*  texture(screenTexture, TexCoords + vec2(i * x_offset, j * y_offset));
  
  if (texture(screenTexture, TexCoords)!=vec4(0, 0, 0, 1))
  FragColor = 1 - FragColor;
  //FragColor = texture(screenTexture, TexCoords);
}