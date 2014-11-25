#include <cstdio>
#include <iostream>


#include "glew.h"
#include "mesh.h"
#include "glut.h"
#include "glaux.h"
#include "wglew.h"
#include "omp.h"


#include "ModelInfo.h"
#include <vector>
#include <string>
#include <cmath>
#include <fstream>
#include "CommonDisplayRoutine.h"

#define MIPMAP
//#define MULTI_THREAD

using namespace std;

vector<mesh*> object;
views* viewing;
lights* lighting;
scenes* Scene;
vector<ModelInfo*> m_infos;
int windowSize[2];
int startX, startY;
int oldX, oldY;
GLdouble rotX, rotY;


int render_line = false;
int model_to_line = true;
int draw_shadow = false;

GLdouble left_l, right_l, back_l, front_l;
GLdouble left_x=0;
GLdouble right_x=0;
GLdouble back_x=0;
GLdouble front_x=0;
GLdouble step;

GLdouble camera_eye[3];
GLdouble camera_vat[3];
GLdouble camera_vup[3];

GLfloat light_pos[3];

double angle_x, angle_z;

GLuint v, f, p;

int thread_count;
pthread_t* threads_ptr;
sem_t* semaphores;
sem_t master_sem;
pthread_mutex_t mutex;

vector<int> job_list;

int workload;

void light(bool);
void display();
void keyboard(unsigned char, int, int);
void mouse(int, int, int, int);
void motion(int, int);

void reshape(GLsizei , GLsizei );

void* monitor(void *);


int main(int argc, char** argv)
{
#ifdef MULTI_THREAD
    workload = 1;
	thread_count = omp_get_num_procs()-1;
#endif
	viewing = new views("assignment3.view");
	lighting = new lights("assignment3.light");
	Scene = new scenes("scene1.scene");

	for(unsigned int x=0;x<(Scene->scene_model.size());x++){
		const char* model_name = (Scene->scene_model)[x].name.c_str();
		
		mesh* temp = new mesh(model_name);
		object.push_back(temp);
        m_infos.push_back(new ModelInfo(model_name,temp));
	}

#ifdef MULTI_THREAD
    pthread_mutex_init(&mutex, NULL);
    threads_ptr = new pthread_t[thread_count];
    semaphores = new sem_t[thread_count];
    for(int i=0;i<thread_count;i++){
        pthread_create(&threads_ptr[i],NULL,monitor,(void*) i);
        sem_init(&semaphores[i], 0, 0);
    }
    sem_init(&master_sem,0,0);
#endif
	camera_eye[0]=viewing->eye[0];	camera_eye[1]=viewing->eye[1];	camera_eye[2]=viewing->eye[2];
	camera_vat[0]=viewing->vat[0];	camera_vat[1]=viewing->vat[1];	camera_vat[2]=viewing->vat[2];
	camera_vup[0]=viewing->vup[0];	camera_vup[1]=viewing->vup[1];	camera_vup[2]=viewing->vup[2];
	

	for(int i=0;i<3;i++) light_pos[i]=(lighting->light_source)[0].position[i];


	GLdouble diff[3];
	diff[0] = camera_vat[0]-camera_eye[0];	diff[1] = camera_vat[1]-camera_eye[1];	diff[2] = camera_vat[2]-camera_eye[2];
	step = sqrt(diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]);

	glutInit(&argc, argv);
	glutInitWindowSize(viewing->viewport[2],viewing->viewport[3]);
	glutInitWindowPosition(0, 0);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_STENCIL);
	glutCreateWindow("Parallel_GL");	
	glewInit();

	

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

	glutMainLoop();
	return 0;
}



void display()
{
#ifdef MULTI_THREAD
    // reset light triangle
    for(int i=0;i<m_infos.size();i++){
        job_list.push_back(i);
    }
    for(int i=0;i<thread_count;i++){
        sem_post(&semaphores[i]);
    }
    for(int i=0;i<m_infos.size();i++){
        sem_wait(&master_sem);
    }
#else
    for(int i=1;i<m_infos.size();i++){
        modelProcess(i,0);
    }

#endif
    
    

    //cout << "run with" << omp_get_thread_num() << endl;
	// clear the buffer
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);      //清除用color
	glClearDepth(1.0f);                        // Depth Buffer (就是z buffer) Setup
	glEnable(GL_DEPTH_TEST);                   // Enables Depth Testing
	glDepthFunc(GL_LEQUAL);                    // The Type Of Depth Test To Do
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);//這行把畫面清成黑色並且清除z buffer

	// viewport transformation
	glViewport(viewing->viewport[0], viewing->viewport[1], viewing->viewport[2], viewing->viewport[3]);

	// projection transformation
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(viewing->fovy, (GLfloat)windowSize[0]/(GLfloat)windowSize[1], viewing->dnear, viewing->dfar);
	
	// viewing and modeling transformation
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	
	glTranslatef(0,0,-(back_x));
	glTranslatef(0,0,(front_x));
	glTranslatef((left_x),0,0);
	glTranslatef(-(right_x),0,0);
	glRotatef(rotX,1.0,0.0,0.0);  //rotate camera on the x-axis (upward / downward)
    glRotatef(rotY,0.0,1.0,0.0);  //rotate camera on the y-axis (left / right)
	

	gluLookAt(	camera_eye[0], camera_eye[1], camera_eye[2],    // eye
				camera_vat[0], camera_vat[1], camera_vat[2],    // center
				camera_vup[0], camera_vup[1], camera_vup[2]);   // up
	
	//cout<<camera_eye[0]<<" "<<camera_eye[1]<<" "<<camera_eye[2]<<endl;

	//注意light位置的設定，要在gluLookAt之後
	light(1); // only ambient on 
	
	// draw ambient product
	for(int x=0;x<(Scene->scene_model.size());x++){
        auto& model_info = *m_infos[x];
		int lastMaterial = -1;
		glPushMatrix();
			glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
			glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
			glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			
        int gl_draw_type;
        if(x==0||!model_to_line){
            gl_draw_type = GL_TRIANGLES;
        }
        else{
            gl_draw_type = GL_LINE_LOOP;
        }
		for(size_t i=0;i < object[x]->fTotal;++i)
		{
			// set material property if this face used different material
			if(lastMaterial != object[x]->faceList[i].m)
			{
				lastMaterial = (int)object[x]->faceList[i].m;
				glMaterialfv(GL_FRONT, GL_AMBIENT  , object[x]->mList[lastMaterial].Ka);
				glMaterialfv(GL_FRONT, GL_DIFFUSE  , object[x]->mList[lastMaterial].Kd);
				glMaterialfv(GL_FRONT, GL_SPECULAR , object[x]->mList[lastMaterial].Ks);
				glMaterialfv(GL_FRONT, GL_SHININESS, &object[x]->mList[lastMaterial].Ns);
				
				//you can obtain the texture name by object[x]->mList[lastMaterial].map_Kd
				//load them once in the main function before mainloop
				//bind them in display function here
			}	

			glBegin(gl_draw_type);
			for (size_t j=0;j<3;++j)
			{	
				// glTexCoord2fv(object[x]->tList[object[x]->faceList[i][j].t].ptr);
				glNormal3fv(object[x]->nList[object[x]->faceList[i][j].n].ptr);
				//glVertex3fv(object[x]->vList[object[x]->faceList[i][j].v].ptr);
                glVertex3fv(model_info.face_draw[i].tri[j]);
			}
			glEnd();
		}
		glPopMatrix();	
		
	}

	glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE); // color buffer writing off
	glDepthMask(GL_FALSE); // depth buffer writing off
	glEnable(GL_STENCIL_TEST); // enable stencil test to change the values in stencil buffer
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glEnable(GL_CULL_FACE); // enable culling
    

	// front shadowing (mark the places that should be in the shadow)
	glCullFace(GL_BACK); // cull back faces
	glStencilFunc(GL_ALWAYS,1,~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); // meet front face -> stencil value + 1
	
    
    
    
    for(int x=0;x<(Scene->scene_model.size());x++){
        if(!draw_shadow){
            break;
        }
        ModelInfo& Model = *m_infos[x];
		int lastMaterial = -1;
		glPushMatrix();
		glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
		glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
		glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			
        
        for(int i=0;i < Model.face_size;++i)
		{
            TriangleInfo& triangle = Model.btm_tri[i];
            auto tri = triangle.coor;
            if(triangle.dot>0){

				glBegin(GL_POLYGON); // draw shadow polygon 1
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][1].v].ptr);	
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][0].v].ptr);	
				glEnd();
			
				glBegin(GL_POLYGON); // draw shadow polygon 2
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][2].v].ptr);	
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][1].v].ptr);	
				glEnd();

				glBegin(GL_POLYGON); // draw shadow polygon 3
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][0].v].ptr);	
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][2].v].ptr);	
				glEnd();
            }
		}

		glPopMatrix();	
		
	}
    
    
    
	// back shadowing(mark the places that shouldn't be in shadow)
    
	glCullFace(GL_FRONT); // cull front faces
	glStencilFunc(GL_ALWAYS,1,~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_DECR); // if meet back faces -> stencil value-1
    
    for(int x=0;x<(Scene->scene_model.size());x++){
        if(!draw_shadow){
            break;
        }
        ModelInfo& Model = *m_infos[x];
		int lastMaterial = -1;
		glPushMatrix();
		glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
		glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
		glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			
        
        for(int i=0;i < Model.face_size;++i)
		{
            TriangleInfo& triangle = Model.btm_tri[i];
            auto tri = triangle.coor;
            if(triangle.dot>0){

				glBegin(GL_POLYGON); // draw shadow polygon 1
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][1].v].ptr);	
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][0].v].ptr);	
				glEnd();
			
				glBegin(GL_POLYGON); // draw shadow polygon 2
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][2].v].ptr);	
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][1].v].ptr);	
				glEnd();

				glBegin(GL_POLYGON); // draw shadow polygon 3
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][0].v].ptr);	
					glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][2].v].ptr);	
				glEnd();
            }
		}

		glPopMatrix();	
		
	}
    
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE); // color buffer on
	glDepthMask(GL_TRUE); // depth buffer on
	glCullFace(GL_BACK); 
	glDisable(GL_CULL_FACE); // disable back face culling, or some planes will disappear
    
    // draw the unshadowed part
	light(0); // all lights on

    
    if(render_line){
	    // line_loop_test
	    glCullFace(GL_FRONT);
	    glStencilFunc(GL_ALWAYS,1,~0);
	    glStencilOp(GL_KEEP, GL_KEEP, GL_DECR);
        for(int x=0;x<(Scene->scene_model.size());x++){
            ModelInfo& Model = *m_infos[x];
		    int lastMaterial = -1;
		    glPushMatrix();
		    glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
		    glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
		    glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			
        
            for(int i=0;i < Model.face_size;++i)
		    {
                TriangleInfo& triangle = Model.btm_tri[i];
                auto tri = triangle.coor;
                if(triangle.dot>0){

				    glBegin(GL_LINE_LOOP); // draw shadow polygon 1
					    glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					    glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					    glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][1].v].ptr);	
					    glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][0].v].ptr);	
				    glEnd();
			
				    glBegin(GL_LINE_LOOP); // draw shadow polygon 2
					    glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					    glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					    glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][2].v].ptr);	
					    glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][1].v].ptr);	
				    glEnd();

				    glBegin(GL_LINE_LOOP); // draw shadow polygon 3
					    glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					    glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					    glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][0].v].ptr);	
					    glVertex3fv(Model.mesh_ptr->vList[Model.mesh_ptr->faceList[i][2].v].ptr);	
				    glEnd();
                }
		    }

		    glPopMatrix();	
		
	    }
    }
	

	glStencilFunc(GL_EQUAL, 0, ~0); // if stencil value = 0, draw the polygon with new lights
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    int m_size = (Scene->scene_model.size());
	for(unsigned int x=0;x<m_size;x++){
        auto& model_info = *m_infos[x];
		int lastMaterial = -1;
		glPushMatrix();
			glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
			glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
			glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
        int gl_draw_type;
        if(x==0||!model_to_line){
            gl_draw_type = GL_TRIANGLES;
        }
        else{
            gl_draw_type = GL_LINE_LOOP;
        }

		for(int i=0;i < object[x]->fTotal;++i)
		{
            if(x>0&&model_info.face_draw[i].draw==0){
                //cout << "Hidden Face" << endl;
                continue;
            }
			// set material property if this face used different material
			if(lastMaterial != object[x]->faceList[i].m)
			{
				lastMaterial = (int)object[x]->faceList[i].m;
				glMaterialfv(GL_FRONT, GL_AMBIENT  , object[x]->mList[lastMaterial].Ka);
				glMaterialfv(GL_FRONT, GL_DIFFUSE  , object[x]->mList[lastMaterial].Kd);
				glMaterialfv(GL_FRONT, GL_SPECULAR , object[x]->mList[lastMaterial].Ks);
				glMaterialfv(GL_FRONT, GL_SHININESS, &object[x]->mList[lastMaterial].Ns);
				
				//you can obtain the texture name by object[x]->mList[lastMaterial].map_Kd
				//load them once in the main function before mainloop
				//bind them in display function here
			}	
			

			glBegin(gl_draw_type);
			for (size_t j=0;j<3;++j)
			{	
				// glTexCoord2fv(object[x]->tList[object[x]->faceList[i][j].t].ptr);
				glNormal3fv(object[x]->nList[object[x]->faceList[i][j].n].ptr);
				//glVertex3fv(object[x]->vList[object[x]->faceList[i][j].v].ptr);	
                glVertex3fv(model_info.face_draw[i].tri[j]);
			}
			glEnd();
		}
		glPopMatrix();	
		
	}	
    
	glDisable(GL_STENCIL_TEST);
	glutSwapBuffers();
}


#ifdef MULTI_THREAD
void* monitor(void* v_rank){
    int rank = (int)v_rank;
    int job;
    printf("Create:%d\n",rank);
    while(true){
        //printf("rank:%d wait for job...\n",rank);
        sem_wait(&semaphores[rank]);
        pthread_mutex_lock(&mutex);
        //printf("*LOCK* Getting jobs...\n");
        if(job_list.empty()){
            pthread_mutex_unlock(&mutex);
        }
        else{
            job = job_list.back();
            job_list.pop_back();
            pthread_mutex_unlock(&mutex);
            sem_post(&semaphores[rank]);
            for(int i=0;i<workload;i++){
                modelProcess(job,rank);
            }
            //    m_infos[job]->CollisionWithMesh(*m_infos[1]);
            //printf("Finish!\n");
            sem_post(&master_sem);
        }
        
        
    }
    pthread_exit(NULL);
    return NULL;
}

#endif