#include "ModelInfo.h"
bool isVertexNear(const GLfloat* a,const GLfloat* b){
    int fit = 0;
    for(int i=0;i<3;i++){
        if(abs(a[i]-b[i])<=0.3f){
            ++fit;
        }
    }
    return fit == 3;
}
bool isTriangleNear(const GLfloat*const* a ,const GLfloat*const* b){
    int fit = 0;
    for(int i=0;i<3;i++){
        if(isVertexNear(a[i],b[i])){
            ++fit;
        }
    }
    return fit > 0;
}

ModelInfo::ModelInfo(const string& name,mesh* v_mesh)
:name(name),
mesh_ptr(v_mesh),
face_size(mesh_ptr->fTotal),
collision_face_check_interval(35)
{
    // initialize random speed
    for(int i=0;i<3;i++){
        mainSpeed[i] = (GLfloat)(rand()%32400)/32400.0f;
    }
    printf("Initial speed for %s is (%f,%f,%f)\n",name.c_str(),mainSpeed[0],mainSpeed[1],mainSpeed[2]);
    // vector size reserve
    btm_tri.reserve(face_size);
    face_draw.reserve(face_size);
    vertexList = mesh_ptr->vList;
    // collision related
    if(collision_face_check_interval <=0){
        collision_face_check_interval = 1;
    }
    for(int i=0;i<face_size;++i){
        face_draw[i].draw = 1;
        for(int j=0;j<3;j++){
            face_draw[i].tri[j] = vertexList[mesh_ptr->faceList[i][j].v].ptr;
            //auto otherPos = face_draw[i].tri[j];
            //printf("my pos is %.2f,%.2f,%.2f\n",otherPos[0],otherPos[1],otherPos[2]);
        }
    }
}

void ModelInfo::CollisionWithMesh(const ModelInfo& other){
    for(int x=0;x<face_size;++x){
        auto& self_face = face_draw[x];
        //face_draw[i] = 1;
        // check each target triangle
        for(int i=0;i<other.face_size;i+=other.collision_face_check_interval){
            auto& other_face = other.face_draw[i];
            if(isTriangleNear(self_face.tri,other_face.tri)){
                self_face.draw = 0;
                //cout << "HIde!" << endl;
            }
            else{
                //self_face.draw = 1;
            }
        }
        
    }
}

void ModelInfo::resetCollision(){
    for(int x=0;x<face_size;++x){
        auto& self_face = face_draw[x];
        self_face.draw = 1;
    }
}

void ModelInfo::GoLeft(GLfloat val){
    for(auto& v:vertexList){
        v[0] -= val;
    }
}
void ModelInfo::GoRight(GLfloat val){
    for(auto& v:vertexList){
        v[0] += val;
    }
}
void ModelInfo::GoUp(GLfloat val){
    for(auto& v:vertexList){
        v[2] += val;
    }
}
void ModelInfo::GoDown(GLfloat val){
    for(auto& v:vertexList){
        v[2] -= val;
    }
}

void  ModelInfo::ApplySpeed(){
    bool speedChanged[3] = {false,false,false};
    GLfloat nextSpeed[3] = {0.0f,0.0f,0.0f};
    for(auto& v:vertexList){
        for(int i=0;i<3;i++){
            v[i] += mainSpeed[i];
            if(!speedChanged[i]&&(v[i]>30||v[i]<-10)){
                nextSpeed[i] = mainSpeed[i]*-1.0f;
                speedChanged[i] = true;
            }
        }
    }
    for(int i=0;i<3;i++){
        if(speedChanged[i]){
            mainSpeed[i] = nextSpeed[i];
        }
    }
    glutPostRedisplay();
}

void ModelInfo::GenerateBottomTriangle(GLfloat* light_pos,int rank){
    //cout << "Generate execute by " << rank << endl;
    GLfloat LtoV[3]; // light to vertex
    GLfloat AtoL[3]; // vertex A to light
	GLfloat AtoB[3]; // vertex A to vertex B
	GLfloat AtoC[3]; // vertex A to vertex C
    GLfloat ABcrossAC[3];
    for(int i=0;i<face_size;++i)
		{
            TriangleInfo& tri = btm_tri[i];
			for (size_t j=0;j<3;++j)
			{	
                for(size_t k=0;k<3;++k) LtoV[k] = mesh_ptr->vList[mesh_ptr->faceList[i][j].v].ptr[k] - light_pos[k];
                for(size_t k=0;k<3;++k)	tri.coor[j][k] = mesh_ptr->vList[mesh_ptr->faceList[i][j].v].ptr[k] + LtoV[k]*SHADOW_RATE;	
			}

			// judge whether the polygon faces the light (if light-faceing->draw shadow polygons)
			
			for(size_t k=0; k<3;++k){
                AtoL[k] = light_pos[k] - mesh_ptr->vList[mesh_ptr->faceList[i][0].v].ptr[k];
				AtoB[k] = mesh_ptr->vList[mesh_ptr->faceList[i][1].v].ptr[k] - mesh_ptr->vList[mesh_ptr->faceList[i][0].v].ptr[k];
				AtoC[k] = mesh_ptr->vList[mesh_ptr->faceList[i][2].v].ptr[k] - mesh_ptr->vList[mesh_ptr->faceList[i][0].v].ptr[k];
			}

			
			ABcrossAC[0] = AtoB[1]*AtoC[2] - AtoB[2]*AtoC[1];
			ABcrossAC[1] = AtoB[2]*AtoC[0] - AtoB[0]*AtoC[2];
			ABcrossAC[2] = AtoB[0]*AtoC[1] - AtoB[1]*AtoC[0];

			

			tri.dot = AtoL[0]*ABcrossAC[0] + AtoL[1]*ABcrossAC[1] + AtoL[2]*ABcrossAC[2];
        }
            
    //cout << "Triangle OK for " << rank<< endl;
}