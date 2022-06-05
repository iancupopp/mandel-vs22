//OpenGL libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "shader.h"

//Used for debug/output
#include <iostream>

//Used for loading config
#include "config_parser.h"

int windowWidth, windowHeight;
int screenWidth, screenHeight;
float scale;
double lastScroll = 0;

//Color/palette related stuff
Point palette[20];
int paletteSz;

//Graph area displayed by the program
float minXStart = -3.5, maxXStart = 2.5;
float minX = -2.5, minY = -2.0; //lower-left corner
float maxX = 1.5, maxY = 2.0; //upper-right corner

//Variable number of iterations
int maxIterations, iterationStep;

//Zoom in coordinates - mainly used for benchmarking / showcasing
//double zoomXPos = 0.32295092104429057, zoomYPos = 0.49139380568602198;
float zoomXPos = -0.72993051140825282, zoomYPos = -0.24047174011417854;
bool shouldZoomIn = false;
float zoomSpeed;

//Assure that the area on the XY system is proportional to the resolution
void areaSanityCheck() {
  minY = maxY - (maxX - minX) * (float)windowHeight / (float)windowWidth;
}

//Load the default view with the main Brot
void defaultView() {
  minX = minXStart, maxX = maxXStart;
  maxY = (maxXStart - minXStart) / 2 * windowHeight / windowWidth;
  minY = -maxY;
  areaSanityCheck();
}

void resizeMandel(const int width, const int height) {
  float scaleX = float(width - windowWidth) / windowWidth;
  float scaleY = float(height - windowHeight) / windowHeight;
  float diffX = maxX - minX;
  float diffY = maxY - minY;
  minX -= scaleX * diffX / 2;
  maxX += scaleX * diffX / 2;
  minY -= scaleY * diffY / 2;
  maxY += scaleY * diffY / 2;
  windowWidth = width, windowHeight = height;
  screenWidth = scale * width, screenHeight = scale * height;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

//Resize window callback function
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  //Update displayed area (both screen size and graph area)
  if (width > 0 && height > 0) //avoid division by 0
    resizeMandel(width, height);
  glViewport(0, 0, width, height);
}

//Maximize window callback function
void window_maximize_callback(GLFWwindow* window, int maximized) {
  int width, height;
  glfwGetWindowSize(window, &width, &height);
  resizeMandel(width, height);
}

//Used for panning
//First check if mouse left button is pressed
static void cursor_position_callback(GLFWwindow* window, double new_x, double new_y) {
  static double x = -1, y = -1;
  if (x != -1 && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    double xDiff = maxX - minX;
    double yDiff = maxY - minY;
    double xDelta = (x - new_x) / windowWidth * xDiff;
    double yDelta = -(y - new_y) / windowHeight * yDiff;
    minX += xDelta, maxX += xDelta;
    minY += yDelta, maxY += yDelta;
  }
  x = new_x, y = new_y;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (action != GLFW_PRESS)
    return;

  if (key == GLFW_KEY_ESCAPE)
    glfwSetWindowShouldClose(window, true);
  else if (key == GLFW_KEY_UP)
    maxIterations = std::min(maxIterations + iterationStep, 4096);
  else if (key == GLFW_KEY_DOWN)
    maxIterations = std::max(4, maxIterations - iterationStep);
  else if (key == GLFW_KEY_B) {
    shouldZoomIn = !shouldZoomIn;
    if (shouldZoomIn)
      defaultView();
  }
}

void zoom(float xPos, float yPos, float amount) {
  float xDiff = maxX - minX;
  float yDiff = maxY - minY;

  minX += amount * xPos / windowWidth * xDiff;
  maxX -= amount * (1 - xPos / windowWidth) * xDiff;

  minY += amount * (1 - yPos / windowHeight) * yDiff;
  maxY -= amount * yPos / windowHeight * yDiff;
}

void zoomIn() {
  //be careful not to zoom more than the floating precision allows
  if (std::min(maxX - minX, maxY - minY) < 0.0003) {
    shouldZoomIn = false;
    return;
  }

  //check if zoom center is within the area displayed on the screen,
  //otherwise return to the default view
  if (zoomXPos < minX || zoomXPos > maxX
    || zoomYPos < minY || zoomYPos > maxY) {
    defaultView();
    return;
  }

  //convert XY axes coordinates to screen coordinates
  float xPos = (zoomXPos - minX) / (maxX - minX) * windowWidth;

  //screen coordinates are measured from top-left corner
  float yPos = (1 - (zoomYPos - minY) / (maxY - minY)) * windowHeight;

  //apply zoom
  zoom(xPos, yPos, 0.01 * zoomSpeed);
}

void scroll_callback(GLFWwindow* window, double xOffset, double yOffset) {
  double cursorX, cursorY;
  glfwGetCursorPos(window, &cursorX, &cursorY);
  screenWidth = windowWidth / 2, screenHeight = windowHeight / 2;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  lastScroll = glfwGetTime();
  zoom(cursorX, cursorY, yOffset * 0.1);
}

int main() {
  //Load config 
  CfgParser parser;
  parser.parseFile("mandel.cfg");
  //Palette
  parser.getPalette(palette, paletteSz);
  bool applySmooth = parser.findBool("smooth");
  int numColors = parser.findInt("n_colors");
  //Supersampling
  scale = parser.findFloat("scale");
  float paletteOffset = 0, paletteOffsetStep = parser.findFloat("palette_cycle_step");
  //Iterations
  zoomXPos = parser.findFloat("x_zoom"), zoomYPos = parser.findFloat("y_zoom");
  maxIterations = parser.findInt("max_iterations");
  zoomSpeed = parser.findFloat("zoom_speed");
  iterationStep = parser.findInt("iteration_step");
  //Display
  bool fullscreen = parser.findBool("fullscreen");
  windowHeight = parser.findInt("screen_height");
  windowWidth = parser.findInt("screen_width");
  screenHeight = scale * windowHeight;
  screenWidth = scale * windowWidth;

  //Initialize GLFW
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  //glFrameBuffer

  //Assure that the area on the XY system is proportional to the resolution and load the default view
  defaultView();
  //Create a window
  GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "mandel", fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr);

  if (!window) {
    std::cout << "Couldn't create a window" << std::endl;
    glfwTerminate();
    return -1;
  }
  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
  glfwSetWindowMaximizeCallback(window, window_maximize_callback);
  glfwSetKeyCallback(window, key_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetCursorPosCallback(window, cursor_position_callback);

  //Load GLAD function pointers
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Couldn't load GLAD" << std::endl;
    glfwTerminate();
    return -1;
  }

  //Configure global opengl state
  glEnable(GL_DEPTH_TEST);

  Shader shader("vert.glsl", "frag.glsl"), screenShader("vertScreen.glsl", "fragScreen.glsl");

  //Draw on entire window area
  //The window is comprised of 2 right triangles
  GLfloat vertices[] = {
      //positions  //texCoords
      -1,  1,       0, 1,
      -1, -1,       0, 0, 
       1, -1,       1, 0,
       1,  1,       1, 1
  };
  GLuint indices[] = {
      0, 1, 2,
      0, 3, 2
  };

  //Create & setup VAO, VBO, EBO
  unsigned int VAO, VBO, EBO;
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);
  glGenBuffers(1, &EBO);
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

  //Link vertex attributes - essentially tell OpenGL
  //how to interpret vertex data
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

  screenShader.use();
  screenShader.setUniform1i("screenTexture", 0);

  //Create & setup FBO
  //The additional FBO is created (besides the default one) to allow for 
  //rendering at a higher resolution (supersampling)
  unsigned int FBO;
  glGenFramebuffers(1, &FBO);


  //Setting up the texture attachment for the framebuffer (mainly used for SSAA or rendering to a resolution
  //which is independent from the window resolution (offscreen rendering)
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // Prevents edge bleeding
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Prevents edge bleeding

  //Attach texture to FBO
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, FBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    std::cout << "Framebuffer is not complete" << std::endl;
   glBindFramebuffer(GL_FRAMEBUFFER, 0);

  //Main render loop
  while (!glfwWindowShouldClose(window)) {
    //std::cout << screenWidth << " " << screenHeight << std::endl;
    //std::cout << windowWidth  << " " << windowHeight << std::endl;

    //Display info using the window's title
    std::string maxIterationsStr = std::to_string(maxIterations);
    std::string reStr = std::to_string((minX + maxX) / 2);
    std::string imStr = std::to_string((minY + maxY) / 2);
    std::string title = "mandel by iancu | " + maxIterationsStr + " iterations | center re: " + reStr + " im: " + imStr;
    glfwSetWindowTitle(window, title.c_str());

    //Make sure to render to the offscreen framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //Activate the shader program (used for generating the mandelbrot)
    shader.use();

    //"Uniforms" are shared variables between CPU/GPU
    //Update uniforms
    shader.setUniform2f("lowerLeft", minX, minY);
    shader.setUniform2f("upperRight", maxX, maxY);
    shader.setUniform1i("maxIterations", maxIterations);
    shader.setUniform2f("viewportDimensions", screenWidth, screenHeight);
    shader.setUniform1i("numColors", numColors);
    shader.setUniform4fv("palette", paletteSz, (GLfloat*)palette);
    shader.setUniform1f("paletteOffset", paletteOffset);
    shader.setUniform1i("applySmooth", applySmooth);
    paletteOffset += paletteOffsetStep;

    //Render mandelbrot to FBO
    //std::cout << screenWidth << " " << screenHeight << "\n";
    glViewport(0, 0, screenWidth, screenHeight);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    //Bind back default FBO
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    //Render mandelbrot texture to screen
    //Also, switch to the screenShader (which chooses the interpolated color from the texture)
    screenShader.use();
    screenShader.setUniform2f("viewportDimensions", windowWidth, windowHeight);

    glBindVertexArray(VAO);
    glBindTexture(GL_TEXTURE_2D, texture);
    glViewport(0, 0, windowWidth, windowHeight);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    //Display image
    glfwSwapBuffers(window);

    if (lastScroll != 0 && glfwGetTime() - lastScroll >= 0.7) {
      lastScroll = 0;
      screenWidth = windowWidth * scale, screenHeight = windowHeight * scale;
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
    //Listen for keyboard/mouse interaction
    glfwPollEvents();

    //Zoom in (benchmark)
    if (shouldZoomIn)
      zoomIn();
  }

  //Unbind
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  //Exit program
  glfwTerminate();
  return 0;
}
