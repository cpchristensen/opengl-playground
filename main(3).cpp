#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

#include <SOIL.h>

// GLM Stuff
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define WINDOW_TITLE "Modern OpenGL"

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

using namespace std;

GLint shaderProgram, WindowWidth = 800, WindowHeight = 600;
// Buffer and Array objects
GLuint VBO, VAO, texture;

// Used for rotating pyramid.
GLfloat object_pitch = 0.0, object_yaw = 0.0, object_angle_increment = 0.01;

/*
 * User defined function prototypes.
 * Initializes basic elements of program.
 * Provides functions for drawing to screen.
 */
void UResizeWindow (int, int);
void URenderGraphics (void);
void UCreateShader (void);
void UCreateBuffers (void);
void UGenerateTexture (void);

const char* vertexShaderSource = 1 + R"GLSL(
	#version 330 core
	layout(location=0) in vec3 position;
	layout(location=2) in vec2 color;

	out vec2 mobileColor;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		gl_Position = projection * view * model * vec4(position, 1.0f);
		mobileColor = vec2(color.x, 1.0f - color.y);
	}
)GLSL";

// FRAGMENT SHADER SOURCE CODE
const char* fragmentShaderSource = 1 + R"GLSL(
	#version 330 core
	in vec2 mobileColor;
	out vec4 gpuColor;

	uniform sampler2D uTexture;

	void main() {
		gpuColor = texture(uTexture, mobileColor);
		// Just makes the object white (for testing).
		// gpuColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
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
	UGenerateTexture();

	// Uses shader program.
	glUseProgram(shaderProgram);

	// Sets background color.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glutDisplayFunc(URenderGraphics);
	glutMainLoop();

    // Garbage Collection
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

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

    // Transforms object.
    glm::mat4 model(1.0f);
    // Centers object in viewport.
    model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));

	// Displays Pyramid and rotates it slightly.
	model = glm::rotate(model, object_pitch, glm::vec3(1.0, 0.0f, 0.0f));
	model = glm::rotate(model, object_yaw, glm::vec3(0.0, 1.0f, 0.0f));
	object_pitch += object_angle_increment;
	object_yaw += object_angle_increment;

    // Scales to double size in xyz.
    model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));

	// Transforms camera.
	glm::mat4 view(1.0f);
	// Moves .5 units on X and-5 units on Z.
	view = glm::translate(view, glm::vec3(0.5f, 0.0f, -5.0f));

	//Creates perspective.
	glm::mat4 projection;
	projection = glm::perspective(45.0f, (GLfloat) WindowWidth / (GLfloat) WindowHeight, 0.1f, 100.0f);

	// Sends matrices to shader program.
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
	GLint projLoc = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	glutPostRedisplay();

	glBindTexture(GL_TEXTURE_2D, texture);

	// Draws array data to screen.
	glDrawArrays(GL_TRIANGLES, 0, 12);
	glDrawArrays(GL_QUADS, 12, 4);

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

	GLint success = 0;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	// Error checking for shader.
	if (success == GL_FALSE) {
		printf("ERROR COMPILING VERTEX SHADER.\n");
	}


	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Retrieves shader and frag source code.
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	// Compiles shader source code.
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	// Error checking for shader.
	if (success == GL_FALSE) {
		printf("ERROR COMPILING FRAGMENT SHADER.\n");
	}

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
	GLfloat verts[] = {
        // Coordinates      texture coords

		// Triange 1
		0.0f, 0.5f, 0.0f,		0.5f, 1.0f,
        -0.5f, -0.5f, 0.5f, 	0.0f, 0.0f,
        0.5f, -0.5f, 0.5f,  	1.0f, 0.0f,
		// Triange 2
		0.0f, 0.5f, 0.0f,   	0.5f, 1.0f,
		0.5f, -0.5f, 0.5f,  	0.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 	1.0f, 0.0f,
		// Triange 3
		0.0f, 0.5f, 0.0f,   	0.5f, 1.0f,
		-0.5f, -0.5f, -0.5f,	1.0f, 0.0f,
		0.5f, -0.5f, -0.5f, 	0.0f, 0.0f,
		// Triange 4
		0.0f, 0.5f, 0.0f,   	0.5f, 1.0f,
        -0.5f, -0.5f, 0.5f, 	1.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,	0.0f, 0.0f,
		// Bottom square.
		-0.5f, -0.5f, 0.5f, 	0.0f, 1.0f,
        0.5f, -0.5f, 0.5f,  	1.0f, 1.0f,
		0.5f, -0.5f, -0.5f, 	1.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,	0.0f, 0.0f
	};

    // Generate buffer IDs
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

	// Activation of VAO before binding.
    glBindVertexArray(VAO);

	// Activates the buffer.
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// Sends data to GPU
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

	// Tells GPU how to handle VBO.
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid*) 0);
	glEnableVertexAttribArray(0); // Sets initial position of rgba in buffer.

    // Tells GPU how to handle VBO.
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid*) (3*sizeof(GLfloat)));
	glEnableVertexAttribArray(2); // Sets initial position of rgba in buffer.

    glBindVertexArray(0);
}

void UGenerateTexture (void) {
	// Creates and binds texture.
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int width, height
	// Reads texture from file.
	unsigned char* image = SOIL_load_image("brick.jpg", &width, &height, 0, SOIL_LOAD_RGB);

	// Writes image data to texture.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Frees image and unbinds texture.
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);
}
