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

/* Keyboard callback. */
void UKeyboard (unsigned char key, GLint x, GLint y);

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

/* Keeps track of if user wants ortho or not.*/
bool isOrtho = false;

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
	/* Sets keyboard callback. */
	glutKeyboardFunc(UKeyboard);
	
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
	view = glm::lookAt(cameraPosition, cameraPosition + CameraForwardZ, CameraUpY);
	
	//Creates perspective.
	glm::mat4 projection(1.0);
	if (isOrtho) {
		projection = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, 0.1f, 100.0f);		
	} else {
		projection = glm::perspective(45.0f, (GLfloat) WindowWidth / (GLfloat) WindowHeight, 0.1f, 100.0f);
	}
	
	// Sends matrices to shader program.
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glutPostRedisplay();

	glDrawElements(GL_TRIANGLES, 36 * 8, GL_UNSIGNED_INT, 0);

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
        /* Table-top */
		
        -0.75f, 1.0f,   1.0f,      1.0f, 0.0f,  1.0f,        
        -0.75f, 1.0f,  -1.0f,      1.0f, 0.0f,  1.0f,        
         0.75f, 1.0f,  -1.0f,      1.0f, 0.0f,  1.0f,   
         0.75f, 1.0f,   1.0f,      1.0f, 0.0f,  1.0f,
		
		-0.7f,  0.95f,  0.95f,     1.0f, 0.0f, 1.0f,        
        -0.7f,  0.95f, -0.95f,     1.0f, 0.0f, 1.0f,        
         0.7f,  0.95f, -0.95f,     1.0f, 0.0f, 1.0f,   
         0.7f,  0.95f,  0.95f,     1.0f, 0.0f, 1.0f,
		/* Leg 1 */
		-0.65f,  0.95f,  0.9f,      1.0f, 0.0f, 0.0f,        
        -0.65f,  0.95f,  0.8f,      1.0f, 0.0f, 0.0f,        
        -0.55f,  0.95f,  0.8f,      1.0f, 0.0f, 0.0f,   
        -0.55f,  0.95f,  0.9f,      1.0f, 0.0f, 0.0f,
		
		-0.64f, -1.0f,  0.89f,     1.0f, 0.0f, 0.0f,        
        -0.64f, -1.0f,  0.81f,     1.0f, 0.0f, 0.0f,        
        -0.56f, -1.0f,  0.81f,     1.0f, 0.0f, 0.0f,   
        -0.56f, -1.0f,  0.89f,     1.0f, 0.0f, 0.0f,
		
		/* Leg 2 */
		-0.65f,  0.95f, -0.8f,      1.0f, 0.0f, 0.0f,        
        -0.65f,  0.95f, -0.9f,      1.0f, 0.0f, 0.0f,        
        -0.55f,  0.95f, -0.9f,      1.0f, 0.0f, 0.0f,   
        -0.55f,  0.95f, -0.8f,      1.0f, 0.0f, 0.0f,
		
		-0.64f, -1.0f, -0.81f,     1.0f, 0.0f, 0.0f,        
        -0.64f, -1.0f, -0.89f,     1.0f, 0.0f, 0.0f,        
        -0.56f, -1.0f, -0.89f,     1.0f, 0.0f, 0.0f,   
        -0.56f, -1.0f, -0.81f,     1.0f, 0.0f, 0.0f,
		
		/* Leg 3 */
		 0.55f,  0.95f, -0.8f,      1.0f, 0.0f, 0.0f,        
         0.55f,  0.95f, -0.9f,      1.0f, 0.0f, 0.0f,        
         0.65f,  0.95f, -0.9f,      1.0f, 0.0f, 0.0f,   
         0.65f,  0.95f, -0.8f,      1.0f, 0.0f, 0.0f,
		
		 0.56f, -1.0f, -0.81f,     1.0f, 0.0f, 0.0f,        
         0.56f, -1.0f, -0.89f,     1.0f, 0.0f, 0.0f,        
         0.64f, -1.0f, -0.89f,     1.0f, 0.0f, 0.0f,   
         0.64f, -1.0f, -0.81f,     1.0f, 0.0f, 0.0f,
		
		/* Leg 4 */
		 0.55f,  0.95f,  0.9f,      1.0f, 0.0f, 0.0f,        
         0.55f,  0.95f,  0.8f,      1.0f, 0.0f, 0.0f,        
         0.65f,  0.95f,  0.8f,      1.0f, 0.0f, 0.0f,   
         0.65f,  0.95f,  0.9f,      1.0f, 0.0f, 0.0f,
		
		 0.56f, -1.0f,  0.89f,     1.0f, 0.0f, 0.0f,        
         0.56f, -1.0f,  0.81f,     1.0f, 0.0f, 0.0f,        
         0.64f, -1.0f,  0.81f,     1.0f, 0.0f, 0.0f,   
         0.64f, -1.0f,  0.89f,     1.0f, 0.0f, 0.0f,
		
		/* Bottom plate */
		-0.6f,  -0.65f,  0.85f,      1.0f, 1.0f, 0.0f,        
        -0.6f,  -0.65f, -0.85f,      1.0f, 1.0f, 0.0f,        
         0.6f,  -0.65f, -0.85f,      1.0f, 1.0f, 0.0f,   
         0.6f,  -0.65f,  0.85f,      1.0f, 1.0f, 0.0f,
		
		-0.6f,  -0.70f,  0.85f,     1.0f, 1.0f, 0.0f,        
        -0.6f,  -0.70f, -0.85f,     1.0f, 1.0f, 0.0f,        
         0.6f,  -0.70f, -0.85f,     1.0f, 1.0f, 0.0f,   
         0.6f,  -0.70f,  0.85f,     1.0f, 1.0f, 0.0f,
			 
		/* Drawer */
		-0.6f,  0.95f,  0.85f,      0.0f, 1.0f, 0.0f,        
        -0.6f,  0.95f, -0.85f,      0.0f, 1.0f, 0.0f,        
         0.6f,  0.95f, -0.85f,      0.0f, 1.0f, 0.0f,   
         0.6f,  0.95f,  0.85f,      0.0f, 1.0f, 0.0f,
		
		-0.6f,  0.25f,  0.85f,     0.0f, 1.0f, 0.0f,        
        -0.6f,  0.25f, -0.85f,     0.0f, 1.0f, 0.0f,        
         0.6f,  0.25f, -0.85f,     0.0f, 1.0f, 0.0f,   
         0.6f,  0.25f,  0.85f,     0.0f, 1.0f, 0.0f,
		
		/* Panel */
		-0.65f,  0.85f,  0.65f,      0.0f, 0.0f, 1.0f,        
        -0.65f,  0.85f, -0.65f,      0.0f, 0.0f, 1.0f,        
        -0.60f,  0.85f, -0.65f,      0.0f, 0.0f, 1.0f,   
        -0.60f,  0.85f,  0.65f,      0.0f, 0.0f, 1.0f,
		
		-0.65f,  0.35f,  0.65f,     0.0f, 0.0f, 1.0f,        
        -0.65f,  0.35f, -0.65f,     0.0f, 0.0f, 1.0f,        
        -0.60f,  0.35f, -0.65f,     0.0f, 0.0f, 1.0f,   
        -0.60f,  0.35f,  0.65f,     0.0f, 0.0f, 1.0f
	};

    // Index data.
    GLuint indices[] = {
		/* Table-top. */
		0, 1, 2,
		2, 3, 0,
		4, 5, 6,
		6, 7, 4,
		0, 4, 7,
		7, 3, 0,
		3, 7, 6,
		6, 2, 3,
		6, 5, 1,
		1, 2, 6,
		0, 4, 5,
		5, 1, 0,
		/* Leg 1 (+8) because any rectangular prism has the same order of indices. */
		0+8, 1+8, 2+8,
		2+8, 3+8, 0+8,
		4+8, 5+8, 6+8,
		6+8, 7+8, 4+8,
		0+8, 4+8, 7+8,
		7+8, 3+8, 0+8,
		3+8, 7+8, 6+8,
		6+8, 2+8, 3+8,
		6+8, 5+8, 1+8,
		1+8, 2+8, 6+8,
		0+8, 4+8, 5+8,
		5+8, 1+8, 0+8,
		
		/* Leg 2 (+8) because any rectangular prism has the same order of indices. */
		0+16, 1+16, 2+16,
		2+16, 3+16, 0+16,
		4+16, 5+16, 6+16,
		6+16, 7+16, 4+16,
		0+16, 4+16, 7+16,
		7+16, 3+16, 0+16,
		3+16, 7+16, 6+16,
		6+16, 2+16, 3+16,
		6+16, 5+16, 1+16,
		1+16, 2+16, 6+16,
		0+16, 4+16, 5+16,
		5+16, 1+16, 0+16,
		
		/* Leg 3 (+24) because any rectangular prism has the same order of indices. */
		0+24, 1+24, 2+24,
		2+24, 3+24, 0+24,
		4+24, 5+24, 6+24,
		6+24, 7+24, 4+24,
		0+24, 4+24, 7+24,
		7+24, 3+24, 0+24,
		3+24, 7+24, 6+24,
		6+24, 2+24, 3+24,
		6+24, 5+24, 1+24,
		1+24, 2+24, 6+24,
		0+24, 4+24, 5+24,
		5+24, 1+24, 0+24,
		
		/* Leg 4 (+32) because any rectangular prism has the same order of indices. */
		0+32, 1+32, 2+32,
		2+32, 3+32, 0+32,
		4+32, 5+32, 6+32,
		6+32, 7+32, 4+32,
		0+32, 4+32, 7+32,
		7+32, 3+32, 0+32,
		3+32, 7+32, 6+32,
		6+32, 2+32, 3+32,
		6+32, 5+32, 1+32,
		1+32, 2+32, 6+32,
		0+32, 4+32, 5+32,
		5+32, 1+32, 0+32,
			
		/* Bottom plate (+40) because any rectangular prism has the same order of indices. */
		0+40, 1+40, 2+40,
		2+40, 3+40, 0+40,
		4+40, 5+40, 6+40,
		6+40, 7+40, 4+40,
		0+40, 4+40, 7+40,
		7+40, 3+40, 0+40,
		3+40, 7+40, 6+40,
		6+40, 2+40, 3+40,
		6+40, 5+40, 1+40,
		1+40, 2+40, 6+40,
		0+40, 4+40, 5+40,
		5+40, 1+40, 0+40,
		
		/* Drawer (+48) because any rectangular prism has the same order of indices. */
		0+48, 1+48, 2+48,
		2+48, 3+48, 0+48,
		4+48, 5+48, 6+48,
		6+48, 7+48, 4+48,
		0+48, 4+48, 7+48,
		7+48, 3+48, 0+48,
		3+48, 7+48, 6+48,
		6+48, 2+48, 3+48,
		6+48, 5+48, 1+48,
		1+48, 2+48, 6+48,
		0+48, 4+48, 5+48,
		5+48, 1+48, 0+48,
		
		/* Panel (+56) because any rectangular prism has the same order of indices. */
		0+56, 1+56, 2+56,
		2+56, 3+56, 0+56,
		4+56, 5+56, 6+56,
		6+56, 7+56, 4+56,
		0+56, 4+56, 7+56,
		7+56, 3+56, 0+56,
		3+56, 7+56, 6+56,
		6+56, 2+56, 3+56,
		6+56, 5+56, 1+56,
		1+56, 2+56, 6+56,
		0+56, 4+56, 5+56,
		5+56, 1+56, 0+56
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
	 *	GLUT_MIDDLE_BUTTON
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
	/* This first portion just updates the state of the mouse and how it has moved. */
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
			
			/* CLAMPING to 180 degrees. */
			/* 3.14159 is approximately 180 degrees in radians.*/
			if (object_yaw > 3.14159) {
				object_yaw = 3.14159;	
			} else if (object_yaw < -3.14159) {
				object_yaw = -3.14159;	
			}
			
			if (object_pitch > 3.14159) {
				object_pitch = 3.14159;	
			} else if (object_pitch < -3.14159) {
				object_pitch = -3.14159;	
			}
			
		} else if (rightIsPressed) {
			/* This code affect zooming in and out in non-ortho mode. */
			if (mouseYOffset > 0) {
				cameraPosition += cameraSpeed * CameraForwardZ;
			} else {
				cameraPosition -= cameraSpeed * CameraForwardZ;
			}
		}
	}
}

void UKeyboard (unsigned char key, GLint x, GLint y) {
	if (key == 'o') {
		/* Toggles orthogonal view with 'o'. */
		isOrtho = !isOrtho;	
	}
}