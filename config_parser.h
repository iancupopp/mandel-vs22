#pragma once

#include <fstream>
#include <map>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

struct Point {
  GLfloat pos;
  GLfloat r, g, b;
  bool operator<(const Point& p) const {
    return pos < p.pos;
  }
};

typedef std::map<std::string, std::string> Map;

class CfgParser {
private:
  Map map;
public:
  void parseFile(const char* filename) {
    std::ifstream cin(filename);
    std::string s;
    while (std::getline(cin, s)) {
      std::stringstream ss(s);
      std::string a, b, c;
      while (ss >> c) {
        //ignore comments
        if (c.find('#') != std::string::npos)
          break;
        if (b == "=")
          map[a] = c;
        a = b;
        b = c;
      }
    }
    cin.close();
  }
  bool findBool(const std::string& s) {
    return map[s] == "true" || map[s] == "1";
  }
  int findInt(const std::string& s) {
    return std::stoi(map[s]);
  }
  float findFloat(const std::string& s) {
    return std::stof(map[s]);
  }
  //Used for retrieving the position and RGB value of the gradient points
  void parse4f(const std::string& s, float* arr) {
    std::stringstream ss(s);
    for (int i = 0; i < 4; ++i) {
      std::string number;
      std::getline(ss, number, ',');
      arr[i] = std::stof(number);
    }
  }
  //Retrieve gradient points
  void getPalette(Point* palette, int& paletteSz) {
    paletteSz = 0;
    for (const auto &p: map)
      if (p.first.find("gradient_point") != std::string::npos) {
        parse4f(p.second, (float *) &palette[paletteSz]);
        palette[paletteSz].pos /= 100.0;
        palette[paletteSz].r /= 255.0;
        palette[paletteSz].g /= 255.0;
        palette[paletteSz].b /= 255.0;
        ++paletteSz;
      }
    palette[paletteSz++] = {1.0, palette[0].r, palette[0].g, palette[0].b};
    std::sort(palette, palette + paletteSz);
  }
};