#ifndef OBJ_LOADER_H_
#define OBJ_LOADER_H_

#include "tiny_obj_loader.h"
#include <GL/glew.h>
#include <GL/glu.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>


typedef struct {
  GLuint vb_id;  // vertex buffer id
  int numTriangles;
  size_t material_id;
} DrawObject;

static std::string GetBaseDir(const std::string& filepath);

static bool FileExists(const std::string& abs_filename);

static void CheckErrors(std::string desc);

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]);

static bool LoadObjAndConvert(float bmin[3], float bmax[3],
                              std::vector<DrawObject>* drawObjects,
                              std::vector<tinyobj::material_t>& materials,
                              std::map<std::string, GLuint>& textures,
                              const char* filename);
#endif
