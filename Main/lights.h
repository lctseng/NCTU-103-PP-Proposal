#ifndef LIGHT_H
#define LIGHT_H

#include<iostream>
#include<vector>
#include"glut.h"

using namespace std;

class source{
public:
	source(string line);
	GLdouble position[3];
	GLfloat a[3]; //ar ag ab
	GLfloat d[3]; //dr dg db
	GLfloat s[3]; //sr sg sb
};

class lights{
public:
	lights(const char* file_name);
	vector<source> light_source;
	GLfloat amb[3];
};

#endif