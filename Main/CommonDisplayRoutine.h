#include "glut.h"
#include "lights.h"
#include "view.h"
#include "scene.h"

extern views* viewing;
extern lights* lighting;
extern scenes* Scene;
extern GLfloat light_pos[3];
extern GLdouble camera_eye[3];
extern GLdouble camera_vat[3];
extern GLdouble camera_vup[3];
extern int windowSize[2];
extern int startX, startY;
extern int oldX, oldY;
extern GLdouble rotX, rotY;
extern vector<ModelInfo*> m_infos;
extern GLdouble left_l, right_l, back_l, front_l;
extern GLdouble left_x;
extern GLdouble right_x;
extern GLdouble back_x;
extern GLdouble front_x;
extern GLdouble step;
extern int time1,time2;	// for UBW
extern int speed ;		// for UBW
extern bool move_enable;

void idle(); // for UBW
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
	static int select = 1;
    auto& selectModel = *m_infos[select];
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
	case 'h':	// first object movement
		selectModel.GoRight(step/30);
		printf("v%d : %f\t%f\t%f\n", select,
						selectModel.vertexList[0].ptr[0],
						selectModel.vertexList[0].ptr[1],
						selectModel.vertexList[0].ptr[2]);
		break;
	case 'f':	// first object movement
		selectModel.GoLeft(step/30);
		printf("v%d : %f\t%f\t%f\n", select,
						selectModel.vertexList[0].ptr[0],
						selectModel.vertexList[0].ptr[1],
						selectModel.vertexList[0].ptr[2]);
		break;
	case 'g':	// first object movement
		selectModel.GoDown(step/30);
		printf("v%d : %f\t%f\t%f\n", select,
						selectModel.vertexList[0].ptr[0],
						selectModel.vertexList[0].ptr[1],
						selectModel.vertexList[0].ptr[2]);
		break;
	case 't':	// first object movement
		selectModel.GoUp(step/30);
		printf("v%d : %f\t%f\t%f\n", select,
						selectModel.vertexList[0].ptr[0],
						selectModel.vertexList[0].ptr[1],
						selectModel.vertexList[0].ptr[2]);
		break;
	case '1':
		printf("press 1\n");
		select = 1;
		break;
	case '2':
		printf("press 2\n");
		select = 2;
		break;
	case '3':
		printf("press 3\n");
		select = 3;
		break;
	case 'r':
		for(int x=1;x<(Scene->scene_model.size());x++){
			printf("v%d : %f\t%f\t%f\n", x,
							(*m_infos[x]).vertexList[1].ptr[0],
							(*m_infos[x]).vertexList[1].ptr[1],
							(*m_infos[x]).vertexList[1].ptr[2]);
		}
		break;
	case 'z':    // toggle move
        move_enable = !move_enable;
        break;
	case 'x':   // reset collision flag
        for(int x=1;x<(Scene->scene_model.size());x++){
		    auto& thisModel = *m_infos[x];
            thisModel.resetCollision();
        }
		break;
	case 'c':
		speed *= -1;
		break;
	default:
		break;
	}
	glutPostRedisplay();
	
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
}

void modelProcess(int job,int rank){
    m_infos[job]->GenerateBottomTriangle(light_pos,rank); 
    
    if(job>1){
        if(move_enable){
            m_infos[job]->ApplySpeed();
        }
        for(int i=1;i<(Scene->scene_model.size());i++){
            if(i!=job){ // the other model
                m_infos[job]->CollisionWithMesh(*m_infos[i]);
            }
        }
    }
}
