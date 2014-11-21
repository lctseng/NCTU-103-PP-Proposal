#ifndef VIEW_H
#define VIEW_H

#include<iostream>
#include<vector>
#include "glut.h"
using namespace std;


class views{
public:

	views(const char* file_name);
	GLdouble eye[3];
	GLdouble vat[3];
	GLdouble vup[3];
	GLdouble fovy;
	GLdouble dnear;
	GLdouble dfar;
	GLdouble viewport[4];
};

#endif