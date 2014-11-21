#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <fstream>
#include "scene.h"

using namespace std;


scenes::scenes(const char* file_name){
	
	fstream sceneFile(file_name);
	
	while(!sceneFile.eof()){
		string line;
		getline(sceneFile,line);
		stringstream ss;
		ss << line;
		string term;
		ss >> term;
		if(term=="model"){
			model m(line);
			scene_model.push_back(m);
		}
		else{
			cout << "Wrong scene_file format!\n";
		}

	}
}
model::model(string line)
{
	stringstream ss;
	ss << line;
	string term;
	ss >> term >> name >> sc[0] >> sc[1] >> sc[2] >> angle >> r[0] >> r[1] >> r[2] >> t[0] >> t[1] >> t[2];
}