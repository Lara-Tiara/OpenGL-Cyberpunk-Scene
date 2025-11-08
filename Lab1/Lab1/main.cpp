
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <iostream>

// Macro for indexing vertex buffer
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;
GLuint gVAO[2];
GLuint gVBO[2];
GLuint gProgColor = 0, gProgYellow = 0;
//GLuint gVAO[2], gVBO[2];

// Vertex Shader (for convenience, it is defined in the main here, but we will be using text files for shaders in future)
// Note: Input to this shader is the vertex positions that we specified for the triangle. 
// Note: gl_Position is a special built-in variable that is supposed to contain the vertex position (in X, Y, Z, W)
// Since our triangle vertices were specified as vec3, we just set W to 1.0.
/*
static const char* pVS = "                                                    \n\
#version 330                                                                  \n\
                                                                              \n\
in vec3 vPosition;															  \n\
in vec4 vColor;																  \n\
out vec4 color;																 \n\
                                                                              \n\
                                                                               \n\
void main()                                                                     \n\
{                                                                                \n\
    gl_Position = vec4(vPosition.x, vPosition.y, vPosition.z, 1.0);  \n\
	color = vColor;							\n\
}";

// Fragment Shader
// Note: no input in this shader, it just outputs the colour of all fragments, in this case set to red (format: R, G, B, A).
static const char* pFS = "                                              \n\
#version 330                                                             \n\
																		\n\
in vec4 color;                                                           \n\
out vec4 FragColor;                                                       \n\
                                                                           \n\
void main()                                                                 \n\
{                                                                            \n\
FragColor = color;									 \n\
}";
*/

static const char* pVS =
"#version 330\n"
"layout(location = 0) in vec3 vPosition;\n"
"layout(location = 1) in vec4 vColor;\n"
"out vec4 color;\n"
"void main(){\n"
"    gl_Position = vec4(vPosition, 1.0);\n"
"    color = vColor;\n"
"}\n";

// Fragment shader A: use interpolated color
static const char* pFS_COLOR =
"#version 330\n"
"in vec4 color;\n"
"out vec4 FragColor;\n"
"void main(){ FragColor = color; }\n";

// Fragment shader B: force constant yellow
// (declare the input to keep the linker happy even if unused)
static const char* pFS_YELLOW =
"#version 330\n"
"in vec4 color;\n"
"out vec4 FragColor;\n"
"void main(){ FragColor = vec4(1.0, 1.0, 0.0, 1.0); }\n";

GLuint CompileProgram(const char* vsSrc, const char* fsSrc)
{
	GLuint prog = glCreateProgram();
	auto addShader = [&](const char* src, GLenum type) {
		GLuint sh = glCreateShader(type);
		glShaderSource(sh, 1, &src, nullptr);
		glCompileShader(sh);
		GLint ok = GL_FALSE; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
		if (!ok) {
			char log[1024]; glGetShaderInfoLog(sh, 1024, nullptr, log);
			fprintf(stderr, "Shader compile error: %s\n", log); exit(1);
		}
		glAttachShader(prog, sh);
		glDeleteShader(sh); // safe after attach
		};

	addShader(vsSrc, GL_VERTEX_SHADER);
	addShader(fsSrc, GL_FRAGMENT_SHADER);

	glLinkProgram(prog);
	GLint ok = GL_FALSE; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log);
		fprintf(stderr, "Link error: %s\n", log); exit(1);
	}
	return prog;
}

// Shader Functions- click on + to expand
#pragma region SHADER_FUNCTIONS
static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType)
{
	// create a shader object
    GLuint ShaderObj = glCreateShader(ShaderType);

    if (ShaderObj == 0) {
        fprintf(stderr, "Error creating shader type %d\n", ShaderType);
        exit(0);
    }
	// Bind the source code to the shader, this happens before compilation
	glShaderSource(ShaderObj, 1, (const GLchar**)&pShaderText, NULL);
	// compile the shader and check for errors
    glCompileShader(ShaderObj);
    GLint success;
	// check for shader related errors using glGetShaderiv
    glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar InfoLog[1024];
        glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
        fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
        exit(1);
    }
	// Attach the compiled shader object to the program object
    glAttachShader(ShaderProgram, ShaderObj);
}


GLuint CompileShaders()
{
	//Start the process of setting up our shaders by creating a program ID
	//Note: we will link all the shaders together into this ID
    GLuint shaderProgramID = glCreateProgram();
    if (shaderProgramID == 0) {
        fprintf(stderr, "Error creating shader program\n");
        exit(1);
    }

	// Create two shader objects, one for the vertex, and one for the fragment shader
    AddShader(shaderProgramID, pVS, GL_VERTEX_SHADER);
    AddShader(shaderProgramID, pFS_COLOR, GL_FRAGMENT_SHADER);

    GLint Success = 0;
    GLchar ErrorLog[1024] = { 0 };


	// After compiling all shader objects and attaching them to the program, we can finally link it
    glLinkProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
        exit(1);
	}

	// program has been successfully linked but needs to be validated to check whether the program can execute given the current pipeline state
    glValidateProgram(shaderProgramID);
	// check for program related errors using glGetProgramiv
    glGetProgramiv(shaderProgramID, GL_VALIDATE_STATUS, &Success);
    if (!Success) {
        glGetProgramInfoLog(shaderProgramID, sizeof(ErrorLog), NULL, ErrorLog);
        fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
        exit(1);
    }
	// Finally, use the linked shader program
	// Note: this program will stay in effect for all draw calls until you replace it with another or explicitly disable its use
    glUseProgram(shaderProgramID);
	return shaderProgramID;
}

#pragma endregion SHADER_FUNCTIONS

// VBO Functions - click on + to expand
#pragma region VBO_FUNCTIONS
GLuint generateObjectBuffer(GLfloat vertices[], GLfloat colors[]) {
	GLuint numVertices = 6;
	// Genderate 1 generic buffer object, called VBO
	GLuint VBO;
 	glGenBuffers(1, &VBO);
	// In OpenGL, we bind (make active) the handle to a target name and then execute commands on that target
	// Buffer will contain an array of vertices 
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// After binding, we now fill our object with data, everything in "Vertices" goes to the GPU
	glBufferData(GL_ARRAY_BUFFER, numVertices*7*sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	// if you have more data besides vertices (e.g., vertex colours or normals), use glBufferSubData to tell the buffer when the vertices array ends and when the colors start
	glBufferSubData (GL_ARRAY_BUFFER, 0, numVertices*3*sizeof(GLfloat), vertices);
	glBufferSubData (GL_ARRAY_BUFFER, numVertices*3*sizeof(GLfloat), numVertices*4*sizeof(GLfloat), colors);
return VBO;
}

void linkCurrentBuffertoShader(GLuint shaderProgramID){
	GLuint numVertices = 6;
	// find the location of the variables that we will be using in the shader program
	GLuint positionID = glGetAttribLocation(shaderProgramID, "vPosition");
	GLuint colorID = glGetAttribLocation(shaderProgramID, "vColor");
	// Have to enable this
	glEnableVertexAttribArray(positionID);
	// Tell it where to find the position data in the currently active buffer (at index positionID)
    glVertexAttribPointer(positionID, 3, GL_FLOAT, GL_FALSE, 0, 0);
	// Similarly, for the color data.
	glEnableVertexAttribArray(colorID);
	glVertexAttribPointer(colorID, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(numVertices*3*sizeof(GLfloat)));
}
#pragma endregion VBO_FUNCTIONS


void display(){

	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(gProgColor);
	glBindVertexArray(gVAO[0]);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glUseProgram(gProgYellow);
	glBindVertexArray(gVAO[1]);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBindVertexArray(0);
	glUseProgram(0);
	// NB: Make the call to draw the geometry in the currently activated vertex buffer. This is where the GPU starts to work!
	//glDrawArrays(GL_TRIANGLES, 0, 6);
    glutSwapBuffers();
}


void init()
{
	// Create 3 vertices that make up a triangle that fits on the viewport 
	/*
	GLfloat vertices[] = {
			// Left triangle
			-1.0f, -1.0f, 0.0f,   // bottom left
			 0.0f, -1.0f, 0.0f,   // bottom middle
			-0.5f,  0.5f, 0.0f,   // top left

			 // Right triangle
			 0.0f, -1.0f, 0.0f,   // bottom middle
			 1.0f, -1.0f, 0.0f,   // bottom right
			 0.5f,  0.5f, 0.0f    // top right
	};
	// Create a color array that identfies the colors of each vertex (format R, G, B, A)
	GLfloat colors[] = {
			// Left triangle colors (green, red, blue)
			0.0f, 1.0f, 0.0f, 1.0f,   // vertex 1
			1.0f, 0.0f, 0.0f, 1.0f,   // vertex 2
			0.0f, 0.0f, 1.0f, 1.0f,   // vertex 3

			// Right triangle colors (yellow, cyan, magenta)
			1.0f, 1.0f, 0.0f, 1.0f,   // vertex 4
			0.0f, 1.0f, 1.0f, 1.0f,   // vertex 5
			1.0f, 0.0f, 1.0f, 1.0f    // vertex 6
	};
	*/

	// --- Triangle 1 (left) ---
	GLfloat tri1[] = {
		// pos                    // color (RGBA)
		-1.0f, -1.0f, 0.0f,       0.0f, 1.0f, 0.0f, 1.0f,   // bottom-left (green)
		 0.0f, -1.0f, 0.0f,       1.0f, 0.0f, 0.0f, 1.0f,   // bottom-mid (red)
		-0.5f,  0.5f, 0.0f,       0.0f, 0.0f, 1.0f, 1.0f    // top-left (blue)
	};

	// --- Triangle 2 (right) ---
	GLfloat tri2[] = {
		// pos                    // color (RGBA)
		 0.0f, -1.0f, 0.0f,       1.0f, 1.0f, 0.0f, 1.0f,   // bottom-mid (yellow)
		 1.0f, -1.0f, 0.0f,       0.0f, 1.0f, 1.0f, 1.0f,   // bottom-right (cyan)
		 0.5f,  0.5f, 0.0f,       1.0f, 0.0f, 1.0f, 1.0f    // top-right (magenta)
	};

	// Set up the shaders
	GLuint shaderProgramID = CompileShaders();
	glUseProgram(shaderProgramID);

	GLint posLoc = glGetAttribLocation(shaderProgramID, "vPosition");
	GLint colorLoc = glGetAttribLocation(shaderProgramID, "vColor");

	glGenVertexArrays(2, gVAO);
	glGenBuffers(2, gVBO);

	auto setupVAO = [&](int idx, GLfloat* data) {
		glBindVertexArray(gVAO[idx]);
		glBindBuffer(GL_ARRAY_BUFFER, gVBO[idx]);
		const GLsizei stride = (3 + 4) * sizeof(GLfloat);
		glBufferData(GL_ARRAY_BUFFER, 3 * stride, data, GL_STATIC_DRAW);

		// layout(location=0) -> position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

		// layout(location=1) -> color
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(GLfloat)));

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	};

	setupVAO(0, tri1);
	setupVAO(1, tri2);
	gProgColor = CompileProgram(pVS, pFS_COLOR);
	gProgYellow = CompileProgram(pVS, pFS_YELLOW);
	// Put the vertices and colors into a vertex buffer object
	//generateObjectBuffer(vertices, colors);
	// Link the current buffer to the shader
	//linkCurrentBuffertoShader(shaderProgramID);	
}

int main(int argc, char** argv){

	// Set up the window
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Hello Triangle");
	// Tell glut where the display function is
	glutDisplayFunc(display);

	glewExperimental = GL_TRUE;
	// A call to glewInit() must be done after glut is initialized!
    GLenum res = glewInit();
	// Check for any errors
    if (res != GLEW_OK) {
      fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
      return 1;
    }
	// Set up your objects and shaders
	init();
	// Begin infinite event loop
	glutMainLoop();
    return 0;
}











