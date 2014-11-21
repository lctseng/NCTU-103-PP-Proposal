#ifndef SCENE_H
#define SCENE_H

#include<iostream>
#include<vector>
#include<string>
#include"glut.h"

using namespace std;

class model{
public:
	model(string line);
	string name;
	GLfloat sc[3];
	GLfloat angle;
	GLfloat r[3]; 
	GLfloat t[3]; 
};

class scenes{
public:
	scenes(const char* file_name);
	vector<model> scene_model;
};

#endif