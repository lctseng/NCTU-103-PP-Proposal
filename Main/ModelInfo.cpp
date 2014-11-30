#include "ModelInfo.h"

ModelInfo::~ModelInfo(){
    for(int i=0;i<WORLD_HEIGHT;i++){
        delete[] vertexList[i];
        delete[] nextList[i];
    }
    delete[] vertexList;
    delete[] nextList;
    delete loads;
}
ModelInfo::ModelInfo(const string& name,mesh* v_mesh,int thread_count)
:name(name),
mesh_ptr(v_mesh),
face_size(mesh_ptr->fTotal)
{
    // generate workload
    loads = new WorkLoad[thread_count];
    int part = WORLD_HEIGHT / thread_count;
    int remain = WORLD_HEIGHT - part*thread_count;
    int start = 0;
    for(int i=0;i<thread_count;i++){
        loads[i].start = start;
        start += part;
        loads[i].end = start;
    }
    loads[thread_count-1].end += remain;

    for(int i=0;i<thread_count;i++){
        printf("Loads for %d is %d to %d\n",i,loads[i].start,loads[i].end);
    }
    // initialize random speed
    for(int i=0;i<3;i++){
        mainSpeed[i] = (GLfloat)(rand()%32400)/32400.0f;
    }
    printf("Initial speed for %s is (%f,%f,%f)\n",name.c_str(),mainSpeed[0],mainSpeed[1],mainSpeed[2]);
    // vector size reserve
    GenerateGameOfLifeBase();
    // collision related
}

void ModelInfo::GenerateGameOfLifeBase(){
    vertexList = new mesh::Vec3*[WORLD_HEIGHT];
    nextList = new mesh::Vec3*[WORLD_HEIGHT];
    for(int i=0;i<WORLD_HEIGHT;i++){
        vertexList[i] = new mesh::Vec3[WORLD_WIDTH];
        nextList[i] = new mesh::Vec3[WORLD_WIDTH];
    }
    for(int i=0;i<WORLD_HEIGHT;i++){
        for(int j=0;j<WORLD_WIDTH;j++){
            vertexList[i][j].ptr[0] = i*0.1;
            vertexList[i][j].ptr[1] = j*0.1;
            vertexList[i][j].ptr[2] = 10.0f;
            vertexList[i][j].dead = rand()%2!=0;
        }
    }
    for(int i=0;i<5;i++){
        GenerateNextGeneration();
        MigrateToNext();
    }
}

inline bool checkRange(int x,int y){
    return x>=0&&x<ModelInfo::WORLD_HEIGHT&&y>=0&&y<ModelInfo::WORLD_WIDTH;
}

int ModelInfo::Direct1(int i,int j){
    int x = i-1, y = j+1;
    if(!checkRange(x,y)||vertexList[x][y].dead){
        return 0;
    }
    else{
        return 1;
    }
}
int ModelInfo::Direct2(int i,int j){
    int x = i,y = j+1;
    if(!checkRange(x,y)||vertexList[x][y].dead){
        return 0;
    }
    else{
        return 1;
    }
}
int ModelInfo::Direct3(int i,int j){
    int x = i+1,y = j+1;
    if(!checkRange(x,y)||vertexList[x][y].dead){
        return 0;
    }
    else{
        return 1;
    }
}
int ModelInfo::Direct4(int i,int j){
    int x = i-1, y = j;
    if(!checkRange(x,y)||vertexList[x][y].dead){
        return 0;
    }
    else{
        return 1;
    }
}
int ModelInfo::Direct6(int i,int j){
    int x = i+1, y = j;
    if(!checkRange(x,y)||vertexList[x][y].dead){
        return 0;
    }
    else{
        return 1;
    }
}
int ModelInfo::Direct7(int i,int j){
    int x = i-1, y = j-1;
    if(!checkRange(x,y)||vertexList[x][y].dead){
        return 0;
    }
    else{
        return 1;
    }
}
int ModelInfo::Direct8(int i,int j){
    int x = i,y = j-1;
    if(!checkRange(x,y)||vertexList[x][y].dead){
        return 0;
    }
    else{
        return 1;
    }
}
int ModelInfo::Direct9(int i,int j){
    int x = i+1,y = j-1;
    if(!checkRange(x,y)||vertexList[x][y].dead){
        return 0;
    }
    else{
        return 1;
    }
}

int ModelInfo::Surround(int i,int j){
    return Direct2(i,j)+Direct4(i,j)+Direct6(i,j)+Direct8(i,j)+Direct1(i,j)+Direct3(i,j)+Direct7(i,j)+Direct9(i,j);
}

void ModelInfo::MigrateToNext(){
    for(int i=0;i<WORLD_HEIGHT;i++){
        for(int j=0;j<WORLD_WIDTH;j++){
            vertexList[i][j].dead = nextList[i][j].dead;
        }
    }
}

void ModelInfo::GenerateNextGeneration(){
    for(int i=0;i<WORLD_HEIGHT;i++){
        for(int j=0;j<WORLD_WIDTH;j++){
            auto& node = vertexList[i][j];
            
            if(node.dead){
                int sur = Surround(i,j);
                if(sur==3){
                    nextList[i][j].dead = false;
                }
            }
            else{
                int sur = Surround(i,j);
                if(sur<2){
                    nextList[i][j].dead = true;
                }
                else if(sur>3){
                    nextList[i][j].dead = true;
                }
            }
            
        }
    }
}


void ModelInfo::ThreadMigrate(int rank){
    const WorkLoad& load = loads[rank];
    for(int i=load.start;i<load.end;i++){
        for(int j=0;j<WORLD_WIDTH;j++){
            auto& node = vertexList[i][j];
            
            if(node.dead){
                int sur = Surround(i,j);
                if(sur==3){
                    nextList[i][j].dead = false;
                }
            }
            else{
                int sur = Surround(i,j);
                if(sur<2){
                    nextList[i][j].dead = true;
                }
                else if(sur>3){
                    nextList[i][j].dead = true;
                }
            }
            
        }
    }
}

void ModelInfo::ThreadGeneration(int rank){
    const WorkLoad& load = loads[rank];
    for(int i=load.start;i<load.end;i++){
        for(int j=0;j<WORLD_WIDTH;j++){
            vertexList[i][j].dead = nextList[i][j].dead;
        }
    }
}