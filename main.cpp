//#define GLFW_INCLUDE_GLCOREARB
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string>

#include "utils.h"
#include "glsl.h"

// OpenGL Helpers

void glAssert( unsigned int obj, GLenum statusType, void ( APIENTRY* ivFun )( GLuint, GLenum, GLint* ),
    void ( APIENTRY* infoLogFun )( GLuint, GLsizei, GLsizei*, GLchar* ) ) {
    GLint statusCode = GL_FALSE;
    ivFun( obj, statusType, &statusCode );
    if ( statusCode == GL_TRUE ) {
        return;
    }

    GLint length = 0;
    ivFun( obj, GL_INFO_LOG_LENGTH, &length );

    char* error_log = (char*) alloca( length );
    infoLogFun( obj, length, &length, &error_log[0] );

    fprintf( stderr, "%s\n", error_log );
    //free( error_log );
    exit( 0 );
}

// Structures

typedef struct { float x, y, z, r, r2; double px, py; } gamestate;
typedef struct {
    unsigned int program;
    int P, RX, RY, M, aspectRatio, time;
} entity;
typedef struct { 
    entity* entities; 
    unsigned int entity_count; 
    gamestate state; 
    bool paused;
} scene;

char* title = "Loading...";

unsigned int makeShader( const char* code, GLenum shaderType ) {
    unsigned int shader = glCreateShader( shaderType );
    glShaderSource( shader, 1, &code, NULL );
    glCompileShader( shader );

    glAssert( shader, GL_COMPILE_STATUS, glGetShaderiv, glGetShaderInfoLog );

    return shader;
}

unsigned int makeProgram( const char* vertexShaderSource, const char* fragmentShaderSource ) {
    unsigned int vertexShader = vertexShaderSource ? makeShader( vertexShaderSource, GL_VERTEX_SHADER ) : 0;
    unsigned int fragmentShader = fragmentShaderSource ? makeShader( fragmentShaderSource, GL_FRAGMENT_SHADER ) : 0;

    unsigned int program = glCreateProgram();
    if ( vertexShader ) { glAttachShader( program, vertexShader ); }
    if ( fragmentShader ) { glAttachShader( program, fragmentShader ); }
    glLinkProgram( program );

    glAssert( program, GL_LINK_STATUS, glGetProgramiv, glGetProgramInfoLog );

    if ( vertexShader ) { glDetachShader( program, vertexShader ); }
    if ( vertexShader ) { glDeleteShader( vertexShader ); }
    if ( fragmentShader ) { glDetachShader( program, fragmentShader ); }
    if ( fragmentShader ) { glDeleteShader( fragmentShader ); }

    return program;
}

// Entities

entity makeEntity( scene* s, const char* vs, const char* fs ) {
    entity e;

    // Load Program
    e.program = makeProgram( vs, fs );
    e.P = glGetUniformLocation( e.program, "P" );
    e.RX = glGetUniformLocation( e.program, "RX" );
    e.RY = glGetUniformLocation( e.program, "RY" );
    e.M = glGetUniformLocation( e.program, "M" );
    e.aspectRatio = glGetUniformLocation( e.program, "aspectRatio" );
    e.time = glGetUniformLocation( e.program, "time" );

    s->entities = (entity*) realloc( s->entities, ++s->entity_count * sizeof( entity ) );
    memcpy( &s->entities[s->entity_count - 1], &e, sizeof( entity ) );

    return e;
}

void renderEntity( entity e, matrix P, matrix RX, matrix RY, float time, float aspectRatio ) {
    glUseProgram( e.program );
    glUniformMatrix4fv( e.P, 1, GL_FALSE, P.m );
    glUniformMatrix4fv( e.RX, 1, GL_FALSE, RX.m );
    glUniformMatrix4fv( e.RY, 1, GL_FALSE, RY.m );
    glUniform1f( e.time, time );
    glUniform1f( e.aspectRatio, aspectRatio );

    glBegin( GL_TRIANGLE_STRIP );
    glVertex2f( -1.0, 1.0 );
    glVertex2f( -1.0, -1.0 );
    glVertex2f( 1.0, 1.0 );
    glVertex2f( 1.0, -1.0 );
    glEnd();
}

void deleteEntity( entity e ) {
    glDeleteProgram( e.program );
}

// Scene

scene s = { .entities = 0, .entity_count = 0, .state = {.x = 0.0f, .y = 0, .z = 0, .r = 0, .r2 = 0.0f } };

void renderScene( scene s, int w, int h ) {
    matrix p = getProjectionMatrix( w, h );
    matrix rx = getRotationMatrix( 0, s.state.r2 );
    matrix ry = getRotationMatrix( 1, s.state.r );
    static double lastTime = 0, time = -1e1f;
    if ( !s.paused )
        time += ( glfwGetTime() - lastTime ) * 3e0f;
    lastTime = glfwGetTime();
    for ( unsigned int i = 0; i < s.entity_count; i++ )
        renderEntity( s.entities[i], p, rx, ry, (float)time, (float) w / h );
}

void deleteScene( scene s ) {
    for ( unsigned int i = 0; i < s.entity_count; i++ )
        deleteEntity( s.entities[i] );
    free( s.entities );
}

void keyCallback( GLFWwindow* window, int key, int scancode, int action, int mods ) {
    if ( action != GLFW_PRESS )
        return;
    if ( key == GLFW_KEY_ESCAPE )
        glfwSetWindowShouldClose( window, 1 );
    if ( key ==  GLFW_KEY_SPACE )
        s.paused = !s.paused;
}

// Main Loop

int main() {
    glfwInit();
    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 2 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
    glfwWindowHint( GLFW_MAXIMIZED, GLFW_TRUE );
    GLFWwindow* window = glfwCreateWindow( 800, 600, "Test", NULL, NULL );
    glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
    glfwMakeContextCurrent( window );

    glfwSetKeyCallback( window, keyCallback );

    gladLoadGL();

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glEnable( GL_CULL_FACE );
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

    makeEntity( &s, skyVertShader, skyFragShader );

    glfwGetCursorPos( window, &s.state.px, &s.state.py );
    while ( !glfwWindowShouldClose( window ) ) {
        // Move Cursor
        double mx, my;
        glfwGetCursorPos( window, &mx, &my );
        s.state.r -= -(float) ( mx - s.state.px ) * 2e-3f;
        s.state.r2 -= (float) ( my - s.state.py ) * 2e-3f;
        s.state.px = (float) mx;
        s.state.py = (float) my;

        // Clear Framebuffer
        glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

        // Render the Scene
        int w, h;
        glfwGetWindowSize( window, &w, &h );
        renderScene( s, w, h );
        glfwSetWindowTitle( window, title );

        // Swap
        glfwSwapBuffers( window );
        glfwPollEvents();
    }

    deleteScene( s );

    glfwTerminate();
    return 0;
}