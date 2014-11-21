#include "mesh.h"
#include "glut.h"
#include "omp.h"
#include <string>
#include <iostream>
#include "pthread.h"
#include "sched.h"
#include "semaphore.h"
#define SHADOW_RATE 10
using namespace std;



struct TriangleInfo{
    GLfloat coor[3][3];
    GLfloat dot;
};

struct ModelInfo{
    ModelInfo(const string& name,mesh* v_mesh);
    void GenerateBottomTriangle(GLfloat* light_pos,int rank);
    vector<TriangleInfo> btm_tri;
    string name;
    mesh* mesh_ptr;
    int face_size;
};