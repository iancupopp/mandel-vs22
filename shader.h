#include <glad/glad.h>
#include <streambuf>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <GLFW/glfw3.h>

class Shader {
private:
  unsigned int id;
  std::unordered_map<std::string, unsigned int> uniformLocation;
public:
  Shader(const char* vertFile, const char* fragFile) {
    //Create & compile the vertex shader
    unsigned int vertexShader;

    std::string vertexSource = readSource(vertFile);
    const char* vertexSourceChr = vertexSource.c_str();
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSourceChr, nullptr);
    glCompileShader(vertexShader);

    //Error checking
    char shaderInfoLog[512];
    if (!checkShaderCompilation(vertexShader, shaderInfoLog)) {
      std::cout << "Compiling the vertex shader encountered an error" << std::endl;
      std::cout << shaderInfoLog << std::endl;
    }

    //Create & assign the fragment shader
    unsigned int fragmentShader;
    std::string fragmentSource = readSource(fragFile);
    const char* fragmentSourceChr = fragmentSource.c_str();
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSourceChr, nullptr);
    glCompileShader(fragmentShader);

    //Error checking
    char fragmentInfoLog[512];
    if (!checkShaderCompilation(fragmentShader, fragmentInfoLog)) {
      std::cout << "Compiling the fragment shader encountered an error" << std::endl;
      std::cout << fragmentInfoLog << std::endl;
    }

    //Create & assign the shader program
    //aka link shaders
    id = glCreateProgram();
    glAttachShader(id, vertexShader);
    glAttachShader(id, fragmentShader);
    glLinkProgram(id);

    //Error checking
    char programInfoLog[512];
    if (!checkProgramCompilation(programInfoLog)) {
      std::cout << "Couldn't create shader program" << std::endl;
      std::cout << programInfoLog << std::endl;
    }

    //Delete shader objects - they're already linked
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
  }
  void use() {
    glUseProgram(id);
  }
  void cacheUniform(const char* str) {
    //Cache uniform locations for speed
    uniformLocation[str] = glGetUniformLocation(id, str);
  }
  void setUniform1i(const char* str, int val) {
    if (uniformLocation.find(str) == uniformLocation.end())
      cacheUniform(str);
    glUniform1i(uniformLocation[str], val);
  }
  void setUniform1f(const char* str, float val) {
    if (uniformLocation.find(str) == uniformLocation.end())
      cacheUniform(str);
    glUniform1f(uniformLocation[str], val);
  }
  void setUniform2f(const char* str, float val1, float val2) {
    if (uniformLocation.find(str) == uniformLocation.end())
      cacheUniform(str);
    glUniform2f(uniformLocation[str], val1, val2);
  }
  void setUniform4fv(const char* str, int sz, GLfloat* arr) {
    if (uniformLocation.find(str) == uniformLocation.end())
      cacheUniform(str);
    glUniform4fv(uniformLocation[str], sz, arr);
  }
  std::string readSource(const char* filename) {
    std::ifstream source(filename);
    std::string str((std::istreambuf_iterator<char>(source)),
      std::istreambuf_iterator<char>());
    return str;
  }
  bool checkShaderCompilation(const unsigned int shader, char infoLog[512]) {
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success) return true;
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    return false;
  }
  bool checkProgramCompilation(char infoLog[512]) {
    int success;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (success) return true;
    glGetProgramInfoLog(id, 512, nullptr, infoLog);
    return false;
  }

};