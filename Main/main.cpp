#include <cstdio>
#include <iostream>

#include "glew.h"
#include "mesh.h"
#include "glut.h"
#include "glaux.h"
#include "wglew.h"
#include "omp.h"

#include "lights.h"
#include "view.h"
#include "scene.h"
#include <vector>
#include <string>
#include <cmath>
#include <fstream>

#define SHADOW_RATE 10
#define MIPMAP

using namespace std;

vector<mesh*> object;
views* viewing;
lights* lighting;
scenes* Scene;
int windowSize[2];
int startX, startY;
int oldX, oldY;
GLdouble rotX, rotY;

int compute_shadow = true;
int render_shadow = true;
int render_line = false;

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

void light(bool);
void display();
void keyboard(unsigned char, int, int);
void mouse(int, int, int, int);
void motion(int, int);

void reshape(GLsizei , GLsizei );

int main(int argc, char** argv)
{
	
    cout << omp_get_num_procs() << endl;
	viewing = new views("assignment3.view");
	lighting = new lights("assignment3.light");
	Scene = new scenes("scene1.scene");

	for(unsigned int x=0;x<(Scene->scene_model.size());x++){
		const char* model_name = (Scene->scene_model)[x].name.c_str();
		
		mesh* temp = new mesh(model_name);
		object.push_back(temp);
	}
	
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
	glutCreateWindow("Assignment3");	
	glewInit();

	
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutMainLoop();
	
	return 0;
}

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
	
	cout<<camera_eye[0]<<" "<<camera_eye[1]<<" "<<camera_eye[2]<<endl;

	//注意light位置的設定，要在gluLookAt之後
	light(1); // only ambient on 
	
	// draw ambient product
	for(unsigned int x=0;x<(Scene->scene_model.size());x++){
		int lastMaterial = -1;
		glPushMatrix();
			glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
			glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
			glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			

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
			
			glBegin(GL_TRIANGLES);
			for (size_t j=0;j<3;++j)
			{	
				// glTexCoord2fv(object[x]->tList[object[x]->faceList[i][j].t].ptr);
				glNormal3fv(object[x]->nList[object[x]->faceList[i][j].n].ptr);
				glVertex3fv(object[x]->vList[object[x]->faceList[i][j].v].ptr);	
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
	for(unsigned int x=0;x<(Scene->scene_model.size());x++){
		
		int lastMaterial = -1;
		glPushMatrix();
			glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
			glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
			glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			

		for(size_t i=0;i < object[x]->fTotal;++i)
		{
            GLfloat* tri[3]; // bottom triangle
            GLfloat LtoV[3]; // light to vertex
            GLfloat AtoL[3]; // vertex A to light
			GLfloat AtoB[3]; // vertex A to vertex B
			GLfloat AtoC[3]; // vertex A to vertex C
            GLfloat ABcrossAC[3];
            GLfloat dot=0;
			if(compute_shadow){
			
			for(size_t j=0;j<3;++j){
				tri[j]= new GLfloat[3];
			}

			
			
			for (size_t j=0;j<3;++j)
			{	
				for(size_t k=0;k<3;++k) LtoV[k] = object[x]->vList[object[x]->faceList[i][j].v].ptr[k] - light_pos[k];
				for(size_t k=0;k<3;++k)	tri[j][k] = object[x]->vList[object[x]->faceList[i][j].v].ptr[k] + LtoV[k]*2;	
			}

			// judge whether the polygon faces the light (if light-faceing->draw shadow polygons)
			
			for(size_t k=0; k<3;++k){
				AtoL[k] = light_pos[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
				AtoB[k] = object[x]->vList[object[x]->faceList[i][1].v].ptr[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
				AtoC[k] = object[x]->vList[object[x]->faceList[i][2].v].ptr[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
			}

			
			ABcrossAC[0] = AtoB[1]*AtoC[2] - AtoB[2]*AtoC[1];
			ABcrossAC[1] = AtoB[2]*AtoC[0] - AtoB[0]*AtoC[2];
			ABcrossAC[2] = AtoB[0]*AtoC[1] - AtoB[1]*AtoC[0];

			

			dot = AtoL[0]*ABcrossAC[0] + AtoL[1]*ABcrossAC[1] + AtoL[2]*ABcrossAC[2];
            }
			// draw shadow polygons
            if (render_shadow){
			if(dot>0){
				//glBegin(GL_TRIANGLES); // draw bottom triangle
				//for (size_t j=0;j<3;++j)
				//{	
				//	glNormal3fv(object[x]->nList[object[x]->faceList[i][j].n].ptr);
				//	glVertex3f(tri[j][0], tri[j][1], tri[j][2]);
				//}
				//glEnd();

				glBegin(GL_POLYGON); // draw shadow polygon 1
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][1].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][0].v].ptr);	
				glEnd();
			
				glBegin(GL_POLYGON); // draw shadow polygon 2
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][2].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][1].v].ptr);	
				glEnd();

				glBegin(GL_POLYGON); // draw shadow polygon 3
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][0].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][2].v].ptr);	
				glEnd();
            }}
		}

		glPopMatrix();	
		
	}
    
	// back shadowing(mark the places that shouldn't be in shadow)
    
	glCullFace(GL_FRONT); // cull front faces
	glStencilFunc(GL_ALWAYS,1,~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_DECR); // if meet back faces -> stencil value-1
	
    for(unsigned int x=0;x<(Scene->scene_model.size());x++){
		
		int lastMaterial = -1;
		glPushMatrix();
			glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
			glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
			glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			

		for(size_t i=0;i < object[x]->fTotal;++i)
		{
				
			GLfloat* tri[3]; // bottom triangle
            GLfloat LtoV[3]; // light to vertex
            GLfloat AtoL[3]; // vertex to light
			GLfloat AtoB[3];
			GLfloat AtoC[3];
            GLfloat ABcrossAC[3];
            GLfloat dot=0;

            if(compute_shadow){
			for(size_t j=0;j<3;++j){
				tri[j]= new GLfloat[3];
			}

			
			for (size_t j=0;j<3;++j)
			{	
				for(size_t k=0;k<3;++k) LtoV[k] = object[x]->vList[object[x]->faceList[i][j].v].ptr[k] - light_pos[k];
				for(size_t k=0;k<3;++k)	tri[j][k] = object[x]->vList[object[x]->faceList[i][j].v].ptr[k] + LtoV[k]*SHADOW_RATE;
				
			}

			
			//glBegin(GL_TRIANGLES); // draw bottom triangle
			//for (size_t j=0;j<3;++j)
			//{	
			//	glNormal3fv(object[x]->nList[object[x]->faceList[i][j].n].ptr);
			//	glVertex3f(tri[j][0], tri[j][1], tri[j][2]);
			//}
			//glEnd();
			
			for(size_t k=0; k<3;++k){
				AtoL[k] = light_pos[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
				AtoB[k] = object[x]->vList[object[x]->faceList[i][1].v].ptr[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
				AtoC[k] = object[x]->vList[object[x]->faceList[i][2].v].ptr[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
			}

			
			ABcrossAC[0] = AtoB[1]*AtoC[2] - AtoB[2]*AtoC[1];
			ABcrossAC[1] = AtoB[2]*AtoC[0] - AtoB[0]*AtoC[2];
			ABcrossAC[2] = AtoB[0]*AtoC[1] - AtoB[1]*AtoC[0];

			

			dot = AtoL[0]*ABcrossAC[0] + AtoL[1]*ABcrossAC[1] + AtoL[2]*ABcrossAC[2];

            }
            if(render_shadow){
			if(dot>0){
				glBegin(GL_POLYGON); // draw shadow polygon 1
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][1].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][0].v].ptr);	
				glEnd();
			
				glBegin(GL_POLYGON); // draw shadow polygon 2
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][2].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][1].v].ptr);	
				glEnd();

				glBegin(GL_POLYGON); // draw shadow polygon 3
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][0].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][2].v].ptr);	
				glEnd();
			}
            }
		}
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
	for(unsigned int x=0;x<(Scene->scene_model.size());x++){
		
		int lastMaterial = -1;
		glPushMatrix();
			glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
			glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
			glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			

		for(size_t i=0;i < object[x]->fTotal;++i)
		{
				
			GLfloat* tri[3]; // bottom triangle
			for(size_t j=0;j<3;++j){
				tri[j]= new GLfloat[3];
			}

			GLfloat LtoV[3]; // light to vertex
			for (size_t j=0;j<3;++j)
			{	
				for(size_t k=0;k<3;++k) LtoV[k] = object[x]->vList[object[x]->faceList[i][j].v].ptr[k] - light_pos[k];
				for(size_t k=0;k<3;++k)	tri[j][k] = object[x]->vList[object[x]->faceList[i][j].v].ptr[k] + LtoV[k]*SHADOW_RATE;
				
			}

			GLfloat AtoL[3]; // vertex to light
			GLfloat AtoB[3];
			GLfloat AtoC[3];
			for(size_t k=0; k<3;++k){
				AtoL[k] = light_pos[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
				AtoB[k] = object[x]->vList[object[x]->faceList[i][1].v].ptr[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
				AtoC[k] = object[x]->vList[object[x]->faceList[i][2].v].ptr[k] - object[x]->vList[object[x]->faceList[i][0].v].ptr[k];
			}

			GLfloat ABcrossAC[3];
			ABcrossAC[0] = AtoB[1]*AtoC[2] - AtoB[2]*AtoC[1];
			ABcrossAC[1] = AtoB[2]*AtoC[0] - AtoB[0]*AtoC[2];
			ABcrossAC[2] = AtoB[0]*AtoC[1] - AtoB[1]*AtoC[0];

			GLfloat dot;

			dot = AtoL[0]*ABcrossAC[0] + AtoL[1]*ABcrossAC[1] + AtoL[2]*ABcrossAC[2];


			if(dot>0){
				glBegin(GL_LINE_LOOP); // draw shadow polygon 1
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][1].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][0].v].ptr);	
				glEnd();
			
				glBegin(GL_LINE_LOOP); // draw shadow polygon 2
					glVertex3f(tri[1][0],tri[1][1],tri[1][2]);
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][2].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][1].v].ptr);	
				glEnd();

				glBegin(GL_LINE_LOOP); // draw shadow polygon 3
					glVertex3f(tri[2][0],tri[2][1],tri[2][2]);
					glVertex3f(tri[0][0],tri[0][1],tri[0][2]);
					glVertex3fv(object[x]->vList[object[x]->faceList[i][0].v].ptr);	
					glVertex3fv(object[x]->vList[object[x]->faceList[i][2].v].ptr);	
				glEnd();
			}
		}
	}
    }
	

	glStencilFunc(GL_EQUAL, 0, ~0); // if stencil value = 0, draw the polygon with new lights
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	for(unsigned int x=0;x<(Scene->scene_model.size());x++){
		int lastMaterial = -1;
		glPushMatrix();
			glTranslatef((Scene->scene_model)[x].t[0], (Scene->scene_model)[x].t[1], (Scene->scene_model)[x].t[2]);
			glRotatef((Scene->scene_model)[x].angle, (Scene->scene_model)[x].r[0], (Scene->scene_model)[x].r[1], (Scene->scene_model)[x].r[2]);
			glScalef((Scene->scene_model)[x].sc[0], (Scene->scene_model)[x].sc[1], (Scene->scene_model)[x].sc[2]);
			

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
			
			glBegin(GL_TRIANGLES);
			for (size_t j=0;j<3;++j)
			{	
				// glTexCoord2fv(object[x]->tList[object[x]->faceList[i][j].t].ptr);
				glNormal3fv(object[x]->nList[object[x]->faceList[i][j].n].ptr);
				glVertex3fv(object[x]->vList[object[x]->faceList[i][j].v].ptr);	
			}
			glEnd();
		}
		glPopMatrix();	
		
	}	
	glDisable(GL_STENCIL_TEST);
	glutSwapBuffers();
}

void reshape(GLsizei w, GLsizei h)
{
	windowSize[0] = w;
	windowSize[1] = h;
}

void keyboard(unsigned char key, int x, int y)
{
	
	printf("you press the key %c \n", key);
	printf("the mouse is on %lf %lf \n", x, y);
	/*GLdouble diff[3];
	diff[0] = camera_vat[0]-camera_eye[0];
	diff[1] = camera_vat[1]-camera_eye[1]
	diff[2] = camera_vat[2]-camera_eye[2];*/
	if(key=='s'){                          // backward
		/*camera_eye[0]=camera_eye[0]-diff[0]/20;
		camera_eye[1]=camera_eye[1]-diff[1]/20;
		camera_eye[2]=camera_eye[2]-diff[2]/20;*/
		back_x = back_x+step/20;
	}
	else if(key=='w'){                     // forward
		/*camera_eye[0]=camera_eye[0]+diff[0]/20;
		camera_eye[1]=camera_eye[1]+diff[1]/20;
		camera_eye[2]=camera_eye[2]+diff[2]/20;*/
		front_x = front_x+step/20;
	}
	else if(key=='a'){                     // left
		/*GLdouble cross[3];
		cross[0] = camera_vup[1] * diff[2] - diff[1] * camera_vup[2];
		cross[1] = -camera_vup[0] * diff[2] + diff[0] * camera_vup[2];
		cross[2] = camera_vup[0] * diff[1] - diff[0] * camera_vup[1];
		GLdouble length = sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);

		camera_eye[0]=camera_eye[0]+cross[0]/20; // eye and vat shift together
		camera_eye[1]=camera_eye[1]+cross[1]/20;
		camera_eye[2]=camera_eye[2]+cross[2]/20;
		camera_vat[0]=camera_vat[0]+cross[0]/20;
		camera_vat[1]=camera_vat[1]+cross[1]/20;
		camera_vat[2]=camera_vat[2]+cross[2]/20;*/
		left_x = left_x+step/30;
	}
	else if(key=='d'){                    // right
		/*GLdouble cross[3];
		cross[0] = camera_vup[1] * diff[2] - diff[1] * camera_vup[2];
		cross[1] = -camera_vup[0] * diff[2] + diff[0] * camera_vup[2];
		cross[2] = camera_vup[0] * diff[1] - diff[0] * camera_vup[1];
		GLdouble length = sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);

		camera_eye[0]=camera_eye[0]-cross[0]/20;
		camera_eye[1]=camera_eye[1]-cross[1]/20;
		camera_eye[2]=camera_eye[2]-cross[2]/20;
		camera_vat[0]=camera_vat[0]-cross[0]/20;
		camera_vat[1]=camera_vat[1]-cross[1]/20;
		camera_vat[2]=camera_vat[2]-cross[2]/20;*/
		right_x = right_x+step/30;
	}
	else if(key=='l'){
		light_pos[0] =light_pos[0]-step/30;
	}
	else if(key=='j'){
		light_pos[0] =light_pos[0]+step/30;
	}
	else if(key=='k'){
		light_pos[2] =light_pos[2]-step/30;
	}
	else if(key=='i'){
		light_pos[2] =light_pos[2]+step/30;
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
		printf("the mouse moves %d %d \n", x-startX,  y-startY);
	}
}
void motion(int x, int y){
	printf("the mouse is moving to %d %d \n", x, y);
	GLdouble vX = x-oldX;
	GLdouble vY = y-oldY;
	rotX = rotX + vY/10; // total amount of upward / downward motion
	rotY = rotY + vX/10; // total amount of left / right motion
	oldX = x;
	oldY = y;
	glutPostRedisplay();
	//if(rotX >= 360) rotX = rotX-360;
	//if(rotY >= 360) rotY = rotY-360;
	/*GLdouble diff[3];
	diff[0] = camera_vat[0]-camera_eye[0];
	diff[1] = camera_vat[1]-camera_eye[1];
	diff[2] = camera_vat[2]-camera_eye[2];
	GLdouble diff_unit[3];
	diff_unit[0] = diff[0] / sqrt(diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]);
	diff_unit[1] = diff[1] / sqrt(diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]);
	diff_unit[2] = diff[2] / sqrt(diff[0]*diff[0] + diff[1]*diff[1] + diff[2]*diff[2]);
		
	GLdouble cross[3];
	cross[0] = camera_vup[1] * diff[2] - diff[1] * camera_vup[2];
	cross[1] = -camera_vup[0] * diff[2] + diff[0] * camera_vup[2];
	cross[2] = camera_vup[0] * diff[1] - diff[0] * camera_vup[1];
	
	GLdouble unit[3];
	unit[0] = cross[0] / sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
	unit[1] = cross[1] / sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);
	unit[2] = cross[2] / sqrt(cross[0]*cross[0] + cross[1]*cross[1] + cross[2]*cross[2]);

	GLdouble up[3];
	up[0] = diff[1] * unit[2] - unit[1] * diff[2];
	up[1] = -diff[0] * unit[2] + unit[0] * diff[2];
	up[2] = diff[0] * unit[1] - unit[0] * diff[1];
	GLdouble up_unit[3];
	up_unit[0] = up[0] / sqrt(up[0]*up[0] + up[1]*up[1] + up[2]*up[2]);
	up_unit[1] = up[1] / sqrt(up[0]*up[0] + up[1]*up[1] + up[2]*up[2]);
	up_unit[2] = up[2] / sqrt(up[0]*up[0] + up[1]*up[1] + up[2]*up[2]);
	
	GLdouble vX = x-oldX;
	GLdouble vY = oldY-y;
	GLdouble length = sqrt(vX*vX + vY*vY);

	if(diff_unit[1]>=0.999){
		camera_vat[0]=camera_vat[0] + ((-1)*unit[0]*(vX/length)*5 + up_unit[0]*(vY/length)*5)*10;
		camera_vat[1]=camera_vat[1] + ((-1)*unit[1]*(vX/length)*5 + up_unit[1]*(vY/length)*5)*10;
		camera_vat[2]=camera_vat[2] + ((-1)*unit[2]*(vX/length)*5 + up_unit[2]*(vY/length)*5)*10;
	}
	else{
	camera_vat[0]=camera_vat[0] + (-1)*unit[0]*(vX/length)/2 + up_unit[0]*(vY/length)/2;
	camera_vat[1]=camera_vat[1] + (-1)*unit[1]*(vX/length)/2 + up_unit[1]*(vY/length)/2;
	camera_vat[2]=camera_vat[2] + (-1)*unit[2]*(vX/length)/2 + up_unit[2]*(vY/length)/2;
	}
	
	glutPostRedisplay();*/
}
