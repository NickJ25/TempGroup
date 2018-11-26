
// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>

//include objects
#include "Lights.h"
#include "Model.h"
#include "Shader.h"
#include "SkyBox.h"
#include "Camera.h"
#include "libpng16\png.h"
#include "PNGProcessor.h"

//rt3d
#include "rt3d.h"
#include "rt3dObjLoader.h"

// Include GLFW
#include <GLFW/glfw3.h>

#if _DEBUG
#pragma comment(linker, "/subsystem:\"console\" /entry:\"WinMainCRTStartup\"")
#endif

using namespace std;

//window 
GLFWwindow * window;

//camera 
Camera camera(glm::vec3(14.0f, -0.75f, 6.0f));

//time keeping
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;


DirLight dirLight = {
	0.5f, 2.0f, 2.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
};

SpotLight light{

	glm::cos(glm::radians(12.5f)),
	glm::cos(glm::radians(17.5f)),

	1.0f, 0.09f, 0.032f,

{ 1.0f, 1.0f, 1.0f },
{ 0.8f, 0.8f, 0.8f },
{ 1.0f, 1.0f, 1.0f }

};

PointLight pointLight{

	1.0f, 0.09f, 0.032f,

{ 1.0f, 1.0f, 1.0f },
{ 0.8f, 0.8f, 0.8f },
{ 1.0f, 1.0f, 1.0f },
{ 1.0f, 3.0f, 13.0f }

};

//normal map functions
bool normalMapped = true;


//glfw values
bool keys[1024];

GLfloat lastX = 400, lastY = 300;

bool firstMouse = true;

int windowWidth = 1024;
int windowHeight = 768;

// bias matrix
glm::mat4 biasMatrix(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 0.5, 0.0,
	0.5, 0.5, 0.5, 1.0
);

//assimp models
Model *model1;
Model *model2;
Model *model3;

//buffer and shader values
GLuint Framebuffer;
GLuint depthTexture;

//shaders
Shader *furProgram;
Shader *shadowShader;
Shader *shader;

// Fur Settings
float furLength = 0.2;
int layers = 30;
int furDensity = 50000;
int furPatternNum = 0;
float furFlowOffset = 0; // For fur animation/ movement.
bool increment = false;

//fur textures
GLuint furTextures[5];


//fog shader variables
int fogSelector = 2;
//0 plane based; 1 range based
int depthFog = 1;

// Mesh Index Count
GLuint meshIndexCount = 0;
GLuint meshObjects[1];

//function prototypes
GLuint LoadTexture(const char *path);
void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow *window, double xPos, double yPos);
void update();

int initglfw()
{

	// Initialise GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Shadow Mapping", NULL, NULL);
	if (window == NULL) {
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// But on MacOS X with a retina screen it'll be 1024*2 and 768*2, so we get the actual framebuffer size:
	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);


	// Set the required callback functions
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseCallback);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited mouvement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	glfwSetCursorPos(window, 1024 / 2, 768 / 2);


	// grey background
	glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);

	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	//glEnable(GL_CULL_FACE);
}

void initModels()
{
	model1 = new Model("res/models/nanosuit.obj");

	model2 = new Model("res/untitled.obj");

}

int initFrameBuffer()
{
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	Framebuffer = 0;
	glGenFramebuffers(1, &Framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);

	// Depth texture
	depthTexture;
	glGenTextures(1, &depthTexture);
	glBindTexture(GL_TEXTURE_2D, depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, 1024, 1024, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	// No color output in the bound framebuffer, only depth.
	glDrawBuffer(GL_NONE);

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

}

void setLights(Shader* shader)
{
	// Use our shader
	glUseProgram(shader->Program);

	//set the directional light 
	glUniform3f(glGetUniformLocation(shader->Program, "dirLight.direction"), dirLight.direction[0], dirLight.direction[1], dirLight.direction[2]);
	glUniform3f(glGetUniformLocation(shader->Program, "dirLight.ambient"), dirLight.ambient[0], dirLight.ambient[1], dirLight.ambient[2]);
	glUniform3f(glGetUniformLocation(shader->Program, "dirLight.diffuse"), dirLight.diffuse[0], dirLight.diffuse[1], dirLight.diffuse[2]);
	glUniform3f(glGetUniformLocation(shader->Program, "dirLight.specular"), dirLight.specular[0], dirLight.specular[1], dirLight.specular[2]);

	//set camera position
	GLint viewPosLoc = glGetUniformLocation(shader->Program, "viewPos");
	glUniform3f(viewPosLoc, camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);

	//spotlight
	glUniform3f(glGetUniformLocation(shader->Program, "light.ambient"), light.ambient[0], light.ambient[1], light.ambient[2]);
	glUniform3f(glGetUniformLocation(shader->Program, "light.diffuse"), light.diffuse[0], light.diffuse[1], light.diffuse[2]);
	glUniform3f(glGetUniformLocation(shader->Program, "light.specular"), light.specular[0], light.specular[1], light.specular[2]);
	glUniform1f(glGetUniformLocation(shader->Program, "light.constant"), light.constant);
	glUniform1f(glGetUniformLocation(shader->Program, "light.linear"), light.linear);
	glUniform1f(glGetUniformLocation(shader->Program, "light.quadratic"), light.quadratic);
	glUniform3f(glGetUniformLocation(shader->Program, "light.position"), camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
	glUniform3f(glGetUniformLocation(shader->Program, "light.direction"), camera.GetFront().x, camera.GetFront().y, camera.GetFront().z);
	glUniform1f(glGetUniformLocation(shader->Program, "light.cutOff"), light.cutOff);
	glUniform1f(glGetUniformLocation(shader->Program, "light.outerCutOff"), light.outerCutOff);

	//spotlight
	glUniform3f(glGetUniformLocation(shader->Program, "pointLight.ambient"), pointLight.ambient[0], pointLight.ambient[1], pointLight.ambient[2]);
	glUniform3f(glGetUniformLocation(shader->Program, "pointLight.diffuse"), pointLight.diffuse[0], pointLight.diffuse[1], pointLight.diffuse[2]);
	glUniform3f(glGetUniformLocation(shader->Program, "pointLight.specular"), pointLight.specular[0], pointLight.specular[1], pointLight.specular[2]);
	glUniform1f(glGetUniformLocation(shader->Program, "pointLight.constant"), pointLight.constant);
	glUniform1f(glGetUniformLocation(shader->Program, "pointLight.linear"), pointLight.linear);
	glUniform1f(glGetUniformLocation(shader->Program, "pointLight.quadratic"), pointLight.quadratic);
	glUniform3f(glGetUniformLocation(shader->Program, "pointLight.position"), pointLight.position[0], pointLight.position[1], pointLight.position[2]);

	glUniform1i(glGetUniformLocation(shader->Program, "normalMapped"), normalMapped);
}

void setContext(bool attach, int windowWidth, int windowHeight)
{
	if(attach == true)
	glBindFramebuffer(GL_FRAMEBUFFER, Framebuffer);
	else
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glViewport(0, 0, windowWidth, windowHeight); // Render on the whole framebuffer, complete from the lower left corner to the upper right
	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK); // Cull back-facing triangles -> draw only front-facing triangles

	// Clear the screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

int main(int argc, char *argv[])
{

	//create a window and initialise GLFW and glew
	initglfw();

	//load models from files
	initModels();

	// generate the shader for the shadow shader
	shadowShader = new Shader("res/shaders/DepthRTT.vertexshader", "res/shaders/DepthRTT.fragmentshader");

	//generate a frame buffer for the shadow map
	initFrameBuffer();

	// generate the shader for the actual rendering
	shader = new Shader("res/shaders/fogMap.vert", "res/shaders/fogMap.frag");
	//shader = new Shader("res/shaders/ShadowMapping.vertexshader", "res/shaders/ShadowMapping.fragmentshader");
	//shader = new Shader("res/shaders/NormalMapping.vert", "res/shaders/NormalMapping.frag");
	// Fur Shader Program
	furProgram = new Shader("res/shaders/furShader.vert", "res/shaders/furShader.frag");

	//set texture uniforms
	GLuint uniformIndex = glGetUniformLocation(furProgram->Program, "textureUnit1");
	glUniform1i(uniformIndex, 1);
	uniformIndex = glGetUniformLocation(furProgram->Program, "textureUnit0");
	glUniform1i(uniformIndex, 0);


	// load fur textures
	furTextures[0] = LoadTexture("res/FloorObject/close-up-concrete-creativity-908286.jpg");
	furTextures[1] = LoadTexture("res/Textures/leopard.bmp");
	furTextures[2] = LoadTexture("res/Textures/giraffe.bmp");
	furTextures[3] = LoadTexture("res/Textures/cow.bmp");
	PNGProcessor pngprocess;
	furTextures[4] = pngprocess.createFurTextures(383832, 128, 20, furDensity, "furPattern.png");

	SkyBox::Instance()->init("res/FogMap");

	//alternative object loader

	vector<GLfloat> verts;
	vector<GLfloat> norms;
	vector<GLfloat> tex_coords;
	vector<GLuint> indices;

	// Prepare Cube model
	rt3d::loadObj("res/models/nanosuit.obj", verts, norms, tex_coords, indices);
	meshIndexCount = indices.size();
	meshObjects[0] = rt3d::createMesh(verts.size() / 3, verts.data(), nullptr, norms.data(), tex_coords.data(), meshIndexCount, indices.data());



	do {
		
		//glDisable(GL_BLEND);
		//glEnable(GL_DEPTH_TEST);

		// Set frame time
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// Check and call events
		glfwPollEvents();
		update();
		
		//set up framebuffer context
		setContext(true, 1024, 1024);

		// Use the shadow shader
		glUseProgram(shadowShader->Program);

		// Compute the MVP matrix from the light's point of view
		glm::mat4 depthProjectionMatrix = glm::ortho<float>(-10, 10, -10, 10, -10, 20);
		glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(0.5, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
		glm::mat4 depthModelMatrix = glm::mat4(1.0);

		depthModelMatrix = glm::scale(depthModelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
		glm::mat4 depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;


		glUniformMatrix4fv(glGetUniformLocation(shadowShader->Program, "depthMVP"), 1, GL_FALSE, glm::value_ptr(depthMVP));
		model1->DrawVMesh(*shadowShader);

		depthModelMatrix = glm::mat4(1.0);
		depthModelMatrix = glm::scale(depthModelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
		depthModelMatrix = glm::translate(depthModelMatrix, glm::vec3(0.0f, -18.0f, 0.0f));

		depthMVP = depthProjectionMatrix * depthViewMatrix * depthModelMatrix;
		glUniformMatrix4fv(glGetUniformLocation(shadowShader->Program, "depthMVP"), 1, GL_FALSE, glm::value_ptr(depthMVP));

		//draw the shadow for model2 (vertices and indices only)
		//model2->DrawVMesh(*shadowShader);
		//disable the framebuffer
		setContext(false, windowWidth, windowHeight);

		//set up all the  lights
		setLights(shader);
		
		// Compute the MVP matrix from keyboard and mouse input
		glm::mat4 ProjectionMatrix = glm::perspective(camera.GetZoom(), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f); ;// getProjectionMatrix();
		glm::mat4 ViewMatrix = camera.GetViewMatrix();

		glm::mat4 ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
	
		glm::mat4 depthBiasMVP = biasMatrix * depthMVP;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "MVP"), 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "M"), 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "V"), 1, GL_FALSE, &ViewMatrix[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "DepthBiasMVP"), 1, GL_FALSE, &depthBiasMVP[0][0]);
		glUniform3f(glGetUniformLocation(shader->Program, "LightInvDirection_worldspace"), dirLight.direction[0],dirLight.direction[1],dirLight.direction[2]);


		//set fog values based on input
		glUniform1i(glGetUniformLocation(shader->Program, "fogSelector"), fogSelector);
		glUniform1i(glGetUniformLocation(shader->Program, "depthFog"), depthFog);

		//set the shadow to the highest possible texture (to avoid overwriting by the model class)
		glActiveTexture(GL_TEXTURE31);
		glBindTexture(GL_TEXTURE_2D, depthTexture);
		glUniform1i(glGetUniformLocation(shader->Program, "shadowMap"), 31);

		// draw the full mesh this time
		model1->DrawDMesh(*shader);


		//reset the model matrix
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f, 0.2f, 0.2f));
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, -18.0f, 0.0f));
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "MVP"), 1, GL_FALSE, glm::value_ptr(MVP));
		glUniformMatrix4fv(glGetUniformLocation(shader->Program, "M"), 1, GL_FALSE, glm::value_ptr(ModelMatrix));

		// draw the second model too
		//model2->DrawDMesh(*shader);
		
		//fur shader part
		// use the fur shader
		//glEnable(GL_DEPTH_TEST);
		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		//glDepthMask(GL_TRUE);

		furProgram->Use();

		//set the projection
		glUniformMatrix4fv(glGetUniformLocation(furProgram->Program, "projection"), 1, GL_FALSE, glm::value_ptr(ProjectionMatrix));

		//reset model matrix
		ModelMatrix = glm::mat4(1.0f);

		//create modelview and apply translation
		glm::mat4 modelview = ModelMatrix * ViewMatrix;
		modelview = glm::translate(modelview, glm::vec3(0.0f, 1.0f, 10.0f));
		modelview = ModelMatrix *ViewMatrix;
		glUniformMatrix4fv(glGetUniformLocation(furProgram->Program, "modelview"), 1, GL_FALSE, glm::value_ptr(modelview));

		// Pass through the total amount of layers
		GLuint uniformIndex = glGetUniformLocation(furProgram->Program, "layers");
		glUniform1f(uniformIndex, (float)layers);
		// Pass through fur length 
		uniformIndex = glGetUniformLocation(furProgram->Program, "furLength");
		glUniform1f(uniformIndex, furLength);
		float num = 1;

		/*glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, furTextures[furPatternNum]);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, furTextures[4]);*/



		for (int i = 0; i < layers; i++) {
			// Pass through currentLayer

			uniformIndex = glGetUniformLocation(furProgram->Program, "currentLayer");
			glUniform1f(uniformIndex, (float)i);

			// Determine the alpha and pass it through via UVScale
			uniformIndex = glGetUniformLocation(furProgram->Program, "UVScale");
			num = num - (1 / (float)layers);
			if (num > 1) num = 1;
			if (num < 0) num = 0;
			glUniform1f(uniformIndex, num);

			// Passthrough fur movement.
			uniformIndex = glGetUniformLocation(furProgram->Program, "furFlowOffset");
			if (furFlowOffset > 0.01) {
				increment = false;
			}
			else if (furFlowOffset < -0.01) {
				increment = true;
			}
			if (increment) furFlowOffset += 0.00001;
			else furFlowOffset -= 0.00001;
			glUniform1f(uniformIndex, furFlowOffset * ((float)i / (float)layers));

		//	rt3d::drawIndexedMesh(meshObjects[0], meshIndexCount, GL_TRIANGLES);
			model1->DrawFurMesh(*furProgram, furTextures[1], furTextures[4]);

		}

		SkyBox::Instance()->draw();
		//glDepthMask(GL_TRUE);
		//swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

		//remove rotation from matrix and apply to the skybox
		glm::mat4 pv = ProjectionMatrix * glm::mat4(glm::mat3(camera.GetViewMatrix()));
		SkyBox::Instance()->update(pv);


	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Cleanup shader
	glDeleteProgram(shader->Program);
	glDeleteProgram(shadowShader->Program);

	// Cleanup framebuffer
	glDeleteFramebuffers(1, &Framebuffer);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

void MouseCallback(GLFWwindow *window, double xPos, double yPos)
{

	if (firstMouse)
	{
		lastX = xPos;
		lastY = yPos;
		firstMouse = false;
	}

	GLfloat xOffset = xPos - lastX;
	GLfloat yOffset = lastY - yPos;  // Reversed since y-coordinates go from bottom to left

	lastX = xPos;
	lastY = yPos;

	camera.ProcessMouseMovement(xOffset, yOffset);
}

void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mode)
{

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			keys[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			keys[key] = false;
		}
	}
}

void update()
{
	
	// Camera controls
	if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])
	{
		camera.ProcessKeyboard(FORWARD, deltaTime);
	}

	if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])
	{
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	}

	if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
	{
		camera.ProcessKeyboard(LEFT, deltaTime);
	}

	if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
	{
		camera.ProcessKeyboard(RIGHT, deltaTime);


	}

	if (keys[GLFW_KEY_R]) normalMapped = true;
	if (keys[GLFW_KEY_F]) normalMapped = false;

	if (keys[GLFW_KEY_Y]) fogSelector = 0;
	if (keys[GLFW_KEY_U]) fogSelector = 1;
	if (keys[GLFW_KEY_I]) fogSelector = 2; //best one

	if (keys[GLFW_KEY_K]) depthFog = 0;
	if (keys[GLFW_KEY_L]) depthFog = 1;  // best one

}


GLuint LoadTexture(const char *path)
{
	//Generate texture ID and load texture data
	string filename = string(path);

	GLuint textureID;
	glGenTextures(1, &textureID);

	int width, height;

	unsigned char *image = SOIL_load_image(filename.c_str(), &width, &height, 0, SOIL_LOAD_RGB);

	// Assign texture to ID
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	SOIL_free_image_data(image);

	return textureID;
}
