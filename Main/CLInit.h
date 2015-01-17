#include "CL/cl.h"
#include "CL/cl_gl.h"
#include <iostream>
using namespace std;

#define BUF_SIZE 20000

cl_platform_id platform;
cl_device_id device;
cl_context ctx;
cl_command_queue cmdQue;
cl_int clErrno;
cl_mem vert_in;
cl_mem color_out;
cl_mem dead_in;
cl_mem dead_out;
cl_program clProgram;
cl_kernel clKernelCompute;
cl_kernel clKernelMigrate;
extern GLfloat* world;
extern GLfloat* color;
extern GLbyte* dead;
extern size_t worldSize;
extern size_t worldBytes;
extern const unsigned int WORLD_HEIGHT;
extern const unsigned int WORLD_WIDTH;
/* CGL */
GLuint Vbuf,Cbuf;

bool checkCLGL();
bool IsCLExtensionSupported( const char *extension );

void initDevice(){
    // get plaform
    cerr << "Getting Platform Info..." << endl;
    cl_uint plaNum;
    clErrno = clGetPlatformIDs(
        1, // max number
        &platform, // point to store
        &plaNum // result num
      );
    if(clErrno == CL_SUCCESS){
        cerr << "CL platform ID:" << platform << endl;
    }
    else{
        cerr << "Fail to get CL platform ID!" << endl;
        exit(1);
    }
    // setting device
    cerr << "Setting Device..." << endl;
    cl_uint devNum;
    clErrno = clGetDeviceIDs(
        platform,
        CL_DEVICE_TYPE_GPU,
        1, // number of need
        &device, // result store
        &devNum
      );

    if(clErrno == CL_SUCCESS){
        cerr << "CL device ID:" << device << endl;
    }
    else{
        cerr << "Fail to get CL device ID!" << endl;
        exit(1);
    }

    if(!checkCLGL()){
        exit(1);
    }

    // create context
    cerr << "Creating Context..." << endl;
    cl_context_properties props[ ] =
    {
        CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext( ),
        CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC( ),
        CL_CONTEXT_PLATFORM, (cl_context_properties) platform,
        0
    };
    ctx= clCreateContext(
        props, // context properry
        1, // number of device
        &device, // device id
        NULL, // callback, NONE
        NULL, // user data, NONE
        &clErrno
      );
    if(clErrno == CL_SUCCESS){
        cerr << "CL context ID:" << ctx << endl;
    }
    else{
        cerr << "Fail to create context!" << endl;
        exit(1);
    }
    // create command queue
    cerr << "Creating Command Queue..." << endl;
    cmdQue = clCreateCommandQueue(
        ctx,
        device,
        NULL, // properties
        &clErrno
      );
    if(clErrno == CL_SUCCESS){
        cerr << "CL command queue ID:" << cmdQue << endl;
    }
    else{
        cerr << "Fail to create command queue!" << endl;
        exit(1);
    }
     
}

bool checkCLGL(){
    if( IsCLExtensionSupported( "cl_khr_gl_sharing" ) )
    {
        fprintf( stderr, "cl_khr_gl_sharing is supported.\n" );
    }
    else
    {
        fprintf( stderr, "cl_khr_gl_sharing is not supported -- sorry.\n" );
        return false;
    }
    return true;
}



bool
IsCLExtensionSupported( const char *extension )
{
    // see if the extension is bogus:
    if( extension == NULL || extension[0] == '\0' )
        return false;
    char * where = (char *) strchr( extension, ' ' );
    if( where != NULL )
        return false;
    // get the full list of extensions:
    size_t extensionSize;
    clGetDeviceInfo( device, CL_DEVICE_EXTENSIONS, 0, NULL, &extensionSize );
    char *extensions = new char [ extensionSize ];
    clGetDeviceInfo( device, CL_DEVICE_EXTENSIONS, extensionSize, extensions, NULL );
    for( char * start = extensions ; ; )
    {
        where = (char *) strstr( (const char *) start, extension );
        if( where == 0 )
        {
            delete [ ] extensions;
            return false;
    }
    char * terminator = where + strlen(extension); // points to what should be the separator
    if( *terminator == ' ' || *terminator == '\0' || *terminator == '\r' || *terminator == '\n' )
    {
        delete [ ] extensions;
        return true;
    }
    start = terminator;
    }
}


void initBuffer(){
    
    // input data vertex
    cerr << "Creating input data buffer...VERTEX" << endl;
    // fill buffer
    glBindBuffer( GL_ARRAY_BUFFER , Vbuf);
    glBufferData( GL_ARRAY_BUFFER, worldBytes, world,  GL_STATIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER , 0);
    
    
    // bind to OpenGL
    vert_in = clCreateFromGLBuffer( ctx, 0, Vbuf, &clErrno );
    if(clErrno == CL_SUCCESS){
        cerr << "Input Memory ID : " << vert_in << endl;
    }
    else{
        cerr << "Fail to create input data buffer! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    
    
    // input data dead
    cerr << "Creating input data buffer...DEAD" << endl;
    dead_in = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, // create read-only and copy data into it
        worldSize * sizeof(GLbyte), // memory size
        dead, // host pointer
        &clErrno
      );

    if(clErrno == CL_SUCCESS){
        cerr << "Input Memory ID : " << dead_in << endl;
    }
    else{
        cerr << "Fail to create command queue! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    
    // output data : color
    cerr << "Creating output data buffer...COLOR" << endl;
    // fill buffer
    glBindBuffer( GL_ARRAY_BUFFER , Cbuf);
    glBufferData( GL_ARRAY_BUFFER, worldBytes, color,  GL_STATIC_DRAW );
    glBindBuffer( GL_ARRAY_BUFFER , 0);
    
    // bind to OpenGL
    
    color_out = clCreateFromGLBuffer( ctx, 0, Cbuf, &clErrno );
    if(clErrno == CL_SUCCESS){
        cerr << "Output Memory ID : " << color_out << endl;
    }
    else{
        cerr << "Fail to create output data buffer! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    
    

    // output data dead
    cerr << "Creating output data buffer...DEAD" << endl;
    dead_out = clCreateBuffer(
        ctx,
        CL_MEM_READ_WRITE , // create read-write buffer
        worldSize * sizeof(GLbyte), // memory size
        NULL, // host pointer
        &clErrno
      );

    if(clErrno == CL_SUCCESS){
        cerr << "Output Memory ID : " << dead_out << endl;
    }
    else{
        cerr << "Fail to create output DEAD buffer ! Errno : " <<  clErrno <<endl;
        exit(1);
    }





}

void loadCLProgram(){
    // create program
    cerr << "Loading CL program source file..." << endl;
    string prog;
    
    // read cl files
    ifstream fin("GameOfLife.cl.cpp");
    string line;
    while(getline(fin,line)){
        prog += line;
        prog += "\n";
    }
    
    // create cl program
    size_t len = prog.size();
    const char* strPtr = prog.c_str();
    clProgram = clCreateProgramWithSource(
        ctx,
        1, // string count
        &strPtr,
        &len,
        &clErrno
      );
    if(clErrno == CL_SUCCESS){
        cerr << "CL Program ID : " << clProgram << endl;
    }
    else{
        cerr << "Fail to create CL program! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    // compile the source
    clErrno = clBuildProgram(
        clProgram,
        1, // # of device
        &device,
        NULL, // no options,
        NULL, // no callback
        NULL // no user data
      );
    
    if(clErrno == CL_SUCCESS){
        cerr << "Program Build Success!" << endl;
    }
    else{
        cerr << "Program Buld Fail! Errno:  " << clErrno <<endl;
        char buf[BUF_SIZE];
        size_t ret_size;
        memset(buf,0x0,sizeof(char)*BUF_SIZE);
        clErrno = clGetProgramBuildInfo(
            clProgram,
            device,
            CL_PROGRAM_BUILD_LOG,
            sizeof(char)*BUF_SIZE,
            buf,
            &ret_size
          );
        cerr << "Error Message :  "  <<endl;
        if(ret_size >= BUF_SIZE){
            cerr << "Error buffer not enough for CL build log" << endl;
        }
        else{
            cerr << buf << endl;
        }
        exit(1);
    }

}

void setKernel(){
    /* ***** Compute ***** */
    // create kernel
    cerr << "Creating CL Compute Kernel..." << endl;
    clKernelCompute = clCreateKernel(
        clProgram,
        "GenerateNextGeneration",
        &clErrno
      );
    if(clErrno == CL_SUCCESS){
        cerr << "CL Kernel ID : " << clKernelCompute  << endl;
    }
    else{
        cerr << "Fail to create CL Kernel! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    // set kernel argment
    cerr << "Setting Kernel Argument..." << endl;
    bool result = true;
    result = result && CL_SUCCESS == clSetKernelArg(clKernelCompute,0,sizeof(cl_mem),&dead_in); // kernel, arg_index , data_size, data_value
    result = result && CL_SUCCESS == clSetKernelArg(clKernelCompute,1,sizeof(cl_mem),&dead_out); // kernel, arg_index , data_size, data_value
    result = result && CL_SUCCESS == clSetKernelArg(clKernelCompute,2,sizeof(unsigned int),&WORLD_HEIGHT); // kernel, arg_index , data_size, data_value
    result = result && CL_SUCCESS == clSetKernelArg(clKernelCompute,3,sizeof(unsigned int),&WORLD_WIDTH); // kernel, arg_index , data_size, data_value
    if(!result){
        cerr << "Set kernel argment fail!" << endl;
        exit(1);
    }
    else{
        cerr << "Set kernel argment success!" << endl;
    }


    /* ***** Migrate ***** */
    // create kernel
    cerr << "Creating CL Migrate Kernel..." << endl;
    clKernelMigrate = clCreateKernel(
        clProgram,
        "MigrateToNext",
        &clErrno
      );
    if(clErrno == CL_SUCCESS){
        cerr << "CL Kernel ID : " << clKernelMigrate  << endl;
    }
    else{
        cerr << "Fail to create CL Kernel! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    // set kernel argment
    cerr << "Setting Kernel Argument..." << endl;
    result = true;
    result = result && CL_SUCCESS == clSetKernelArg(clKernelMigrate,0,sizeof(cl_mem),&dead_in); // kernel, arg_index , data_size, data_value
    result = result && CL_SUCCESS == clSetKernelArg(clKernelMigrate,1,sizeof(cl_mem),&dead_out); // kernel, arg_index , data_size, data_value
    result = result && CL_SUCCESS == clSetKernelArg(clKernelMigrate,2,sizeof(cl_mem),&color_out); // kernel, arg_index , data_size, data_value
    result = result && CL_SUCCESS == clSetKernelArg(clKernelMigrate,3,sizeof(unsigned int),&WORLD_HEIGHT); // kernel, arg_index , data_size, data_value
    result = result && CL_SUCCESS == clSetKernelArg(clKernelMigrate,4,sizeof(unsigned int),&WORLD_WIDTH); // kernel, arg_index , data_size, data_value
    if(!result){
        cerr << "Set kernel argment fail!" << endl;
        exit(1);
    }
    else{
        cerr << "Set kernel argment success!" << endl;
    }
}


void runKernel(){
    // get GL buffer
    
    cl_event ev_acquire;
    clErrno = clEnqueueAcquireGLObjects( cmdQue, 1, &color_out, 0, NULL, &ev_acquire );
    if(clErrno == CL_SUCCESS){
        //cerr << "Acquire GL object Success!" << endl;
    }
    else{
        cerr << "Fail to acquire GL object! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    
    

    // enqueuing kernel
    // global_size
    
    size_t globalSize[2] = {WORLD_HEIGHT,WORLD_WIDTH};
    size_t localSize[2] = {16,16};
    cl_event ev_compute;
    clErrno = clEnqueueNDRangeKernel(
        cmdQue,
        clKernelCompute,
        2, // work dim
        NULL, // work offset
        globalSize,
        localSize, // local size
        0, // # of wait for evnet 
        NULL, // event list
        &ev_compute // associated event
      );
    if(clErrno == CL_SUCCESS){
        //cerr << "CL Kernel Enqueue Success!" << endl;
    }
    else{
        cerr << "Fail to enqueue kernel Compute! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    
    // migrate
    

    cl_event ev_migrate;
    
    clErrno = clEnqueueNDRangeKernel(
        cmdQue,
        clKernelMigrate,
        2, // work dim
        NULL, // work offset
        globalSize,
        localSize, // local size
        0, // # of wait for evnet 
        NULL, // event list
        &ev_migrate // associated event
      );
    if(clErrno == CL_SUCCESS){
        //cerr << "CL Kernel Enqueue Success!" << endl;
    }
    else{
        cerr << "Fail to enqueue kernel Migrate! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    
    // release
    
    cl_event ev_release;
    clErrno = clEnqueueReleaseGLObjects( cmdQue, 1, &color_out, 0, NULL, &ev_release );
    if(clErrno == CL_SUCCESS){
        //cerr << "Release GL object Success!" << endl;
    }
    else{
        cerr << "Fail to release GL object! Errno : " <<  clErrno <<endl;
        exit(1);
    }
    

    
}


