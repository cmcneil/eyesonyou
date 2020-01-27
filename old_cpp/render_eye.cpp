#include <iostream>
#include <map>
#include <string>
#include <vector>


#include <GL/glew.h>
#include <GL/glu.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
//#include <GL/glew.h>
//#include <GL/glu.h>

#include "objloader.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

void PrintOpenGLErrors(char const * const Function, char const * const File, int const Line)
{
	bool Succeeded = true;

	GLenum Error = glGetError();
	if (Error != GL_NO_ERROR)
	{
		char const * ErrorString = (char const *) gluErrorString(Error);
		if (ErrorString)
			std::cerr << ("OpenGL Error in %s at line %d calling function %s: '%s'", File, Line, Function, ErrorString) << std::endl;
		else
			std::cerr << ("OpenGL Error in %s at line %d calling function %s: '%d 0x%X'", File, Line, Function, Error, Error) << std::endl;
	}
}

#ifdef _DEBUG
#define CheckedGLCall(x) do { PrintOpenGLErrors(">>BEFORE<< "#x, __FILE__, __LINE__); (x); PrintOpenGLErrors(#x, __FILE__, __LINE__); } while (0)
#define CheckedGLResult(x) (x); PrintOpenGLErrors(#x, __FILE__, __LINE__);
#define CheckExistingErrors(x) PrintOpenGLErrors(">>BEFORE<< "#x, __FILE__, __LINE__);
#else
#define CheckedGLCall(x) (x)
#define CheckedGLResult(x) (x)
#define CheckExistingErrors(x)
#endif


void PrintShaderInfoLog(GLint const Shader)
{
	int InfoLogLength = 0;
	int CharsWritten = 0;

	glGetShaderiv(Shader, GL_INFO_LOG_LENGTH, & InfoLogLength);

	if (InfoLogLength > 0)
	{
		GLchar * InfoLog = new GLchar[InfoLogLength];
		glGetShaderInfoLog(Shader, InfoLogLength, & CharsWritten, InfoLog);
		std::cout << "Shader Info Log:" << std::endl << InfoLog << std::endl;
		delete [] InfoLog;
	}
}

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

class timerutil {
 public:
  // C timer
  // using namespace std;
  typedef clock_t time_t;

  void start() { t_[0] = clock(); }
  void end() { t_[1] = clock(); }

  time_t sec() { return (time_t)((t_[1] - t_[0]) / CLOCKS_PER_SEC); }
  time_t msec() { return (time_t)((t_[1] - t_[0]) * 1000 / CLOCKS_PER_SEC); }
  time_t usec() { return (time_t)((t_[1] - t_[0]) * 1000000 / CLOCKS_PER_SEC); }
  time_t current() { return (time_t)clock(); }

 private:

  time_t t_[2];
};

typedef struct {
  GLuint vb_id;  // vertex buffer id
  int numTriangles;
  size_t material_id;
} DrawObject;

std::vector<DrawObject> gDrawObjects;

int width = 768;
int height = 768;

double prevMouseX, prevMouseY;
bool mouseLeftPressed;
bool mouseMiddlePressed;
bool mouseRightPressed;
float curr_quat[4];
float prev_quat[4];
float eye[3], lookat[3], up[3];

GLFWwindow* window;

static std::string GetBaseDir(const std::string& filepath) {
  if (filepath.find_last_of("/\\") != std::string::npos)
    return filepath.substr(0, filepath.find_last_of("/\\"));
  return "";
}

static bool FileExists(const std::string& abs_filename) {
  bool ret;
  FILE* fp = fopen(abs_filename.c_str(), "rb");
  if (fp) {
    ret = true;
    fclose(fp);
  } else {
    ret = false;
  }

  return ret;
}

static void CheckErrors(std::string desc) {
  GLenum e = glGetError();
  if (e != GL_NO_ERROR) {
    fprintf(stderr, "OpenGL error in \"%s\": %d (%d)\n", desc.c_str(), e, e);
    exit(20);
  }
}

static void CalcNormal(float N[3], float v0[3], float v1[3], float v2[3]) {
  float v10[3];
  v10[0] = v1[0] - v0[0];
  v10[1] = v1[1] - v0[1];
  v10[2] = v1[2] - v0[2];

  float v20[3];
  v20[0] = v2[0] - v0[0];
  v20[1] = v2[1] - v0[1];
  v20[2] = v2[2] - v0[2];

  N[0] = v20[1] * v10[2] - v20[2] * v10[1];
  N[1] = v20[2] * v10[0] - v20[0] * v10[2];
  N[2] = v20[0] * v10[1] - v20[1] * v10[0];

  float len2 = N[0] * N[0] + N[1] * N[1] + N[2] * N[2];
  if (len2 > 0.0f) {
    float len = sqrtf(len2);

    N[0] /= len;
    N[1] /= len;
    N[2] /= len;
  }
}

namespace  // Local utility functions
{
struct vec3 {
  float v[3];
  vec3() {
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
  }
};

void normalizeVector(vec3 &v) {
  float len2 = v.v[0] * v.v[0] + v.v[1] * v.v[1] + v.v[2] * v.v[2];
  if (len2 > 0.0f) {
    float len = sqrtf(len2);

    v.v[0] /= len;
    v.v[1] /= len;
    v.v[2] /= len;
  }
}

bool hasSmoothingGroup(const tinyobj::shape_t& shape)
{
  for (size_t i = 0; i < shape.mesh.smoothing_group_ids.size(); i++) {
    if (shape.mesh.smoothing_group_ids[i] > 0) {
      return true;
    }
  }
  return false;
}

void computeSmoothingNormals(const tinyobj::attrib_t& attrib, const tinyobj::shape_t& shape,
                             std::map<int, vec3>& smoothVertexNormals) {
  smoothVertexNormals.clear();
  std::map<int, vec3>::iterator iter;

  for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
    // Get the three indexes of the face (all faces are triangular)
    tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
    tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
    tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

    // Get the three vertex indexes and coordinates
    int vi[3];      // indexes
    float v[3][3];  // coordinates

    for (int k = 0; k < 3; k++) {
      vi[0] = idx0.vertex_index;
      vi[1] = idx1.vertex_index;
      vi[2] = idx2.vertex_index;
      assert(vi[0] >= 0);
      assert(vi[1] >= 0);
      assert(vi[2] >= 0);

      v[0][k] = attrib.vertices[3 * vi[0] + k];
      v[1][k] = attrib.vertices[3 * vi[1] + k];
      v[2][k] = attrib.vertices[3 * vi[2] + k];
    }

    // Compute the normal of the face
    float normal[3];
    CalcNormal(normal, v[0], v[1], v[2]);

    // Add the normal to the three vertexes
    for (size_t i = 0; i < 3; ++i) {
      iter = smoothVertexNormals.find(vi[i]);
      if (iter != smoothVertexNormals.end()) {
        // add
        iter->second.v[0] += normal[0];
        iter->second.v[1] += normal[1];
        iter->second.v[2] += normal[2];
      } else {
        smoothVertexNormals[vi[i]].v[0] = normal[0];
        smoothVertexNormals[vi[i]].v[1] = normal[1];
        smoothVertexNormals[vi[i]].v[2] = normal[2];
      }
    }

  }  // f

  // Normalize the normals, that is, make them unit vectors
  for (iter = smoothVertexNormals.begin(); iter != smoothVertexNormals.end();
       iter++) {
    normalizeVector(iter->second);
  }

}  // computeSmoothingNormals
}  // namespace

static bool LoadObjAndConvert(float bmin[3], float bmax[3],
                              std::vector<DrawObject>* drawObjects,
                              std::vector<tinyobj::material_t>& materials,
                              std::map<std::string, GLuint>& textures,
                              const char* filename) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;

  timerutil tm;

  tm.start();

  std::string base_dir = GetBaseDir(filename);
  if (base_dir.empty()) {
    base_dir = ".";
  }

  base_dir += "/";

  std::string warn;
  std::string err;
  bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename,
                              base_dir.c_str());
  if (!warn.empty()) {
    std::cout << "WARN: " << warn << std::endl;
  }
  if (!err.empty()) {
    std::cerr << err << std::endl;
  }

  tm.end();

  if (!ret) {
    std::cerr << "Failed to load " << filename << std::endl;
    return false;
  }

  printf("Parsing time: %d [ms]\n", (int)tm.msec());

  printf("# of vertices  = %d\n", (int)(attrib.vertices.size()) / 3);
  printf("# of normals   = %d\n", (int)(attrib.normals.size()) / 3);
  printf("# of texcoords = %d\n", (int)(attrib.texcoords.size()) / 2);
  printf("# of materials = %d\n", (int)materials.size());
  printf("# of shapes    = %d\n", (int)shapes.size());

  // Append `default` material
  materials.push_back(tinyobj::material_t());

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%d].diffuse_texname = %s\n", int(i),
           materials[i].diffuse_texname.c_str());
  }

  // Load diffuse textures
  {
    for (size_t m = 0; m < materials.size(); m++) {
      tinyobj::material_t* mp = &materials[m];

      if (mp->diffuse_texname.length() > 0) {
        // Only load the texture if it is not already loaded
        if (textures.find(mp->diffuse_texname) == textures.end()) {
          GLuint texture_id;
          int w, h;
          int comp;

          std::string texture_filename = mp->diffuse_texname;
          if (!FileExists(texture_filename)) {
            // Append base dir.
            texture_filename = base_dir + mp->diffuse_texname;
            if (!FileExists(texture_filename)) {
              std::cerr << "Unable to find file: " << mp->diffuse_texname
                        << std::endl;
              exit(1);
            }
          }

          unsigned char* image =
              stbi_load(texture_filename.c_str(), &w, &h, &comp, STBI_default);
          if (!image) {
            std::cerr << "Unable to load texture: " << texture_filename
                      << std::endl;
            exit(1);
          }
          std::cout << "Loaded texture: " << texture_filename << ", w = " << w
                    << ", h = " << h << ", comp = " << comp << std::endl;

          glGenTextures(1, &texture_id);
          glBindTexture(GL_TEXTURE_2D, texture_id);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
          glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
          if (comp == 3) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, image);
          } else if (comp == 4) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, image);
          } else {
            assert(0);  // TODO
          }
          glBindTexture(GL_TEXTURE_2D, 0);
          stbi_image_free(image);
          textures.insert(std::make_pair(mp->diffuse_texname, texture_id));
        }
      }
    }
  }

  bmin[0] = bmin[1] = bmin[2] = std::numeric_limits<float>::max();
  bmax[0] = bmax[1] = bmax[2] = -std::numeric_limits<float>::max();

  {
    for (size_t s = 0; s < shapes.size(); s++) {
      DrawObject o;
      std::vector<float> buffer;  // pos(3float), normal(3float), color(3float)
      vector<GLfloat> mesh_vertex;
      vector<GLfloat> mesh_normals;
      vector<GLfloat> mesh_colors;
      vector<GLfloat> mesh_textCoords;
      vector<GLfloat> mesh_indices;
       
      for (long i = 0; i < shapes[s].mesh_indices[i].size(); i++) {
		mesh_indices.push_back(shapes[s].mesh_indices[i].vertex_index);
	  }

      // Check for smoothing group and compute smoothing normals
      std::map<int, vec3> smoothVertexNormals;
      if (hasSmoothingGroup(shapes[s]) > 0) {
        std::cout << "Compute smoothingNormal for shape [" << s << "]" << std::endl;
        computeSmoothingNormals(attrib, shapes[s], smoothVertexNormals);
      }

      for (size_t f = 0; f < shapes[s].mesh.indices.size() / 3; f++) {
        tinyobj::index_t idx0 = shapes[s].mesh.indices[3 * f + 0];
        tinyobj::index_t idx1 = shapes[s].mesh.indices[3 * f + 1];
        tinyobj::index_t idx2 = shapes[s].mesh.indices[3 * f + 2];

        int current_material_id = shapes[s].mesh.material_ids[f];

        if ((current_material_id < 0) ||
            (current_material_id >= static_cast<int>(materials.size()))) {
          // Invaid material ID. Use default material.
          current_material_id =
              materials.size() -
              1;  // Default material is added to the last item in `materials`.
        }
        // if (current_material_id >= materials.size()) {
        //    std::cerr << "Invalid material index: " << current_material_id <<
        //    std::endl;
        //}
        //
        float diffuse[3];
        for (size_t i = 0; i < 3; i++) {
          diffuse[i] = materials[current_material_id].diffuse[i];
        }
        float tc[3][2];
        if (attrib.texcoords.size() > 0) {
          if ((idx0.texcoord_index < 0) || (idx1.texcoord_index < 0) ||
              (idx2.texcoord_index < 0)) {
            // face does not contain valid uv index.
            tc[0][0] = 0.0f;
            tc[0][1] = 0.0f;
            tc[1][0] = 0.0f;
            tc[1][1] = 0.0f;
            tc[2][0] = 0.0f;
            tc[2][1] = 0.0f;
          } else {
            assert(attrib.texcoords.size() >
                   size_t(2 * idx0.texcoord_index + 1));
            assert(attrib.texcoords.size() >
                   size_t(2 * idx1.texcoord_index + 1));
            assert(attrib.texcoords.size() >
                   size_t(2 * idx2.texcoord_index + 1));

            // Flip Y coord.
            tc[0][0] = attrib.texcoords[2 * idx0.texcoord_index];
            tc[0][1] = 1.0f - attrib.texcoords[2 * idx0.texcoord_index + 1];
            tc[1][0] = attrib.texcoords[2 * idx1.texcoord_index];
            tc[1][1] = 1.0f - attrib.texcoords[2 * idx1.texcoord_index + 1];
            tc[2][0] = attrib.texcoords[2 * idx2.texcoord_index];
            tc[2][1] = 1.0f - attrib.texcoords[2 * idx2.texcoord_index + 1];
          }
        } else {
          tc[0][0] = 0.0f;
          tc[0][1] = 0.0f;
          tc[1][0] = 0.0f;
          tc[1][1] = 0.0f;
          tc[2][0] = 0.0f;
          tc[2][1] = 0.0f;
        }

        float v[3][3];
        for (int k = 0; k < 3; k++) {
          int f0 = idx0.vertex_index;
          int f1 = idx1.vertex_index;
          int f2 = idx2.vertex_index;
          assert(f0 >= 0);
          assert(f1 >= 0);
          assert(f2 >= 0);

          v[0][k] = attrib.vertices[3 * f0 + k];
          v[1][k] = attrib.vertices[3 * f1 + k];
          v[2][k] = attrib.vertices[3 * f2 + k];
          bmin[k] = std::min(v[0][k], bmin[k]);
          bmin[k] = std::min(v[1][k], bmin[k]);
          bmin[k] = std::min(v[2][k], bmin[k]);
          bmax[k] = std::max(v[0][k], bmax[k]);
          bmax[k] = std::max(v[1][k], bmax[k]);
          bmax[k] = std::max(v[2][k], bmax[k]);
        }

        float n[3][3];
        {
          bool invalid_normal_index = false;
          if (attrib.normals.size() > 0) {
            int nf0 = idx0.normal_index;
            int nf1 = idx1.normal_index;
            int nf2 = idx2.normal_index;

            if ((nf0 < 0) || (nf1 < 0) || (nf2 < 0)) {
              // normal index is missing from this face.
              invalid_normal_index = true;
            } else {
              for (int k = 0; k < 3; k++) {
                assert(size_t(3 * nf0 + k) < attrib.normals.size());
                assert(size_t(3 * nf1 + k) < attrib.normals.size());
                assert(size_t(3 * nf2 + k) < attrib.normals.size());
                n[0][k] = attrib.normals[3 * nf0 + k];
                n[1][k] = attrib.normals[3 * nf1 + k];
                n[2][k] = attrib.normals[3 * nf2 + k];
              }
            }
          } else {
            invalid_normal_index = true;
          }

          if (invalid_normal_index && !smoothVertexNormals.empty()) {
            // Use smoothing normals
            int f0 = idx0.vertex_index;
            int f1 = idx1.vertex_index;
            int f2 = idx2.vertex_index;

            if (f0 >= 0 && f1 >= 0 && f2 >= 0) {
              n[0][0] = smoothVertexNormals[f0].v[0];
              n[0][1] = smoothVertexNormals[f0].v[1];
              n[0][2] = smoothVertexNormals[f0].v[2];

              n[1][0] = smoothVertexNormals[f1].v[0];
              n[1][1] = smoothVertexNormals[f1].v[1];
              n[1][2] = smoothVertexNormals[f1].v[2];

              n[2][0] = smoothVertexNormals[f2].v[0];
              n[2][1] = smoothVertexNormals[f2].v[1];
              n[2][2] = smoothVertexNormals[f2].v[2];

              invalid_normal_index = false;
            }
          }

          if (invalid_normal_index) {
            // compute geometric normal
            CalcNormal(n[0], v[0], v[1], v[2]);
            n[1][0] = n[0][0];
            n[1][1] = n[0][1];
            n[1][2] = n[0][2];
            n[2][0] = n[0][0];
            n[2][1] = n[0][1];
            n[2][2] = n[0][2];
          }
        }

        for (int k = 0; k < 3; k++) {
		  mesh_vertex.push_back(v[k][0]);
		  mesh_vertex.push_back(v[k][1]);
		  mesh_vertex.push_back(v[k][2]);
		  
		  mesh_normals.push_back(n[k][0]);
		  mesh_normals.push_back(n[k][1]);
		  mesh_normals.push_back(n[k][2]);

          // Combine normal and diffuse to get color.
          float normal_factor = 0.2;
          float diffuse_factor = 1 - normal_factor;
          float c[3] = {n[k][0] * normal_factor + diffuse[0] * diffuse_factor,
                        n[k][1] * normal_factor + diffuse[1] * diffuse_factor,
                        n[k][2] * normal_factor + diffuse[2] * diffuse_factor};
          float len2 = c[0] * c[0] + c[1] * c[1] + c[2] * c[2];
          if (len2 > 0.0f) {
            float len = sqrtf(len2);

            c[0] /= len;
            c[1] /= len;
            c[2] /= len;
          }
          mesh_colors.push_back(c[0] * 0.5 + 0.5);
          mesh_colors.push_back(c[1] * 0.5 + 0.5);
          mesh_colors.push_back(c[2] * 0.5 + 0.5);
          
          mesh_textCoords.push_back(tc[k][0]);
          mesh_textCoords.push_back(tc[k][1]);
        }
      }

      o.vb_id = 0;
      o.numTriangles = 0;

      // OpenGL viewer does not support texturing with per-face material.
      if (shapes[s].mesh.material_ids.size() > 0 &&
          shapes[s].mesh.material_ids.size() > s) {
        o.material_id = shapes[s].mesh.material_ids[0];  // use the material ID
                                                         // of the first face.
      } else {
        o.material_id = materials.size() - 1;  // = ID for default material.
      }
      printf("shape[%d] material_id %d\n", int(s), int(o.material_id));

      //~ if (buffer.size() > 0) {
        //~ glGenBuffers(1, &o.vb_id);
        //~ glBindBuffer(GL_ARRAY_BUFFER, o.vb_id);
        //~ glBufferData(GL_ARRAY_BUFFER, buffer.size() * sizeof(float),
                     //~ &buffer.at(0), GL_STATIC_DRAW);
        //~ o.numTriangles = buffer.size() / (3 + 3 + 3 + 2) /
                         //~ 3;  // 3:vtx, 3:normal, 3:col, 2:texcoord

        //~ printf("shape[%d] # of triangles = %d\n", static_cast<int>(s),
               //~ o.numTriangles);
      //~ }

      //~ drawObjects->push_back(o);
      GLuint positionVBO = 0;
      GLuint texcoordVBO = 0;
      GLuint normalVBO = 0;
      GLuint indicesEBO = 0;
      
      // Upload per-vertex positions
	  if (!mesh_vertex.empty())
	  {
	   glGenBuffers(1, &positionVBO);
	   glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
	   glBufferData(GL_ARRAY_BUFFER, mesh_vertex.size() * sizeof(GLfloat), &mesh_vertex[0], GL_STATIC_DRAW); // GL_DYNAMIC_DRAW ?
	   glBindBuffer(GL_ARRAY_BUFFER, 0);
	   positionVBO_array.push_back(positionVBO);
	  }
	
	  // Upload per-vertex texture coordinates
	  if (!mesh_textCoords.empty())
	  {
	   glGenBuffers(1, &texcoordVBO);
	   glBindBuffer(GL_ARRAY_BUFFER, texcoordVBO);
	   glBufferData(GL_ARRAY_BUFFER,
	          mesh_textCoords.size() * sizeof(float),
	          &mesh_textCoords[0], GL_STATIC_DRAW);
	   glBindBuffer(GL_ARRAY_BUFFER, 0);
	  }
	
	  // Upload per-vertex normals
	  if (!mesh_normals.empty())
	  {
	   glGenBuffers(1, &normalVBO);
	   glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
	   glBufferData(GL_ARRAY_BUFFER, mesh_normals.size() * sizeof(GLfloat), &mesh_normals[0], GL_STATIC_DRAW);
	   glBindBuffer(GL_ARRAY_BUFFER, 0);
	   normalVBO_array.push_back(normalVBO);
	  }
	  
	  if (!shapes[0].mesh.indices.empty()) {
		glGenBuffers(1, &indicesEBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				shapes[s].mesh.indices.size()*sizeof(unsigned int),
				shapes[s].mesh.indices.data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		
		indicesEBO_array.push_back(indicesEBO);
		indicesEBOSize_array.push_back(shapes[s].mesh.indices.size());
	  }
	  
	  GLuint meshVAO;
	  vglGenVertexArrays(1, &meshVAO);
	  meshVAO_array.push_back(meshVAO);
	  
	  if (positionVBO != 0) {
		glBindVertexArray(meshVAO);
		glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
	  }
	  
	  if (texcoordVBO != 0) {
		glBindVertexArray(meshVAO);
		glBindBuffer(GL_ARRAY_BUFFER, texcoordVBO);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(1);
		glBindVertexArray(0);
	  }
	  
	  if (normalVBO != 0) {
		glBindVertexArray(meshVAO);
		glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE,
			sizeof(float)*3, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glEnableVertexAttribArray(2);
		glBindVertexArray(0);
	  }
	  
	  if (indicesEBO != 0) {
		glBindVertexArray(meshVAO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesEBO);
		glBindVertexArray(0);
	  }
    }
  }

  printf("bmin = %f, %f, %f\n", bmin[0], bmin[1], bmin[2]);
  printf("bmax = %f, %f, %f\n", bmax[0], bmax[1], bmax[2]);

  return true;
}

//~ typedef struct {
  //~ GLuint vb_id;  // vertex buffer id
  //~ int numTriangles;
  //~ size_t material_id;
//~ } DrawObject


//~ vector<DrawObject> gDrawObjects;


int main(int argc, char** argv) {
	GLuint vertex_buffer, vertex_shader, fragment_shader, program;
	GLint mvp_location, vpos_location, vcol_location;
	
	if (!glfwInit()) {
		// init failed
		exit(EXIT_FAILURE);
	}
	//~ bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename,
	                            //~ ".");
	glfwSetErrorCallback(error_callback);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Eyes on You", NULL, NULL);
	if (!window) {
	    cout << "Window Creation Failed!" << endl;
	}
	glfwSetKeyCallback(window, key_callback);
	glfwMakeContextCurrent(window);
	
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		glfwTerminate();
		return -1;
	}
    glfwSwapInterval(1);


	//~ // Begin openGL Code
	//~ char const * VertexShaderSource = R"GLSL(
		//~ #version 150
		//~ in vec2 position;
		//~ void main()
		//~ {
			//~ gl_Position = vec4(position, 0.0, 1.0);
		//~ }
	//~ )GLSL";

	//~ char const * FragmentShaderSource = R"GLSL(
		//~ #version 150
		//~ out vec4 outColor;
		//~ void main()
		//~ {
			//~ outColor = vec4(1.0, 1.0, 1.0, 1.0);
		//~ }
	//~ )GLSL";

	//~ GLfloat const Vertices [] = {
		//~ 0.0f, 0.5f,
		//~ 0.5f, -0.5f,
		//~ -0.5f, -0.5f
	//~ };

	//~ GLuint const Elements [] = {
		//~ 0, 1, 2
	//~ };
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);

	//~ GLuint VAO;
	//~ CheckedGLCall(glGenVertexArrays(1, & VAO));
	//~ CheckedGLCall(glBindVertexArray(VAO));
	
	//~ GLuint programID = LoadShaders("transform_vertex_shader.glsl",
								   //~ "texture_fragment_shader.glsl");
	//~ GLuint matrixID = glGetUniformLocation(programID, "MVP");
	//~ GLuint Texture = loadDDS;

	//~ GLuint VBO;
	//~ CheckedGLCall(glGenBuffers(1, & VBO));
	//~ CheckedGLCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
	//~ CheckedGLCall(glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW));
	//~ CheckedGLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));

	//~ GLuint EBO;
	//~ CheckedGLCall(glGenBuffers(1, & EBO));
	//~ CheckedGLCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO));
	//~ CheckedGLCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Elements), Elements, GL_STATIC_DRAW));

	//~ GLint Compiled;
	//~ GLuint VertexShader = CheckedGLResult(glCreateShader(GL_VERTEX_SHADER));
	//~ CheckedGLCall(glShaderSource(VertexShader, 1, & VertexShaderSource, NULL));
	//~ CheckedGLCall(glCompileShader(VertexShader));
	//~ CheckedGLCall(glGetShaderiv(VertexShader, GL_COMPILE_STATUS, & Compiled));
	//~ if (! Compiled)
	//~ {
		//~ std::cerr << "Failed to compile vertex shader!" << std::endl;
		//~ PrintShaderInfoLog(VertexShader);
	//~ }

	//~ GLuint FragmentShader = CheckedGLResult(glCreateShader(GL_FRAGMENT_SHADER));
	//~ CheckedGLCall(glShaderSource(FragmentShader, 1, & FragmentShaderSource, NULL));
	//~ CheckedGLCall(glCompileShader(FragmentShader));
	//~ CheckedGLCall(glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, & Compiled));
	//~ if (! Compiled)
	//~ {
		//~ std::cerr << "Failed to compile fragment shader!" << std::endl;
		//~ PrintShaderInfoLog(FragmentShader);
	//~ }
	
	//~ GLuint ShaderProgram = CheckedGLResult(glCreateProgram());
	//~ CheckedGLCall(glAttachShader(ShaderProgram, VertexShader));
	//~ CheckedGLCall(glAttachShader(ShaderProgram, FragmentShader));
	//~ CheckedGLCall(glBindFragDataLocation(ShaderProgram, 0, "outColor"));
	//~ CheckedGLCall(glLinkProgram(ShaderProgram));
	//~ CheckedGLCall(glUseProgram(ShaderProgram));

	//~ GLint PositionAttribute = CheckedGLResult(glGetAttribLocation(ShaderProgram, "position"));
	//~ CheckedGLCall(glEnableVertexAttribArray(PositionAttribute));

	//~ CheckedGLCall(glBindBuffer(GL_ARRAY_BUFFER, VBO));
	//~ CheckedGLCall(glVertexAttribPointer(PositionAttribute, 2, GL_FLOAT, GL_FALSE, 0, 0));
	//~ CheckedGLCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
	
	
	// LOAD FILE
	std::vector<glm::vec3> out_vertices;
	std::vector<glm::vec2> out_uvs;
	std::vector<glm::vec3> out_normals;
	//~ bool ret = loadOBJ("obj/eyeball.obj", out_vertices, out_uvs, out_normals);
	
	//~ cout << out_vertices << endl;
	//~ for(int i = 0; i < out_vertices.size(); i++) {
		//~ cout << out_vertices[i].x << ", " << out_vertices[i].y << ", " << out_vertices[i].z << endl;
	//~ }
	
	// LOAD FILE OTHER WAY
	tinyobj::ObjReaderConfig loaderConfig;
	loaderConfig.triangulate = true;
	loaderConfig.vertex_color = true;
	
	tinyobj::ObjReader reader = tinyobj::ObjReader();
	string eyeballFilename = "eyeball.obj";
	bool parseRet = reader.ParseFromFile(eyeballFilename);
	cout << "Parse Is Valid: " << reader.Valid();
	for(auto mat : reader.GetMaterials()) {
	 cout << "material: " << mat.name;
	}
	
	while (!glfwWindowShouldClose(window)) {
		//~ CheckedGLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		//~ CheckedGLCall(glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0));
        glfwSwapBuffers(window);
        glfwPollEvents();
	}
	
	//~ CheckedGLCall(glDeleteProgram(ShaderProgram));
	//~ CheckedGLCall(glDeleteShader(FragmentShader));
	//~ CheckedGLCall(glDeleteShader(VertexShader));

	//~ CheckedGLCall(glDeleteBuffers(1, & EBO));
	//~ CheckedGLCall(glDeleteBuffers(1, & VBO));
	//~ CheckedGLCall(glDeleteVertexArrays(1, & VAO));
	
	//~ glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
	
}
