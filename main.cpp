//#define GLFW_INCLUDE_GLCOREARB
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string>

#include "utils.h"

#define GL(line) do { line; assert(glGetError() == GL_NO_ERROR); } while(0)
#define GLSL(str) (const char*)"#version 120\n" #str

// Sky Shaders

const char* skyVertShader = GLSL(
    varying vec3 var_pos;
varying vec3 fsun;
uniform mat4 P;
uniform mat4 RX;
uniform mat4 RY;
uniform float time = 0.0;
uniform float aspectRatio = 1.0;

void main() {
    gl_Position = gl_Vertex;
    var_pos = ( RY * RX * vec4( gl_Position.xy, -1, 1 ) ).xyz;
    var_pos.y /= aspectRatio;
    fsun = vec3( 0.0, sin( time * 0.01 ), -cos( time * 0.01 ) );
}
);

const char* skyFragShader = GLSL(
    varying vec3 var_pos;
varying vec3 fsun;

uniform float time = 0.0;
uniform float cirrus = 0.4;
uniform float cumulus = 0.8;

const float Br = 0.0025;
const float Bm = 0.0003;
const float g = 0.9800;
const vec3 nitrogen = vec3( 0.650, 0.570, 0.475 );
const vec3 Kr = Br / pow( nitrogen, vec3( 4.0 ) );
const vec3 Km = Bm / pow( nitrogen, vec3( 0.84 ) );

float hash( float n ) {
    return fract( sin( n ) * 43758.5453123 );
}

float noise( vec3 x ) {
    vec3 f = fract( x );
    float n = dot( floor( x ), vec3( 1.0, 157.0, 113.0 ) );
    return mix( mix( mix( hash( n + 0.0 ), hash( n + 1.0 ), f.x ),
        mix( hash( n + 157.0 ), hash( n + 158.0 ), f.x ), f.y ),
        mix( mix( hash( n + 113.0 ), hash( n + 114.0 ), f.x ),
            mix( hash( n + 270.0 ), hash( n + 271.0 ), f.x ), f.y ), f.z );
}

const mat3 m = mat3( 0.0, 1.60, 1.20, -1.6, 0.72, -0.96, -1.2, -0.96, 1.28 );
float fbm( vec3 p ) {
    float f = 0.0;
    f += noise( p ) / 2; p = m * p * 1.1;
    f += noise( p ) / 4; p = m * p * 1.2;
    f += noise( p ) / 6; p = m * p * 1.3;
    f += noise( p ) / 12; p = m * p * 1.4;
    f += noise( p ) / 24;
    return f;
}

void main() {
    vec4 color = vec4( 0 );
    if ( var_pos.y < 0 ) {
        gl_FragColor = vec4( 0.3 );
        return;
    }

    vec3 pos = normalize( var_pos );

    if ( dot( pos, fsun ) > 0.997 ) {
        gl_FragColor = vec4( 1 - dot( pos, fsun ) * gl_FragCoord );
        return;
    }

    if ( fract( pos.y * 10 ) < 0.05 ) {
        gl_FragColor = vec4( 0, 1, 0, 0 );
        return;
    }

    if ( fract( abs( pos.z ) * 10 ) < 0.03 ) {
        gl_FragColor = vec4( 0, 0, 1, 0 );
        return;
    }

    // Atmosphere Scattering
    float mu = dot( normalize( pos ), normalize( fsun ) );
    //vec3 extinction = mix( exp( -exp( -( ( pos.y + fsun.y * 4.0 ) * ( exp( -pos.y * 16.0 ) + 0.1 ) / 80.0 ) / Br ) * ( exp( -pos.y * 16.0 ) + 0.1 ) * Kr / Br ) * exp( -pos.y * exp( -pos.y * 8.0 ) * 4.0 ) * exp( -pos.y * 2.0 ) * 4.0, vec3( 1.0 - exp( fsun.y ) ) * 0.2, -fsun.y * 0.2 + 0.5 );
    float ext1 = 0.5 - pos.y;
    ext1 = pos.y;
    //ext1 -= max( 0.2 - fsun.y, 0 ) * ( 1 - dot( pos, fsun ) );
    vec3 extinction = vec3( ext1 );
    color.rgb = 3.0 / ( 8.0 * 3.14 ) * ( 1.0 + mu * mu ) * ( Kr + Km * ( 1.0 - g * g ) / ( 2.0 + g * g ) / pow( 1.0 + g * g - 2.0 * g * mu, 1.5 ) ) / ( Br + Bm ) * extinction;

    // Cirrus Clouds
    float density = smoothstep( 1.0 - cirrus, 1.0, fbm( pos.xyz / pos.y * 2.0 + time * 0.05 ) ) * 0.3;
    //color.rgb = mix(color.rgb, extinction * 4.0, density * max(pos.y, 0.0));

    // Cumulus Clouds
    for ( int i = 0; i < 3; i++ ) {
        float density = smoothstep( 1.0 - cumulus, 1.0, fbm( ( 0.7 + float( i ) * 0.01 ) * pos.xyz / pos.y + time * 0.3 ) );
        //color.rgb = mix(color.rgb, extinction * density * 5.0, min(density, 1.0) * max(pos.y, 0.0));
    }

    // Dithering Noise
    color.rgb += noise( pos * 1000 ) * 0.01;
    gl_FragColor = color;
    gl_FragColor.rgb = extinction;
}
);

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
typedef struct { entity* entities; unsigned int entity_count; gamestate state; } scene;

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

scene makeScene() {
    scene s = { .entities = 0, .entity_count = 0, .state = {.x = 0.0f, .y = 0, .z = 0, .r = 0, .r2 = 0.0f } };
    return s;
}

void renderScene( scene s, int w, int h ) {
    matrix p = getProjectionMatrix( w, h );
    matrix rx = getRotationMatrix( 0, s.state.r2 );
    matrix ry = getRotationMatrix( 1, s.state.r );
    for ( unsigned int i = 0; i < s.entity_count; i++ )
        renderEntity( s.entities[i], p, rx, ry, (float) glfwGetTime() * 3e0f - 0e1f, (float) w / h );
}

void deleteScene( scene s ) {
    for ( unsigned int i = 0; i < s.entity_count; i++ )
        deleteEntity( s.entities[i] );
    free( s.entities );
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

    gladLoadGL();

    glEnable( GL_DEPTH_TEST );
    glDepthFunc( GL_LESS );
    glEnable( GL_CULL_FACE );
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

    scene s = makeScene();
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

        if ( glfwGetKey( window, GLFW_KEY_ESCAPE ) )
            glfwSetWindowShouldClose( window, 1 );

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