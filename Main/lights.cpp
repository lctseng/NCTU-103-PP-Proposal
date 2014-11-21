#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <fstream>
#include "lights.h"

using namespace std;


lights::lights(const char* file_name){
	
	fstream lightFile(file_name);
	

	while(!lightFile.eof()){
		string line;
		getline(lightFile,line);
		stringstream ss;
		ss << line;
		string term;
		ss >> term;
		if(term=="light"){
			source l(line);
			light_source.push_back(l);
		}
		else if(term=="ambient"){
			ss >> amb[0] >> amb[1] >> amb[2];
		}
		else{
			cout << "Wrong lights_file format!\n";
		}
	}
}
source::source(string line)
{
	stringstream ss;
	ss << line;
	string term;
	ss >> term >> position[0] >> position[1] >> position[2] >> a[0] >> a[1] >> a[2] >> d[0] >> d[1] >> d[2] >> s[0] >> s[1] >> s[2];
}