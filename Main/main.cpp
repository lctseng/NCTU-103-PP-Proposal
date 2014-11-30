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
#include <omp.h>


#define MIPMAP
#define MULTI_THREAD

using namespace std;

vector<mesh*> object;
views* viewing;
lights* lighting;
scenes* Scene;
vector<ModelInfo*> m_infos;
int windowSize[2];
int startX, startY;
int oldX, oldY;
GLdouble rotX = -66, rotY = 54;


int render_line = false;
int model_to_line = true;
int draw_shadow = false;
bool move_enable = false;
bool draw_ground = false;


GLdouble left_l, right_l, back_l, front_l;
GLdouble left_x=7.26;
GLdouble right_x=58.67;
GLdouble back_x=236.36;
GLdouble front_x=144.16;
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

void idle();		// for UBW
int time1,time2;	// for UBW
int speed = 40;		// for UBW

int main(int argc, char** argv)
{
    thread_count = 0;
#ifdef MULTI_THREAD
    workload = 1;
	thread_count = omp_get_num_procs()-1;
#endif
	viewing = new views("GameOfLife.view");
	lighting = new lights("GameOfLife.light");
	Scene = new scenes("scene1.scene");

	for(unsigned int x=0;x<(Scene->scene_model.size());x++){
		const char* model_name = (Scene->scene_model)[x].name.c_str();
		
		mesh* temp = new mesh(model_name);
		object.push_back(temp);
        m_infos.push_back(new ModelInfo(model_name,temp,thread_count));
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

	
    glutIdleFunc(idle);
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
	light(0); // only ambient on 
	
	// draw ambient product
    
	// draw ambient product
    
	for(int x=0;x<(Scene->scene_model.size());x++){
        auto& model_info = *m_infos[x];
        int gl_draw_type;
        if((x==0&&draw_ground)||!model_to_line){
            gl_draw_type = GL_TRIANGLES;
        }
        else{
            gl_draw_type = GL_POINTS;
        }
        glBegin(gl_draw_type);
        for(int i=0;i < ModelInfo::WORLD_HEIGHT ;++i)
		{	
            for(int j=0;j < ModelInfo::WORLD_WIDTH ;++j){
                if(!model_info.vertexList[i][j].dead){
                    
                    glVertex3fv(model_info.vertexList[i][j].ptr);
			        
                }
            }
            
			
		}
        glEnd();
		
	}

    
	glutSwapBuffers();
}



#ifdef MULTI_THREAD
void* monitor(void* v_rank){
    int rank = (int)v_rank;
    printf("Create:%d , \n",rank);
    while(true){
        //printf("rank:%d wait for job...\n",rank);
        sem_wait(&semaphores[rank]);
        // job for this rank
        m_infos[0]->ThreadGeneration(rank);
        m_infos[0]->ThreadMigrate(rank);
        sem_post(&master_sem);
    }
    pthread_exit(NULL);
    return NULL;
}
#endif

void idle(){
    if(move_enable){
#ifdef MULTI_THREAD
        for(int i=0;i<thread_count;i++){
            sem_post(&semaphores[i]);
        }
        for(int i=0;i<m_infos.size();i++){
            sem_wait(&master_sem);
        }
#else
        m_infos[0]->GenerateNextGeneration();
        m_infos[0]->MigrateToNext();

#endif
  
        glutPostRedisplay();
    }
}