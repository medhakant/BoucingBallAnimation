// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <iostream>
// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream> 
#include <chrono> 
using namespace std::chrono; 
using namespace glm;

#include "shader.h"
#include "texture.h"
#include "objloader.h"

#define WIDTH 1000
#define HEIGHT 1000
#define INIT_HEIGHT 5
#define INIT_VELOCITY 0
float theta = M_PI_2f32,phi=0;
float radius = 9;
float height = INIT_HEIGHT;
float velocity = INIT_VELOCITY;
float coeff_res = 0.05f;
float gravity = -9.8;
int frames_rendered = 0;
std::vector<vec3> translation(2);
bool lmouse_button = false;
bool shift_button = false;
int selection = -1;
glm::mat4 ProjectionMatrix;
glm::mat4 ViewMatrix;
glm::vec3 camera_worldposition;

static void error_callback(int error, const char* description){
    fprintf(stderr, "Error: %s\n", description);
}

//Handle the inputs and perform the tranformation at each frame
void computeInputs(){
	static bool pressed_down = false;
	static double lastTime;
	static double lastX;
	static double lastY;
	if(!lmouse_button){
		pressed_down = false;
	}else{
		if(!pressed_down){
			pressed_down = true;
			lastTime = glfwGetTime();
			glfwGetCursorPos(window, &lastX, &lastY);
		}
	}
	
	if(lmouse_button){
		float mouseSpeed = 0.005f;
		// Compute time difference between current and last frame
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastTime);

		// Get mouse position
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		// Reset mouse position for next frame
		// glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);

		if(selection==-1){
			if(!shift_button){
				// Compute new orientation
				float verticalAngle = phi;
				theta -= mouseSpeed * float(lastX - xpos );
				verticalAngle   -= mouseSpeed * float(lastY - ypos );
				if(verticalAngle>-1*M_PI_2f32 && verticalAngle<M_PI_2f32){
					phi = verticalAngle;
				}
			}else{
				radius -= float(lastY - ypos)*0.05;
			}
		}

		// For the next frame, the "last time" will be "now"
		lastTime = currentTime;
		lastX = xpos;
		lastY = ypos;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS){
		lmouse_button = true;
	}else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE){
		lmouse_button = false;
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
   	if (key == GLFW_KEY_LEFT_SHIFT && action == GLFW_PRESS){
	   	shift_button = true;
   	}else if(key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE){
		shift_button = false;
	}else if(key == GLFW_KEY_R && action == GLFW_PRESS){
		height = INIT_HEIGHT;
		velocity = INIT_VELOCITY;
	}
}

void initPosition(){
	translation[0]= vec3(0.0f, -2.0f, 0.0f);
	translation[1]= vec3(0.0f, 0.5f, 0.0f)+vec3(0.0f,height,0.0f);
}

void updatePosition(float dtime){
	if((abs(translation[1].y - 0.5f) < 0.05 || translation[1].y <= 0.5) && frames_rendered > 1){
		velocity = velocity*(coeff_res-1);
		frames_rendered = 0;
	}
	frames_rendered++;
	height += velocity*dtime + .5*gravity*pow(dtime,2);
	velocity += gravity*dtime;
	translation[1].y = height;
}

int main( void )
{
	// Initialise GLFW
	if( !glfwInit() )
	{
		fprintf( stderr, "Failed to initialize GLFW\n" );
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 16);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwSetErrorCallback(error_callback);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow( WIDTH, HEIGHT, "Bouncing Ball", NULL, NULL);
	if( window == NULL ){
		fprintf( stderr, "Failed to open GLFW window.\n" );
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

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
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
	glfwSetKeyCallback(window,key_callback);
    
    // Set the mouse at the center of the screen
    glfwPollEvents();

    // glfwSetCursorPos(window, WIDTH/2, HEIGHT/2);

	// Dark blue background
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders( "shaders/VertexShader.glsl", "shaders/FragmentShader.glsl" );

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	GLuint Texture = loadDDS("textures/rough.dds");
	
	
	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(programID, "myTextureSampler");

	// Store vertices,uvs,normal for all object types
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;

	// Read our plane.obj file
	std::vector<glm::vec3> planevertices;
	std::vector<glm::vec2> planeuvs;
	std::vector<glm::vec3> planenormals;
	bool res = loadOBJ("objects/plane.obj", planevertices, planeuvs, planenormals);
	int planestart = 0;
	int planesize = planevertices.size();
	for(int i=0;i<planesize;i++){
		vertices.push_back(planevertices[i]);
		uvs.push_back(planeuvs[i]);
		normals.push_back(planenormals[i]);
	}
	// Read our sphere.obj file
	std::vector<glm::vec3> spherevertices;
	std::vector<glm::vec2> sphereuvs;
	std::vector<glm::vec3> spherenormals;
	res = loadOBJ("objects/sphere.obj", spherevertices, sphereuvs, spherenormals);
	int spherestart = planesize;
	int spheresize = spherevertices.size();
	for(int i=0;i<spheresize;i++){
		vertices.push_back(spherevertices[i]);
		uvs.push_back(sphereuvs[i]);
		normals.push_back(spherenormals[i]);
	}

	// Load it into a VBO
	GLuint vertexbuffer;
	GLuint uvbuffer;
	GLuint normalbuffer;

	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, vertices.size()* sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
	
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], GL_STATIC_DRAW);

	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");
	GLuint ColorID = glGetUniformLocation(programID, "Color");
	GLuint ObjectID = glGetUniformLocation(programID,"ID");
	GLuint SpecularID = glGetUniformLocation(programID,"Specular_component");
	GLuint DiffuseID = glGetUniformLocation(programID,"Diffuse_component");


	initPosition();

	auto start = high_resolution_clock::now();
	do{
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Use our shader
		glUseProgram(programID);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			0,                  // attribute
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// 2nd attribute buffer : UVs
		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(
			1,                                // attribute
			2,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);

		// 3rd attribute buffer : normals
		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(
			2,                                // attribute
			3,                                // size
			GL_FLOAT,                         // type
			GL_FALSE,                         // normalized?
			0,                                // stride
			(void*)0                          // array buffer offset
		);


		camera_worldposition = glm::vec3(radius*cos(theta)*cos(phi),radius*sin(phi),radius*sin(theta)*cos(phi));
		// Compute the MVP matrix
		ProjectionMatrix = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 100.0f);
		ViewMatrix = glm::lookAt(
								camera_worldposition, // Camera 
								glm::vec3(0,0,0), // and looks at the origin
								glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
						);
		

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) && !lmouse_button){

			selection =-1;

		}

		// Dark blue background
		glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
		//Re-clear the screen for real rendering
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glm::vec3 lightPos = vec3(5.0f,5.0f,5.0f);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);

		// Bind our texture in Texture Unit 0
		glBindTexture(GL_TEXTURE_2D,Texture);
		glActiveTexture(GL_TEXTURE0);
		// Set our "myTextureSampler" sampler to use Texture Unit 0
		glUniform1i(TextureID, 0);

		// Use our shader
		glUseProgram(programID);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// compute Inputs
		computeInputs();
		// draw the plane

		{
			// glm::mat4 ModelMatrix = glm::mat4(1.0);
			glm::mat4 RotationMatrix = rotate(mat4(1.0f),M_PI_2f32,vec3(0,1,0));
			glm::mat4 TranslationMatrix = translate(mat4(1.0f),translation[0]); // A bit to the right
			glm::mat4 ScalingMatrix = scale(mat4(1.0f), vec3(3.0f, 3.0f, 3.0f));
			glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			// 1.5f, 0.0f, 0.0f

			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			// Send Color and Object ID
			glm::vec3 color = glm::vec3(0,0,1);
			glUniform3f(ColorID,color.r, color.g, color.b);
			glUniform1i(ObjectID,0);

			//Send specular and diffuse values
			glUniform1f(SpecularID,0.0f);
			glUniform1f(DiffuseID,1.0f);

			// Draw the triangles !
			glDrawArrays(GL_TRIANGLES, planestart, planesize );
		}

		// the ball
		{
			// glm::mat4 ModelMatrix = glm::mat4(1.0);
			glm::mat4 RotationMatrix = rotate(mat4(1.0f),M_PI_2f32,vec3(0,1,0));
			vec3 position = translation[0]+translation[1];
			glm::mat4 TranslationMatrix = translate(mat4(1.0f),position); // A bit to the right
			glm::mat4 ScalingMatrix = scale(mat4(1.0f), vec3(0.5f, 0.5f, 0.5f));
			glm::mat4 ModelMatrix = TranslationMatrix * RotationMatrix * ScalingMatrix;
			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
			// 1.5f, 0.0f, 0.0f

			// Send our transformation to the currently bound shader, 
			// in the "MVP" uniform
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
			glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

			// Send Color and Object ID
			glm::vec3 color = glm::vec3(1,0,0);
			glUniform3f(ColorID,color.r, color.g, color.b);
			glUniform1i(ObjectID,1);

			//Send specular and diffuse values
			glUniform1f(SpecularID,0.35f);
			glUniform1f(DiffuseID,0.65f);

			// Draw the triangles !
			glDrawArrays(GL_TRIANGLES, spherestart, spheresize );
		}
		

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();
		auto stop = high_resolution_clock::now();
    	float duration = duration_cast<milliseconds>(stop - start).count();
		start = stop;
		updatePosition(duration/1000.0f);
	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

