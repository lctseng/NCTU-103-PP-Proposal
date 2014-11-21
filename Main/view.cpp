#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <fstream>
#include "view.h"

using namespace std;



views::views(const char* file_name){
	
	fstream viewFile(file_name);
	
	int i=0;
	while(!viewFile.eof()){
		string line;
		getline(viewFile,line);
		stringstream ss;
		ss << line;
		string term;
		ss >> term;
		if(term=="eye"){
			ss >> eye[0] >> eye[1] >> eye[2];
		}
		else if(term=="vat"){
			ss >> vat[0] >> vat[1] >> vat[2];
		}
		else if(term=="vup"){
			ss >> vup[0] >> vup[1] >> vup[2];
		}
		else if(term=="fovy"){
			ss >> fovy;
		}
		else if(term=="dnear"){
			ss >> dnear;
		}
		else if(term=="dfar"){
			ss >> dfar;
		}
		else if(term=="viewport"){
			ss >> viewport[0] >> viewport[1] >> viewport[2] >> viewport[3];
		}
		else{
			cout << "Wrong view_file format!\n";
		}
	}
}
