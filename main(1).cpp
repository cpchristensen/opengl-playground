#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <SOIL.h>

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

GLint shaderProgram, lampProgram, WindowWidth = 800, WindowHeight = 600;
// Buffer and Array objects
GLuint VBO, VAO, lightVAO, texture;

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

/* Keyboard callback. */
void UKeyboard (unsigned char key, GLint x, GLint y);

/* Mouse callbacks */
void UMouseClick (int button, int state, int x, int y);
void UMouseMove (int x, int y);
void UMousePressedMove (int x, int y);

/* Keeps track of where the camera is looking and how fast it moves*/
GLfloat cameraSpeed = 0.01f;
GLchar currentKey;
// glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);
// glm::vec3 CameraUpY = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 CameraForwardZ = glm::vec3(0.0f, 0.0f, -5.0f);
glm::vec3 front = glm::vec3(0.0f, 0.0f, -5.0f);

// Information about where the object is.
glm::vec3 objectPosition(0, 0, 0);
glm::vec3 objectScale(2.0f);
glm::vec3 objectColor(0.6, 0.5, 0.75);

// Stores light information.
glm::vec3 lightColor(1.0, 0, 0.0);
glm::vec3 lightColor2(1.0, 1.0, 1.0);

glm::vec3 lightPosition(0.0, 0.5, -3);
glm::vec3 lightPosition2(-3, 0.5, 0);

glm::vec3 lightScale(0.3);
glm::vec3 lightScale2(0.3);

// Camera information.
glm::vec3 cameraPosition(0.0, 0.0, -6);
float cameraRotation = glm::radians(330.0);


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
	layout(location=1) in vec3 normal;
	layout(location=2) in vec2 texture_coordinates;

	// Outgoing coordinates for the texture.
	out vec2 texture_position;

	// Outgoing surface normals to shader.
	out vec3 Normal;

	// Outgoing colors and pixels to fragment shader.
	out vec3 FragmentPos;

	uniform mat4 model;
	uniform mat4 view;
	uniform mat4 projection;

	void main() {
		// Calculates positioning.
		gl_Position = projection * view * model * vec4(position, 1.0f);
		// Calculates where the texture is.
		texture_position = vec2(texture_coordinates.x, 1.0f - texture_coordinates.y);
		// Calculates normals.
		Normal = mat3(transpose(inverse(model))) * normal;
		// Calculates fragment positions.
		FragmentPos = vec3(model * vec4(position, 1.0f));
	}
)GLSL";

// FRAGMENT SHADER SOURCE CODE
const char* fragmentShaderSource = 1 + R"GLSL(
	#version 330 core

	in vec2 texture_position;
	in vec3 Normal;
	in vec3 FragmentPos;

	out vec4 gpuColor;

	uniform sampler2D uTexture;
	uniform vec3 viewPosition;

	// Light 1 info.
	uniform vec3 lightColor;
	uniform vec3 lightPos;
	uniform float ambientStrength;
	uniform float specularIntensity;
	uniform float highlightSize;

	// Light 2 info.
	uniform vec3 lightColor2;
	uniform vec3 lightPos2;
	uniform float ambientStrength2;
	uniform float specularIntensity2;
	uniform float highlightSize2;

	void main() {
		// Calculates ambient lighting for both light sources.
		vec3 ambient = ambientStrength * lightColor;
		vec3 ambient2 = ambientStrength2 * lightColor2;

		// Calculates diffuse lighting for both light sources.
		vec3 norm = normalize(Normal);
		// Calulates the distance between the light source and position of pixel.
		vec3 lightDirection = normalize(lightPos - FragmentPos);
		// Finds diffuse impact.
		float impact = max(dot(norm, lightDirection), 0.0);
		vec3 diffuse = impact * lightColor;

		vec3 norm2 = normalize(Normal);
		// Calulates the distance between the light source and position of pixel.
		vec3 lightDirection2 = normalize(lightPos2 - FragmentPos);
		// Finds diffuse impact.
		float impact2 = max(dot(norm2, lightDirection2), 0.0);
		vec3 diffuse2 = impact2 * lightColor2;

		// Calulates specular lighting for both light sources.
		// Finds view direction and reflection vector.
		vec3 viewDir = normalize(viewPosition - FragmentPos);
		vec3 reflectDir = reflect(-lightDirection, norm);
		// Uses these values to calculate specular compoenent.
		float specularComponent = pow(max(dot(viewDir, reflectDir), 0.0), highlightSize);
		vec3 specular = specularIntensity * specularComponent * lightColor;

		// Finds view direction and reflection vector.
		vec3 viewDir2 = normalize(viewPosition - FragmentPos);
		vec3 reflectDir2 = reflect(-lightDirection2, norm2);
		// Uses these values to calculate specular compoenent.
		float specularComponent2 = pow(max(dot(viewDir2, reflectDir2), 0.0), highlightSize2);
		vec3 specular2 = specularIntensity2 * specularComponent2 * lightColor2;

		// Uses calculated values to assemble phong lighting.
		// Applies texture as well to complete image.
		vec3 phong = (ambient + diffuse + specular) + (ambient2 + diffuse2 + specular2);
		gpuColor = vec4(phong, 1.0f) * texture(uTexture, texture_position);

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

	/* Sets mouse callbacks.*/
	glutMouseFunc(UMouseClick);
	glutMotionFunc(UMousePressedMove);
	/* Sets keyboard callback. */
	glutKeyboardFunc(UKeyboard);

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
	/* Moves camera based on key press. */

	GLint lightColorLoc, lightPositionLoc, viewPositionLoc;
	GLint ambientLoc, specularLoc, highlightLoc;

	CameraForwardZ = front;

    // Transforms object.
    glm::mat4 model(1.0);
    // Centers object in viewport.
    model = glm::translate(model, objectPosition);

	// NOTE: THIS MAY CAUSE PROBLEMS.
	model = glm::rotate(model, (float) (glm::radians(object_pitch) * RADIANS_TO_DEGREES), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, (float) (glm::radians(object_yaw) * RADIANS_TO_DEGREES), glm::vec3(0.0f, 1.0f, 0.0f));

    model = glm::scale(model, objectScale);

	// Transforms camera.
	glm::mat4 view(1.0);
	view = glm::translate(view, cameraPosition);
	view = glm::rotate(view, cameraRotation, glm::vec3(0.0f, 1.0f, 0.0f));

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

	// Tells the shader the viewing position.
	viewPositionLoc = glGetUniformLocation(shaderProgram, "viewPosition");
	glUniform3f(viewPositionLoc, cameraPosition.x, cameraPosition.y, cameraPosition.z);

	// Sends data for initial light source.
	lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor");
	lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPos");
	ambientLoc = glGetUniformLocation(shaderProgram, "ambientStrength");
	specularLoc = glGetUniformLocation(shaderProgram, "specularIntensity");
	highlightLoc = glGetUniformLocation(shaderProgram, "highlightSize");

	glUniform3f(lightColorLoc, lightColor.r, lightColor.g, lightColor.b);
	glUniform3f(lightPositionLoc, lightPosition.x, lightPosition.y, lightPosition.z);
	glUniform1f(ambientLoc, 0.1);
	glUniform1f(specularLoc, 1.0);
	glUniform1f(highlightLoc, 16.0);

	// Sends data for second light source.
	lightColorLoc = glGetUniformLocation(shaderProgram, "lightColor2");
	lightPositionLoc = glGetUniformLocation(shaderProgram, "lightPos2");
	ambientLoc = glGetUniformLocation(shaderProgram, "ambientStrength2");
	specularLoc = glGetUniformLocation(shaderProgram, "specularIntensity2");
	highlightLoc = glGetUniformLocation(shaderProgram, "highlightSize2");

	glUniform3f(lightColorLoc, lightColor2.r, lightColor2.g, lightColor2.b);
	glUniform3f(lightPositionLoc, lightPosition2.x, lightPosition2.y, lightPosition2.z);
	glUniform1f(ambientLoc, 0.1);
	glUniform1f(specularLoc, 0.1 );
	glUniform1f(highlightLoc, 16.0);

	glBindTexture(GL_TEXTURE_2D, texture);
	// Draws array data to screen.
	glDrawArrays(GL_TRIANGLES, 0, 36 * 8);
    // Deactivate VAO
    glBindVertexArray(0);
	glutPostRedisplay();
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
		int size;
		char* str = (char*) calloc(1024, 1);
		glGetShaderInfoLog(vertexShader, 1024, &size, str);
		printf("ERROR COMPILING VERTEX SHADER.\n%s\n", str);
	}


	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	// Retrieves shader and frag source code.
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	// Compiles shader source code.
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	// Error checking for shader.
	if (success == GL_FALSE) {
		int size;
		char* str = (char*) calloc(1024, 1);
		glGetShaderInfoLog(fragmentShader, 1024, &size, str);
		printf("ERROR COMPILING FRAGMENT SHADER.\n%s\n", str);
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
	// Sets vertex coordinates.
	GLfloat verts[] = {
		// New prism.
		// TOP
		-0.75, 1, 1, 	0, 1, 0, 	0, 1,
		-0.75, 1, -1, 	0, 1, 0, 	0, 0,
		0.75, 1, -1, 	0, 1, 0, 	1, 0,
		0.75, 1, -1, 	0, 1, 0, 	1, 0,
		0.75, 1, 1, 	0, 1, 0, 	1, 1,
		-0.75, 1, 1, 	0, 1, 0, 	0, 1,

		// BOTTOM
		-0.7, 0.95, 0.95, 	0, -1, 0, 	0, 0,
		-0.7, 0.95, -0.95, 	0, -1, 0, 	0, 1,
		0.7, 0.95, -0.95, 	0, -1, 0, 	1, 1,
		0.7, 0.95, -0.95, 	0, -1, 0, 	1, 1,
		0.7, 0.95, 0.95, 	0, -1, 0, 	1, 0,
		-0.7, 0.95, 0.95, 	0, -1, 0, 	0, 0,

		// BACK
		-0.75, 1, 1, 	0, 0, 1, 	1, 1,
		-0.7, 0.95, 0.95, 	0, 0, 1, 	1, 0,
		0.7, 0.95, 0.95, 	0, 0, 1, 	0, 0,
		0.7, 0.95, 0.95, 	0, 0, 1, 	0, 0,
		0.75, 1, 1, 	0, 0, 1, 	0, 1,
		-0.75, 1, 1, 	0, 0, 1, 	1, 1,

		// RIGHT
		0.75, 1, 1, 	1, 0, 0, 	1, 1,
		0.7, 0.95, 0.95, 	1, 0, 0, 	1, 0,
		0.7, 0.95, -0.95, 	1, 0, 0, 	0, 0,
		0.7, 0.95, -0.95, 	1, 0, 0, 	0, 0,
		0.75, 1, -1, 	1, 0, 0, 	0, 1,
		0.75, 1, 1, 	1, 0, 0, 	1, 1,

		// FRONT
		0.7, 0.95, -0.95, 	0, 0, -1, 	1, 0,
		-0.7, 0.95, -0.95, 	0, 0, -1, 	0, 0,
		-0.75, 1, -1, 	0, 0, -1, 	0, 1,
		-0.75, 1, -1, 	0, 0, -1, 	0, 1,
		0.75, 1, -1, 	0, 0, -1, 	1, 1,
		0.7, 0.95, -0.95, 	0, 0, -1, 	1, 0,

		// LEFT
		-0.75, 1, 1, 	-1, 0, 0, 	1, 1,
		-0.7, 0.95, 0.95, 	-1, 0, 0, 	1, 0,
		-0.7, 0.95, -0.95, 	-1, 0, 0, 	0, 0,
		-0.7, 0.95, -0.95, 	-1, 0, 0, 	0, 0,
		-0.75, 1, -1, 	-1, 0, 0, 	0, 1,
		-0.75, 1, 1, 	-1, 0, 0, 	1, 1,

		// New prism.
		// TOP
		-0.65, 0.95, 0.9, 	0, 1, 0, 	0, 1,
		-0.65, 0.95, 0.8, 	0, 1, 0, 	0, 0,
		-0.55, 0.95, 0.8, 	0, 1, 0, 	1, 0,
		-0.55, 0.95, 0.8, 	0, 1, 0, 	1, 0,
		-0.55, 0.95, 0.9, 	0, 1, 0, 	1, 1,
		-0.65, 0.95, 0.9, 	0, 1, 0, 	0, 1,

		// BOTTOM
		-0.64, -1, 0.89, 	0, -1, 0, 	0, 0,
		-0.64, -1, 0.81, 	0, -1, 0, 	0, 1,
		-0.56, -1, 0.81, 	0, -1, 0, 	1, 1,
		-0.56, -1, 0.81, 	0, -1, 0, 	1, 1,
		-0.56, -1, 0.89, 	0, -1, 0, 	1, 0,
		-0.64, -1, 0.89, 	0, -1, 0, 	0, 0,

		// BACK
		-0.65, 0.95, 0.9, 	0, 0, 1, 	1, 1,
		-0.64, -1, 0.89, 	0, 0, 1, 	1, 0,
		-0.56, -1, 0.89, 	0, 0, 1, 	0, 0,
		-0.56, -1, 0.89, 	0, 0, 1, 	0, 0,
		-0.55, 0.95, 0.9, 	0, 0, 1, 	0, 1,
		-0.65, 0.95, 0.9, 	0, 0, 1, 	1, 1,

		// RIGHT
		-0.55, 0.95, 0.9, 	1, 0, 0, 	1, 1,
		-0.56, -1, 0.89, 	1, 0, 0, 	1, 0,
		-0.56, -1, 0.81, 	1, 0, 0, 	0, 0,
		-0.56, -1, 0.81, 	1, 0, 0, 	0, 0,
		-0.55, 0.95, 0.8, 	1, 0, 0, 	0, 1,
		-0.55, 0.95, 0.9, 	1, 0, 0, 	1, 1,

		// FRONT
		-0.56, -1, 0.81, 	0, 0, -1, 	1, 0,
		-0.64, -1, 0.81, 	0, 0, -1, 	0, 0,
		-0.65, 0.95, 0.8, 	0, 0, -1, 	0, 1,
		-0.65, 0.95, 0.8, 	0, 0, -1, 	0, 1,
		-0.55, 0.95, 0.8, 	0, 0, -1, 	1, 1,
		-0.56, -1, 0.81, 	0, 0, -1, 	1, 0,

		// LEFT
		-0.65, 0.95, 0.9, 	-1, 0, 0, 	1, 1,
		-0.64, -1, 0.89, 	-1, 0, 0, 	1, 0,
		-0.64, -1, 0.81, 	-1, 0, 0, 	0, 0,
		-0.64, -1, 0.81, 	-1, 0, 0, 	0, 0,
		-0.65, 0.95, 0.8, 	-1, 0, 0, 	0, 1,
		-0.65, 0.95, 0.9, 	-1, 0, 0, 	1, 1,

		// New prism.
		// TOP
		-0.65, 0.95, -0.8, 	0, 1, 0, 	0, 1,
		-0.65, 0.95, -0.9, 	0, 1, 0, 	0, 0,
		-0.55, 0.95, -0.9, 	0, 1, 0, 	1, 0,
		-0.55, 0.95, -0.9, 	0, 1, 0, 	1, 0,
		-0.55, 0.95, -0.8, 	0, 1, 0, 	1, 1,
		-0.65, 0.95, -0.8, 	0, 1, 0, 	0, 1,

		// BOTTOM
		-0.64, -1, -0.81, 	0, -1, 0, 	0, 0,
		-0.64, -1, -0.89, 	0, -1, 0, 	0, 1,
		-0.56, -1, -0.89, 	0, -1, 0, 	1, 1,
		-0.56, -1, -0.89, 	0, -1, 0, 	1, 1,
		-0.56, -1, -0.81, 	0, -1, 0, 	1, 0,
		-0.64, -1, -0.81, 	0, -1, 0, 	0, 0,

		// BACK
		-0.65, 0.95, -0.8, 	0, 0, 1, 	1, 1,
		-0.64, -1, -0.81, 	0, 0, 1, 	1, 0,
		-0.56, -1, -0.81, 	0, 0, 1, 	0, 0,
		-0.56, -1, -0.81, 	0, 0, 1, 	0, 0,
		-0.55, 0.95, -0.8, 	0, 0, 1, 	0, 1,
		-0.65, 0.95, -0.8, 	0, 0, 1, 	1, 1,

		// RIGHT
		-0.55, 0.95, -0.8, 	1, 0, 0, 	1, 1,
		-0.56, -1, -0.81, 	1, 0, 0, 	1, 0,
		-0.56, -1, -0.89, 	1, 0, 0, 	0, 0,
		-0.56, -1, -0.89, 	1, 0, 0, 	0, 0,
		-0.55, 0.95, -0.9, 	1, 0, 0, 	0, 1,
		-0.55, 0.95, -0.8, 	1, 0, 0, 	1, 1,

		// FRONT
		-0.56, -1, -0.89, 	0, 0, -1, 	1, 0,
		-0.64, -1, -0.89, 	0, 0, -1, 	0, 0,
		-0.65, 0.95, -0.9, 	0, 0, -1, 	0, 1,
		-0.65, 0.95, -0.9, 	0, 0, -1, 	0, 1,
		-0.55, 0.95, -0.9, 	0, 0, -1, 	1, 1,
		-0.56, -1, -0.89, 	0, 0, -1, 	1, 0,

		// LEFT
		-0.65, 0.95, -0.8, 	-1, 0, 0, 	1, 1,
		-0.64, -1, -0.81, 	-1, 0, 0, 	1, 0,
		-0.64, -1, -0.89, 	-1, 0, 0, 	0, 0,
		-0.64, -1, -0.89, 	-1, 0, 0, 	0, 0,
		-0.65, 0.95, -0.9, 	-1, 0, 0, 	0, 1,
		-0.65, 0.95, -0.8, 	-1, 0, 0, 	1, 1,

		// New prism.
		// TOP
		0.55, 0.95, -0.8, 	0, 1, 0, 	0, 1,
		0.55, 0.95, -0.9, 	0, 1, 0, 	0, 0,
		0.65, 0.95, -0.9, 	0, 1, 0, 	1, 0,
		0.65, 0.95, -0.9, 	0, 1, 0, 	1, 0,
		0.65, 0.95, -0.8, 	0, 1, 0, 	1, 1,
		0.55, 0.95, -0.8, 	0, 1, 0, 	0, 1,

		// BOTTOM
		0.56, -1, -0.81, 	0, -1, 0, 	0, 0,
		0.56, -1, -0.89, 	0, -1, 0, 	0, 1,
		0.64, -1, -0.89, 	0, -1, 0, 	1, 1,
		0.64, -1, -0.89, 	0, -1, 0, 	1, 1,
		0.64, -1, -0.81, 	0, -1, 0, 	1, 0,
		0.56, -1, -0.81, 	0, -1, 0, 	0, 0,

		// BACK
		0.55, 0.95, -0.8, 	0, 0, 1, 	1, 1,
		0.56, -1, -0.81, 	0, 0, 1, 	1, 0,
		0.64, -1, -0.81, 	0, 0, 1, 	0, 0,
		0.64, -1, -0.81, 	0, 0, 1, 	0, 0,
		0.65, 0.95, -0.8, 	0, 0, 1, 	0, 1,
		0.55, 0.95, -0.8, 	0, 0, 1, 	1, 1,

		// RIGHT
		0.65, 0.95, -0.8, 	1, 0, 0, 	1, 1,
		0.64, -1, -0.81, 	1, 0, 0, 	1, 0,
		0.64, -1, -0.89, 	1, 0, 0, 	0, 0,
		0.64, -1, -0.89, 	1, 0, 0, 	0, 0,
		0.65, 0.95, -0.9, 	1, 0, 0, 	0, 1,
		0.65, 0.95, -0.8, 	1, 0, 0, 	1, 1,

		// FRONT
		0.64, -1, -0.89, 	0, 0, -1, 	1, 0,
		0.56, -1, -0.89, 	0, 0, -1, 	0, 0,
		0.55, 0.95, -0.9, 	0, 0, -1, 	0, 1,
		0.55, 0.95, -0.9, 	0, 0, -1, 	0, 1,
		0.65, 0.95, -0.9, 	0, 0, -1, 	1, 1,
		0.64, -1, -0.89, 	0, 0, -1, 	1, 0,

		// LEFT
		0.55, 0.95, -0.8, 	-1, 0, 0, 	1, 1,
		0.56, -1, -0.81, 	-1, 0, 0, 	1, 0,
		0.56, -1, -0.89, 	-1, 0, 0, 	0, 0,
		0.56, -1, -0.89, 	-1, 0, 0, 	0, 0,
		0.55, 0.95, -0.9, 	-1, 0, 0, 	0, 1,
		0.55, 0.95, -0.8, 	-1, 0, 0, 	1, 1,

		// New prism.
		// TOP
		0.55, 0.95, 0.9, 	0, 1, 0, 	0, 1,
		0.55, 0.95, 0.8, 	0, 1, 0, 	0, 0,
		0.65, 0.95, 0.8, 	0, 1, 0, 	1, 0,
		0.65, 0.95, 0.8, 	0, 1, 0, 	1, 0,
		0.65, 0.95, 0.9, 	0, 1, 0, 	1, 1,
		0.55, 0.95, 0.9, 	0, 1, 0, 	0, 1,

		// BOTTOM
		0.56, -1, 0.89, 	0, -1, 0, 	0, 0,
		0.56, -1, 0.81, 	0, -1, 0, 	0, 1,
		0.64, -1, 0.81, 	0, -1, 0, 	1, 1,
		0.64, -1, 0.81, 	0, -1, 0, 	1, 1,
		0.64, -1, 0.89, 	0, -1, 0, 	1, 0,
		0.56, -1, 0.89, 	0, -1, 0, 	0, 0,

		// BACK
		0.55, 0.95, 0.9, 	0, 0, 1, 	1, 1,
		0.56, -1, 0.89, 	0, 0, 1, 	1, 0,
		0.64, -1, 0.89, 	0, 0, 1, 	0, 0,
		0.64, -1, 0.89, 	0, 0, 1, 	0, 0,
		0.65, 0.95, 0.9, 	0, 0, 1, 	0, 1,
		0.55, 0.95, 0.9, 	0, 0, 1, 	1, 1,

		// RIGHT
		0.65, 0.95, 0.9, 	1, 0, 0, 	1, 1,
		0.64, -1, 0.89, 	1, 0, 0, 	1, 0,
		0.64, -1, 0.81, 	1, 0, 0, 	0, 0,
		0.64, -1, 0.81, 	1, 0, 0, 	0, 0,
		0.65, 0.95, 0.8, 	1, 0, 0, 	0, 1,
		0.65, 0.95, 0.9, 	1, 0, 0, 	1, 1,

		// FRONT
		0.64, -1, 0.81, 	0, 0, -1, 	1, 0,
		0.56, -1, 0.81, 	0, 0, -1, 	0, 0,
		0.55, 0.95, 0.8, 	0, 0, -1, 	0, 1,
		0.55, 0.95, 0.8, 	0, 0, -1, 	0, 1,
		0.65, 0.95, 0.8, 	0, 0, -1, 	1, 1,
		0.64, -1, 0.81, 	0, 0, -1, 	1, 0,

		// LEFT
		0.55, 0.95, 0.9, 	-1, 0, 0, 	1, 1,
		0.56, -1, 0.89, 	-1, 0, 0, 	1, 0,
		0.56, -1, 0.81, 	-1, 0, 0, 	0, 0,
		0.56, -1, 0.81, 	-1, 0, 0, 	0, 0,
		0.55, 0.95, 0.8, 	-1, 0, 0, 	0, 1,
		0.55, 0.95, 0.9, 	-1, 0, 0, 	1, 1,

		// New prism.
		// TOP
		-0.6, -0.65, 0.85, 	0, 1, 0, 	0, 1,
		-0.6, -0.65, -0.85, 	0, 1, 0, 	0, 0,
		0.6, -0.65, -0.85, 	0, 1, 0, 	1, 0,
		0.6, -0.65, -0.85, 	0, 1, 0, 	1, 0,
		0.6, -0.65, 0.85, 	0, 1, 0, 	1, 1,
		-0.6, -0.65, 0.85, 	0, 1, 0, 	0, 1,

		// BOTTOM
		-0.6, -0.7, 0.85, 	0, -1, 0, 	0, 0,
		-0.6, -0.7, -0.85, 	0, -1, 0, 	0, 1,
		0.6, -0.7, -0.85, 	0, -1, 0, 	1, 1,
		0.6, -0.7, -0.85, 	0, -1, 0, 	1, 1,
		0.6, -0.7, 0.85, 	0, -1, 0, 	1, 0,
		-0.6, -0.7, 0.85, 	0, -1, 0, 	0, 0,

		// BACK
		-0.6, -0.65, 0.85, 	0, 0, 1, 	1, 1,
		-0.6, -0.7, 0.85, 	0, 0, 1, 	1, 0,
		0.6, -0.7, 0.85, 	0, 0, 1, 	0, 0,
		0.6, -0.7, 0.85, 	0, 0, 1, 	0, 0,
		0.6, -0.65, 0.85, 	0, 0, 1, 	0, 1,
		-0.6, -0.65, 0.85, 	0, 0, 1, 	1, 1,

		// RIGHT
		0.6, -0.65, 0.85, 	1, 0, 0, 	1, 1,
		0.6, -0.7, 0.85, 	1, 0, 0, 	1, 0,
		0.6, -0.7, -0.85, 	1, 0, 0, 	0, 0,
		0.6, -0.7, -0.85, 	1, 0, 0, 	0, 0,
		0.6, -0.65, -0.85, 	1, 0, 0, 	0, 1,
		0.6, -0.65, 0.85, 	1, 0, 0, 	1, 1,

		// FRONT
		0.6, -0.7, -0.85, 	0, 0, -1, 	1, 0,
		-0.6, -0.7, -0.85, 	0, 0, -1, 	0, 0,
		-0.6, -0.65, -0.85, 	0, 0, -1, 	0, 1,
		-0.6, -0.65, -0.85, 	0, 0, -1, 	0, 1,
		0.6, -0.65, -0.85, 	0, 0, -1, 	1, 1,
		0.6, -0.7, -0.85, 	0, 0, -1, 	1, 0,

		// LEFT
		-0.6, -0.65, 0.85, 	-1, 0, 0, 	1, 1,
		-0.6, -0.7, 0.85, 	-1, 0, 0, 	1, 0,
		-0.6, -0.7, -0.85, 	-1, 0, 0, 	0, 0,
		-0.6, -0.7, -0.85, 	-1, 0, 0, 	0, 0,
		-0.6, -0.65, -0.85, 	-1, 0, 0, 	0, 1,
		-0.6, -0.65, 0.85, 	-1, 0, 0, 	1, 1,

		// New prism.
		// TOP
		-0.6, 0.95, 0.85, 	0, 1, 0, 	0, 1,
		-0.6, 0.95, -0.85, 	0, 1, 0, 	0, 0,
		0.6, 0.95, -0.85, 	0, 1, 0, 	1, 0,
		0.6, 0.95, -0.85, 	0, 1, 0, 	1, 0,
		0.6, 0.95, 0.85, 	0, 1, 0, 	1, 1,
		-0.6, 0.95, 0.85, 	0, 1, 0, 	0, 1,

		// BOTTOM
		-0.6, 0.25, 0.85, 	0, -1, 0, 	0, 0,
		-0.6, 0.25, -0.85, 	0, -1, 0, 	0, 1,
		0.6, 0.25, -0.85, 	0, -1, 0, 	1, 1,
		0.6, 0.25, -0.85, 	0, -1, 0, 	1, 1,
		0.6, 0.25, 0.85, 	0, -1, 0, 	1, 0,
		-0.6, 0.25, 0.85, 	0, -1, 0, 	0, 0,

		// BACK
		-0.6, 0.95, 0.85, 	0, 0, 1, 	1, 1,
		-0.6, 0.25, 0.85, 	0, 0, 1, 	1, 0,
		0.6, 0.25, 0.85, 	0, 0, 1, 	0, 0,
		0.6, 0.25, 0.85, 	0, 0, 1, 	0, 0,
		0.6, 0.95, 0.85, 	0, 0, 1, 	0, 1,
		-0.6, 0.95, 0.85, 	0, 0, 1, 	1, 1,

		// RIGHT
		0.6, 0.95, 0.85, 	1, 0, 0, 	1, 1,
		0.6, 0.25, 0.85, 	1, 0, 0, 	1, 0,
		0.6, 0.25, -0.85, 	1, 0, 0, 	0, 0,
		0.6, 0.25, -0.85, 	1, 0, 0, 	0, 0,
		0.6, 0.95, -0.85, 	1, 0, 0, 	0, 1,
		0.6, 0.95, 0.85, 	1, 0, 0, 	1, 1,

		// FRONT
		0.6, 0.25, -0.85, 	0, 0, -1, 	1, 0,
		-0.6, 0.25, -0.85, 	0, 0, -1, 	0, 0,
		-0.6, 0.95, -0.85, 	0, 0, -1, 	0, 1,
		-0.6, 0.95, -0.85, 	0, 0, -1, 	0, 1,
		0.6, 0.95, -0.85, 	0, 0, -1, 	1, 1,
		0.6, 0.25, -0.85, 	0, 0, -1, 	1, 0,

		// LEFT
		-0.6, 0.95, 0.85, 	-1, 0, 0, 	1, 1,
		-0.6, 0.25, 0.85, 	-1, 0, 0, 	1, 0,
		-0.6, 0.25, -0.85, 	-1, 0, 0, 	0, 0,
		-0.6, 0.25, -0.85, 	-1, 0, 0, 	0, 0,
		-0.6, 0.95, -0.85, 	-1, 0, 0, 	0, 1,
		-0.6, 0.95, 0.85, 	-1, 0, 0, 	1, 1,

		// New prism.
		// TOP
		-0.65, 0.85, 0.65, 	0, 1, 0, 	0, 1,
		-0.65, 0.85, -0.65, 	0, 1, 0, 	0, 0,
		-0.6, 0.85, -0.65, 	0, 1, 0, 	1, 0,
		-0.6, 0.85, -0.65, 	0, 1, 0, 	1, 0,
		-0.6, 0.85, 0.65, 	0, 1, 0, 	1, 1,
		-0.65, 0.85, 0.65, 	0, 1, 0, 	0, 1,

		// BOTTOM
		-0.65, 0.35, 0.65, 	0, -1, 0, 	0, 0,
		-0.65, 0.35, -0.65, 	0, -1, 0, 	0, 1,
		-0.6, 0.35, -0.65, 	0, -1, 0, 	1, 1,
		-0.6, 0.35, -0.65, 	0, -1, 0, 	1, 1,
		-0.6, 0.35, 0.65, 	0, -1, 0, 	1, 0,
		-0.65, 0.35, 0.65, 	0, -1, 0, 	0, 0,

		// BACK
		-0.65, 0.85, 0.65, 	0, 0, 1, 	1, 1,
		-0.65, 0.35, 0.65, 	0, 0, 1, 	1, 0,
		-0.6, 0.35, 0.65, 	0, 0, 1, 	0, 0,
		-0.6, 0.35, 0.65, 	0, 0, 1, 	0, 0,
		-0.6, 0.85, 0.65, 	0, 0, 1, 	0, 1,
		-0.65, 0.85, 0.65, 	0, 0, 1, 	1, 1,

		// RIGHT
		-0.6, 0.85, 0.65, 	1, 0, 0, 	1, 1,
		-0.6, 0.35, 0.65, 	1, 0, 0, 	1, 0,
		-0.6, 0.35, -0.65, 	1, 0, 0, 	0, 0,
		-0.6, 0.35, -0.65, 	1, 0, 0, 	0, 0,
		-0.6, 0.85, -0.65, 	1, 0, 0, 	0, 1,
		-0.6, 0.85, 0.65, 	1, 0, 0, 	1, 1,

		// FRONT
		-0.6, 0.35, -0.65, 	0, 0, -1, 	1, 0,
		-0.65, 0.35, -0.65, 	0, 0, -1, 	0, 0,
		-0.65, 0.85, -0.65, 	0, 0, -1, 	0, 1,
		-0.65, 0.85, -0.65, 	0, 0, -1, 	0, 1,
		-0.6, 0.85, -0.65, 	0, 0, -1, 	1, 1,
		-0.6, 0.35, -0.65, 	0, 0, -1, 	1, 0,

		// LEFT
		-0.65, 0.85, 0.65, 	-1, 0, 0, 	1, 1,
		-0.65, 0.35, 0.65, 	-1, 0, 0, 	1, 0,
		-0.65, 0.35, -0.65, 	-1, 0, 0, 	0, 0,
		-0.65, 0.35, -0.65, 	-1, 0, 0, 	0, 0,
		-0.65, 0.85, -0.65, 	-1, 0, 0, 	0, 1,
		-0.65, 0.85, 0.65, 	-1, 0, 0, 	1, 1

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
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid*) 0);
	glEnableVertexAttribArray(0); // Sets initial position of rgba in buffer.

    // Tells GPU how to handle VBO.
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid*) (3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1); // Sets initial position of rgba in buffer.

	// Tells GPU the Surface Normal values.
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 8, (GLvoid*) (6 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2); // Sets initial position of rgba in buffer.

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

void UGenerateTexture (void) {
	// Creates and binds texture.
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	int width, height;
	// Reads texture from file.
	unsigned char* image = SOIL_load_image("wood.jpg", &width, &height, 0, SOIL_LOAD_RGB);

	// Writes image data to texture.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);

	// Frees image and unbinds texture.
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);
}
