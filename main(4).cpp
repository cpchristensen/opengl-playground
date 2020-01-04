#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

// GLM Stuff
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define WINDOW_TITLE "Modern OpenGL"

#define RADIANS_TO_DEGREES 57.29578

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

using namespace std;

GLint shaderProgram, WindowWidth = 800, WindowHeight = 600;
// Buffer and Array objects
GLuint VBO, VAO, EBO, texture;

/*
 * User defined function prototypes.
 * Initializes basic elements of program.
 * Provides functions for drawing to screen.
 */
void UResizeWindow (int, int);
void URenderGraphics (void);
void UCreateShader (void);
void UCreateBuffers (void);

/* Mouse callbacks */
void UMouseClick (int button, int state, int x, int y);
void UMouseMove (int x, int y);
void UMousePressedMove (int x, int y);

/* Keeps track of where the camera is looking and how fast it moves*/
GLfloat cameraSpeed = 0.01f;
GLchar currentKey;
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -5.0f);
glm::vec3 front = glm::vec3(0.0f, 0.0f, -5.0f);

/* Maintaining the direction of the both the camera and the cube. */
GLfloat lastMouseX = 400, lastMouseY = 300;
GLfloat mouseXOffset, mouseYOffset, object_yaw = 0.0f, object_pitch = 0.0f, camera_yaw = 90.0f, camera_pitch = 0.0f;
GLfloat sensitivity = 0.05f;
bool mouseDetected = true;
/* Used to track the key combination ALT+CLICK*/
bool leftIsPressed = false;
bool rightIsPressed = false;
bool altIsPressed = false;

const char* vertexShaderSource = 1 + R"GLSL(
	#version 330 core
	layout(location=0) in vec3 position;
	layout(location=1) in vec3 color;
	out vec3 mobileColor;
	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(position, 1.0f);
		mobileColor = color;
	}
)GLSL";

const char* fragmentShaderSource = 1 + R"GLSL(
	#version 330 core
	in vec3 mobileColor;
	out vec4 gpuColor;

	void main() {
		gpuColor = vec4(mobileColor, 1.0);
	}
)GLSL";


int main (int argc, char** argv) {
	GLenum GlewInitResult;
	// Initializes window with size.
	glutInit(&argc, argv);
	// Initializes memory display buffer.
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WindowWidth, WindowHeight);
	// Sets window title and creates window.
	glutCreateWindow(WINDOW_TITLE);
	// Binds user defined functions for reshaping and displaying windows.
	glutReshapeFunc(UResizeWindow);

	// Initializes glew and checks for errors.
	GlewInitResult = glewInit();
	if (GlewInitResult != GLEW_OK) {
		fprintf(stderr, "ERROR: %s\n", glewGetErrorString(GlewInitResult));
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));

	// Creates shader program.
	UCreateShader();
	// Creates Vertex Buffer Object
	UCreateBuffers();
    
	// Uses shader program.
    glUseProgram(shaderProgram);
    
	// Sets background color.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glutDisplayFunc(URenderGraphics);
	
	/* Sets mouse callbacks.*/
	glutMouseFunc(UMouseClick);
	glutMotionFunc(UMousePressedMove);
	
	glutMainLoop();

    // Garbage Collection
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

	return 0;
}

void UResizeWindow (int Width, int Height) {
    WindowWidth = Width;
    WindowHeight = Height;
	glViewport(0, 0, Width, Height);
}

void URenderGraphics (void) {
	
	// Enables the z axis.
	glEnable(GL_DEPTH_TEST);
	// Clear screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Activation VBO before manipulating it.
    glBindVertexArray(VAO);
	/* Moves camera based on key press. */
	
	CameraForwardZ = front;

    // Transforms object.
    glm::mat4 model(1.0);
    // Centers object in viewport.
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, -5.0f));
    // Rotates 45 degrees on the xyz axis.
    //model = glm::rotate(model, 45.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	
	model = glm::rotate(model, (float) (glm::radians(object_pitch) * RADIANS_TO_DEGREES), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, (float) (glm::radians(object_yaw) * RADIANS_TO_DEGREES), glm::vec3(0.0f, 1.0f, 0.0f));
	
    // Scales to double size in xyz.
    model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
	
	// Transforms camera.
	glm::mat4 view(1.0);
	// Moves .5 units on X and-5 units on Z.
	// view = glm::translate(view, glm::vec3(0.5f, 0.0f, -5.0f));
	view = glm::lookAt(cameraPosition, cameraPosition + CameraForwardZ, CameraUpY);
	
	//Creates perspective.
	glm::mat4 projection(1.0);
	projection = glm::perspective(45.0f, (GLfloat) WindowWidth / (GLfloat) WindowHeight, 0.1f, 100.0f);
	
	// Sends matrices to shader program.
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glutPostRedisplay();
	
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Deactivate VAO
    glBindVertexArray(0);

	// Flips front and back buffers.
	glutSwapBuffers();
}

void UCreateShader (void) {
	// Create Vertex shader object.
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);
	
	
	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Retrieves shader and frag source code.
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	// Compiles shader source code.
	glCompileShader(fragmentShader);

    // Create shader program object.
	shaderProgram = glCreateProgram();
	// Attach shaders to program.
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);

	// Link program
	glLinkProgram(shaderProgram);

    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void UCreateBuffers (void) {
	// Sets vertex coordinates.
	GLfloat verts[] = {
        // Coordinates      Colors
		
        0.5f, 0.5f, 0.0f,        1.0f, 0.0f, 0.0f,        
        0.5f, -0.5f, 0.0f,       0.0f, 1.0f, 0.0f,        
        -0.5f, -0.5f, 0.0f,      0.0f, 0.0f, 1.0f,   
        -0.5f, 0.5f, 0.0f,       1.0f, 0.0f, 1.0f,
		
		0.5f, -0.5f, -1.0f,      0.5f, 0.5f, 1.0f,        
        0.5f, 0.5f, -1.0f,       1.0f, 1.0f, 0.5f,        
        -0.5f, 0.5f, -1.0f,      0.2f, 0.2f, 0.5f,   
        -0.5f, -0.5f, -1.0f,     1.0f, 0.0f, 1.0f 
	};

    // Index data.
    GLuint indices[] = {
		
        0, 1, 3,    // Triangle 1
        1, 2, 3,     // Triangle 2
		0, 1, 4,     // Triangle 3
		0, 4, 5,     // Triangle 4
		0, 5, 6,     // Triangle 5
		0, 3, 6,     // Triangle 6
		4, 5, 6,     // Triangle 7
		4, 6, 7,     // Triangle 8
		2, 3, 6,     // Triangle 9
		2, 6, 7,     // Triangle 10
		1, 4, 7,     // Triangle 11
		1, 2, 7     // Triangle 12
    };

    // Generate buffer IDs
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
	
	// Activation of VAO before binding.
    glBindVertexArray(VAO);

	// Activates the buffer.
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Sends data to GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    // Activates the buffer.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	// Sends data to GPU
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Strides between vertex coordinates is 6
	GLint vertexStride = sizeof(GLfloat) * 6;

	// Tells GPU how to handle VBO.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertexStride, (GLvoid*) 0);
	glEnableVertexAttribArray(0); // Sets initial position of rgba in buffer.

    // Tells GPU how to handle VBO.
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertexStride, (GLvoid*) (3*sizeof(GLfloat)));
	glEnableVertexAttribArray(1); // Sets initial position of rgba in buffer.

    glBindVertexArray(0);
}

void UMouseClick (int button, int state, int x, int y) {
	/*
	 * Possible button values:
	 * 	GLUT_LEFT_BUTTON
	 *	GLUT_RIGHT_BUTTON
	 *	GLUT_MIDDLE_BUTTON ???
	 * 
	 *  Possible states:
	 * 	GLUT_DOWN
	 * 	GLUT_UP
	 */
	
	/* Used to create a more intuitive looking action. */
	if (state == GLUT_UP) {
		mouseDetected = true;
	}
	
	switch (button) {
		case GLUT_LEFT_BUTTON:
			leftIsPressed = (state == GLUT_DOWN);
			/* Determines if the alt key is also being pressed. */
			altIsPressed = (leftIsPressed && (glutGetModifiers() == GLUT_ACTIVE_ALT));
			break;
		case GLUT_RIGHT_BUTTON:
			rightIsPressed = (state == GLUT_DOWN);
			/* Determines if the alt key is also being pressed. */
			altIsPressed = (rightIsPressed && (glutGetModifiers() == GLUT_ACTIVE_ALT));
			break;
		default:
			break;
	}
	
}
void UMousePressedMove (int x, int y) {
	/* This first portion just updates the state of the mouse.*/
	if (mouseDetected) {
		lastMouseX = x;
		lastMouseY = y;
		mouseDetected = false;
	}

	mouseXOffset = x - lastMouseX;
	mouseYOffset = lastMouseY - y;

	lastMouseX = x;
	lastMouseY = y;

	mouseXOffset *= sensitivity;
	mouseYOffset *= sensitivity;
	
	/* Handles what type of action is being performed. */
	if (altIsPressed) {
		if (leftIsPressed) {
			/* Changes orientation of objet based on mouse movement. */
			object_yaw += mouseXOffset;
			object_pitch += mouseYOffset;
		} else if (rightIsPressed) {
			if (mouseYOffset > 0) {
				cameraPosition += cameraSpeed * CameraForwardZ;
				
			} else {
				cameraPosition -= cameraSpeed * CameraForwardZ;
			}
		}
	}
}