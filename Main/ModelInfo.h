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
    ~ModelInfo();
    void GenerateGameOfLifeBase();
    void GenerateNextGeneration();
    void MigrateToNext();
    vector<TriangleInfo> btm_tri;
    vector<FaceInfo> face_draw;
    mesh::Vec3** vertexList;
    mesh::Vec3** nextList;
    string name;
    mesh* mesh_ptr;
    GLfloat mainSpeed[3];
    int face_size;
    int vertex_size;
    static const int WORLD_HEIGHT = 2000;
    static const int WORLD_WIDTH = 2000;
    //Direction
    int Direct1(int i,int j);
    int Direct2(int i,int j);
    int Direct3(int i,int j);
    int Direct4(int i,int j);
    int Direct6(int i,int j);
    int Direct7(int i,int j);
    int Direct8(int i,int j);
    int Direct9(int i,int j);
    int Surround(int i,int j);
};