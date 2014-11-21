#include "ModelInfo.h"

ModelInfo::ModelInfo(const string& name,mesh* v_mesh)
:name(name),mesh_ptr(v_mesh),face_size(mesh_ptr->fTotal)
{
    btm_tri.reserve(face_size);
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