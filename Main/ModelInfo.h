#include "mesh.h"
#include "glut.h"
#include "omp.h"
#include <string>
#include <iostream>
#include "pthread.h"
#include "sched.h"
#include "semaphore.h"
#include "cstdlib"
#include "scene.h"
#include <cmath>
#define SHADOW_RATE 10
using namespace std;




struct TriangleInfo{
    GLfloat coor[3][3];
    GLfloat dot;
};

struct FaceInfo{
    GLfloat* tri[3];
    int draw;
};

bool isVertexNear(const GLfloat* a,const GLfloat* b);
bool isTriangleNear(const GLfloat*const* a ,const GLfloat*const* b);

struct ModelInfo{
    ModelInfo(const string& name,mesh* v_mesh);
    void GenerateBottomTriangle(GLfloat* light_pos,int rank);
    void CollisionWithMesh(const ModelInfo& other);
    void GoLeft(GLfloat val);
    void GoRight(GLfloat val);
    void GoUp(GLfloat val);
    void GoDown(GLfloat val);
    void ApplySpeed();
    vector<TriangleInfo> btm_tri;
    vector<FaceInfo> face_draw;
    vector<mesh::Vec3> vertexList;
    string name;
    mesh* mesh_ptr;
    GLfloat mainSpeed[3];
    int face_size;
    int collision_face_check_interval;
};