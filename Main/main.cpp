#include <cstdio>
#include <iostream>


#include "glew.h"
#include "mesh.h"
#include "glut.h"
#include "glaux.h"
#include "wglew.h"
#include "omp.h"
#include "view.h"
#include "scene.h"
#include "lights.h"

#include <vector>
#include <string>
#include <cmath>
#include <fstream>

#include "CLInit.h"

const unsigned int WORLD_HEIGHT = 500;
const unsigned int WORLD_WIDTH = 500;


#define OPEN_CL
using namespace std;

views* viewing;
lights* lighting;
scenes* Scene;

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

void light(bool);
void idle();
void display();
void keyboard(unsigned char, int, int);
void mouse(int, int, int, int);
void motion(int, int);

void reshape(GLsizei , GLsizei );
/* CL */


/* GOL */
GLfloat* world;
GLbyte* dead;
GLfloat* color;
unsigned int worldSize;
unsigned int worldBytes;
void initWorld();
void MigrateToNext();
void GenerateNextGeneration();

int main(int argc, char** argv)
{
    // init world
    initWorld();

	viewing = new views("GameOfLife.view");
	lighting = new lights("GameOfLife.light");
	Scene = new scenes("GameOfLife.scene");

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
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_COLOR_MATERIAL);
    //buffer
    
    //glPointSize( 2. );
    
#ifdef OPEN_CL
    initDevice();
    initBuffer();
    loadCLProgram();
    setKernel();
#endif

	glutMainLoop();
    delete[] world;
    delete[] dead;
    delete[] color;
	return 0;
}

void initWorld(){
    worldSize = WORLD_HEIGHT * WORLD_WIDTH;
    worldBytes = worldSize * 3 * sizeof(GLfloat);
    world = new GLfloat[worldSize*3];
    color = new GLfloat[worldSize*3];
    dead = new GLbyte[worldSize];
    for(int i=0;i<WORLD_HEIGHT;i++){
        for(int j=0;j<WORLD_WIDTH;j++){
            world[(i*WORLD_WIDTH+j)*3+0] = i*0.1;
            world[(i*WORLD_WIDTH+j)*3+1] = j*0.1;
            world[(i*WORLD_WIDTH+j)*3+2] = 10.0f;
            color[(i*WORLD_WIDTH+j)*3+0] = 1.0f;
            color[(i*WORLD_WIDTH+j)*3+1] = 0.5f;
            color[(i*WORLD_WIDTH+j)*3+2] = 0.0f;
            dead[i*WORLD_WIDTH+j] = rand()%2!=0;

        }
    }
    cout << "World Initialized " << endl;
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



    
    
    runKernel();
    
    glBindBuffer( GL_ARRAY_BUFFER , Vbuf);
    glVertexPointer( 3, GL_FLOAT, 0, (void *)0 );
    glEnableClientState( GL_VERTEX_ARRAY );
 
    glBindBuffer( GL_ARRAY_BUFFER, Cbuf );
    glColorPointer( 3, GL_FLOAT, 0, (void *)0 );
    glEnableClientState( GL_COLOR_ARRAY );
    
    
    glDrawArrays( GL_POINTS, 0, worldSize);
    glDisableClientState( GL_VERTEX_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );
    glBindBuffer( GL_ARRAY_BUFFER , 0);
    
	glutSwapBuffers();
    glFlush();
}



void idle(){
    if(move_enable){
        GenerateNextGeneration();
        MigrateToNext();
        glutPostRedisplay();
        
    }
    
}

/*

void MigrateToNext(){
    for(int i=0;i<WORLD_HEIGHT;i++){
        for(int j=0;j<WORLD_WIDTH;j++){
            vertexList[i][j].dead = nextList[i][j].dead;
        }
    }
}

void GenerateNextGeneration(){
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
*/

void MigrateToNext(){}
void GenerateNextGeneration(){}

void light(bool only_ambient)
{	
	
	for(unsigned int i=0; i<(lighting->light_source.size()); i++){
		GLfloat no_specular[] = {0, 0, 0, 0};
		GLfloat no_diffuse[] = {0, 0, 0, 0};

		GLfloat light_specular[] = {(lighting->light_source)[i].s[0], (lighting->light_source)[i].s[1], (lighting->light_source)[i].s[2], 1.0};
		GLfloat light_diffuse[] = {(lighting->light_source)[i].d[0], (lighting->light_source)[i].d[1], (lighting->light_source)[i].d[2], 1.0};
		GLfloat light_ambient[] = {(lighting->light_source)[i].a[0], (lighting->light_source)[i].a[1], (lighting->light_source)[i].a[2], 1.0};
		GLfloat light_position[] = {light_pos[0],light_pos[1],light_pos[2], 1.0};
		glShadeModel(GL_SMOOTH);

		
		// z buffer enable
		glEnable(GL_DEPTH_TEST);

		// enable lighting
		glEnable(GL_LIGHTING);
		// set light property
		glEnable(GL_LIGHT0+i);
		glLightfv(GL_LIGHT0+i, GL_POSITION, light_position);
		if(only_ambient){
			glLightfv(GL_LIGHT0+i, GL_DIFFUSE, no_diffuse);
			glLightfv(GL_LIGHT0+i, GL_SPECULAR, no_specular);
		}
		else{
			glLightfv(GL_LIGHT0+i, GL_DIFFUSE, light_diffuse);
			glLightfv(GL_LIGHT0+i, GL_SPECULAR, light_specular);
		}
		glLightfv(GL_LIGHT0+i, GL_AMBIENT, light_ambient);
	}
	GLfloat ambient[]={lighting->amb[0], lighting->amb[1], lighting->amb[2]};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);

}

void reshape(GLsizei w, GLsizei h)
{
	windowSize[0] = w;
	windowSize[1] = h;
}

void keyboard(unsigned char key, int x, int y)
{
	static int select = 0;
	//printf("you press the key %c \n", key);
	//printf("the mouse is on %lf %lf \n", x, y);
	switch(key)
	{
	case 's':	// backward
		back_x = back_x+step/20;
		break;
	case 'w':	// forward
		front_x = front_x+step/20;
		break;
	case 'a':	// left
		left_x = left_x+step/30;
		break;
	case 'd':	// right
		right_x = right_x+step/30;
		break;
	case 'l':	// light movement
		light_pos[0] =light_pos[0]-step/30;
		break;
	case 'j':	// light movement
		light_pos[0] =light_pos[0]+step/30;
		break;
	case 'k':	// light movement
		light_pos[2] =light_pos[2]-step/30;
		break;
	case 'i':	// light movement
		light_pos[2] =light_pos[2]+step/30;
		break;
	case '1':
		printf("press 1\n");
		//select = 1;
		break;
	case '2':
		printf("press 2\n");
		//select = 2;
		break;
	case '3':
		printf("press 3\n");
		//select = 3;
		break;
	case 'z':    // Migrate
        GenerateNextGeneration();
        MigrateToNext();
        break;
    case 'x':
        move_enable = !move_enable;
        break;
	default:
		break;
	}
	glutPostRedisplay();
    //printf("Light %.2f %.2f %2f \n", light_pos[0],light_pos[1],light_pos[2]);
	//printf("Pos %.2f %.2f %2f %.2f \n", back_x,front_x,left_x,right_x);
}
void mouse(int button, int state, int x, int y){
	if (state == GLUT_DOWN){
		startX = x;
		startY = y;
		oldX = x; // without assigning oldX / oldY, vX / vY will count new x / y, which will lead rotation wrong positioning 
		oldY = y;
	}
	else if(state == GLUT_UP){
		//printf("the mouse moves %d %d \n", x-startX,  y-startY);
	}
}
void motion(int x, int y){
	//printf("the mouse is moving to %d %d \n", x, y);
	GLdouble vX = x-oldX;
	GLdouble vY = y-oldY;
	rotX = rotX + vY/10; // total amount of upward / downward motion
	rotY = rotY + vX/10; // total amount of left / right motion
	oldX = x;
	oldY = y;
	glutPostRedisplay();
    //printf("Eye %2.f %2.f\n",rotX,rotY);
}
