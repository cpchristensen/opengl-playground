#include <iostream>
#include <GL/glew.h>
#include <GL/freeglut.h>

#define WINDOW_TITLE "Modern OpenGL"

#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version "\n" #Source
#endif

using namespace std;

int WindowWidth = 800;
int WindowHeight = 600;

/*
 * User defined function prototypes.
 * Initializes basic elements of program.
 * Provides functions for drawing to screen.
 */
void UInitialize (int, char**);
void UInitWindow (int, char**);
void UResizeWindow (int, int);
void URenderGraphics (void);
void UCreateVBO (void);
void UCreateShaders (void);

// Shader program source code.
const GLchar* VertexShader = GLSL(440,
	in layout(location=0) vec4 vertex_Position; // Receive vertex coordinates from attribute 0.

	// Get vertex colors from VBO.
	in layout(location=1) vec4 colorFromVBO;
	// Color for each vertex.
	out vec4 colorFromVShader;

	void main () {
		gl_Position = vertex_Position; // Sends vertex positions to gl_position vec 4.
		colorFromVShader = colorFromVBO; // References vertex colors sent from the buffer.
	}
);

const GLchar* FragmentShader = GLSL(440,
	in vec4 colorFromVShader;
	out vec4 vertex_Color;
	// Required on my system. The tutorial did not work for me. Further research revealed that gl_FragColor was deprecated in version 3.30.
	// out vec4 fragColor;
	void main () {
		// fragColor = vec4(0.0, 1.0, 0.0, 1.0); // Green.

		// Sets the current vertex color to the color provided from the command buffer.
		vertex_Color = colorFromVShader;
	}
);

int main (int argc, char** argv) {
	UInitialize(argc, argv);
	glutMainLoop();
	return 0;
}

void UInitialize (int argc, char** argv) {
	GLenum GlewInitResult;
	// Initializes window with size.
	UInitWindow(argc, argv);

	// Initializes glew and checks for errors.
	GlewInitResult = glewInit();
	if (GlewInitResult != GLEW_OK) {
		fprintf(stderr, "ERROR: %s\n", glewGetErrorString(GlewInitResult));
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "INFO: OpenGL Version: %s\n", glGetString(GL_VERSION));

	// Creates Vertex Buffer Object
	UCreateVBO();

	// Creates shader program.
	UCreateShaders();

	// Sets background color.
	glClearColor(0, 0, 0, 1);
}

void UInitWindow (int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitWindowSize(WindowWidth, WindowHeight);

	// Initializes memory display buffer.
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);

	// Sets window title and creates window.
	glutCreateWindow(WINDOW_TITLE);

	// Binds user defined functions for reshaping and displaying windows.
	glutReshapeFunc(UResizeWindow);
	glutDisplayFunc(URenderGraphics);
}

void UResizeWindow (int Width, int Height) {
	glViewport(0, 0, Width, Height);
}

void URenderGraphics (void) {
	// Clear screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Creates a triangle.
	GLuint totalVertices = 6; // triangles have three vertices, two triangles = 6 vertices.
	glDrawArrays(GL_TRIANGLES, 0, totalVertices);

	// Flips front and back buffers.
	glutSwapBuffers();
}

void UCreateVBO (void) {
	// Sets vertex coordinates.
	GLfloat verts[] = {
		// Triangle one.
		-1.f, 1.f,			// Coordinates
		1.f, 0.f, 0.f, 1.f, // Red

		-1.f, 0.f,			// Coordinates
		0.f, 1.f, 0.f, 1.f, // Green

		-0.5f, 0.f,			// Coordinates
		0.f, 0.f, 1.f, 1.f, // Blue

		//Triangle two.
		-0.5f, 0.f,			// Coordinates
		0.f, 0.f, 1.f, 1.f, // Blue

		0.f, 0.f,			// Coordinates
		0.f, 1.f, 0.f, 1.f, // Green

		0.f, -1.f,			// Coordinates
		1.f, 0.f, 0.f, 1.f	// Red
	};

	// Stores number of bytes in verts.
	float numVertices = sizeof(verts);

	// VBO id.
	GLuint myBufferID;
	// Creates buffer.
	glGenBuffers(1, &myBufferID);
	// Activates the buffer.
	glBindBuffer(GL_ARRAY_BUFFER, myBufferID);
	// Sends data to GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices, verts, GL_STATIC_DRAW);

	// Creates Vertex Attribute Pointer.
	GLuint floatsPerVertex = 2; // x coordinate and y coordinate.
	glEnableVertexAttribArray(0); // Sets initial position of coordinates in buffer.

	// Strides between vertex coordinates is 6
	GLint vertexStride = sizeof(float) * 6;

	// Tells GPU how to handle VBO.
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, vertexStride, 0);

	glEnableVertexAttribArray(1); // Sets initial position of rgba in buffer.
	// Strides between vertex color codes is 6
	GLint colorStride = sizeof(float) * 6;
	// Tells GPU how to handle coloring.
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, colorStride, (char*) (sizeof(float) * 2));
}

void UCreateShaders (void) {
	// Create shader program object.
	GLuint ProgramId = glCreateProgram();

	// Create Vertex shader object.
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrieves shader and frag source code.
	glShaderSource(vertexShaderId, 1, &VertexShader, NULL);
	glShaderSource(fragmentShaderId, 1, &FragmentShader, NULL);

	// Compiles shader source code.
	glCompileShader(vertexShaderId);
	glCompileShader(fragmentShaderId);

	// Attach shaders to program.
	glAttachShader(ProgramId, vertexShaderId);
	glAttachShader(ProgramId, fragmentShaderId);

	// Link and use program.
	glLinkProgram(ProgramId);
	glUseProgram(ProgramId);
}
