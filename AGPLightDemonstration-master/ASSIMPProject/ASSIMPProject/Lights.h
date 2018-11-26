#pragma once

// Include GLEW
#define GLEW_STATIC
#include <GL/glew.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;


struct DirLight{

GLfloat direction[3];
GLfloat ambient[3];
GLfloat diffuse[3];
GLfloat specular[3];

};

struct SpotLight
{

	float cutOff;
	float outerCutOff;  // values to determine the spotlight size

	float constant;
	float linear;
	float quadratic;  // attenuation values

	GLfloat ambient[3];
	GLfloat diffuse[3];
	GLfloat specular[3]; // light values
	GLfloat position[3];

};

struct PointLight
{
	float constant;
	float linear;
	float quadratic;  // attenuation values

	GLfloat ambient[3];
	GLfloat diffuse[3];
	GLfloat specular[3]; // light values
	GLfloat position[3];

};

