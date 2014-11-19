// NOTE: This definition forces GLM to use radians (not degrees) for ALL of its
// angle arguments. The documentation may not always reflect this fact.
// YOU SHOULD USE THIS IN ALL FILES YOU CREATE WHICH INCLUDE GLM
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "glew/glew.h"
#include <GL/glut.h>

#include <fstream>
#include <iostream>
#include <string>
#include <time.h>

#include "Geometry.h"
#include "Sphere.h"
#include "Cylinder.h"
#include "Cube.h"
#include "Scene.h"
#include "Node.h"

#include "tests.h"


static const float PI = 3.141592653589f;


// Vertex arrays needed for drawing
unsigned int vboPos;
unsigned int vboCol;
unsigned int vboNor;
unsigned int vboIdx;

// Attributes
unsigned int locationPos;
unsigned int locationCol;
unsigned int locationNor;

// Uniforms
unsigned int unifModel;
unsigned int unifModelInvTr;
unsigned int unifViewProj;
unsigned int unifLightPos;
unsigned int unifLightColor;

// Needed to compile and link and use the shaders
unsigned int shaderProgram;

// Window dimensions, change if you want a bigger or smaller window
unsigned int windowWidth = 640;
unsigned int windowHeight = 480;

// Animation/transformation stuff
clock_t old_time;
float rotation = 0.0f;

// TESTING GEOMETRY
Geometry* geometry;
void sampleDrawSphere(glm::mat4 model);

// Helper function to read shader source and put it in a char array
// thanks to Swiftless
std::string textFileRead(const char*);

// Some other helper functions from CIS 565 and CIS 277
void printLinkInfoLog(int);
void printShaderInfoLog(int);
void printGLErrorLog();

// Standard glut-based program functions
void init(void);
void resize(int, int);
void display(void);
void keypress(unsigned char, int, int);
void mousepress(int button, int state, int x, int y);
void cleanup(void);

void initShader();
void cleanupShader();

// modified from CIS 277
void sampleUploadSquare();
void sampleDrawSquare(glm::mat4 model);

// traverse scene graph
Scene* scene = new Scene;
void traverse(Node* n, glm::mat4x4 t);
// draw functions for different geometries
void draw(Node*, glm::mat4);
// used for ordered node selection
vector<Node*> selection_stack;
void preorder(Node*);


int main(int argc, char** argv)
{
	// create scene
	std::string filename;
	std::cout << "Enter file name: ";
	std::cin >> filename;
	const char * c = filename.c_str();
	scene->readFile(c);

    glutInit(&argc, argv);
    // Use RGBA double buffered window
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Scene Graph");

    glewInit();
	
	// RUN TESTS
	RunTests();

    init();
    glutDisplayFunc(display);
    glutReshapeFunc(resize);
    glutKeyboardFunc(keypress);
    glutMouseFunc(mousepress);
    glutIdleFunc(display);

    glutMainLoop();
    return 0;
}


void traverse(Node* n, glm::mat4x4 t) {
	t = t * n->transformation_matrix;

	if(n->geo != NULL) {     // if n.child[i] points to geometry
		draw(n, t);
	}

    for (unsigned int i = 0; i < n->children.size(); i++) {
        traverse(n->children.at(i), t);
    }

}

void preorder(Node* n) {
    for (unsigned int i = 0; i < n->children.size(); i++) {
        preorder(n->children.at(i));
    }
	selection_stack.push_back(n);
}

void init()
{
    // Create the VBOs and vboIdx we'll be using to render images in OpenGL
    glGenBuffers(1, &vboPos);
    glGenBuffers(1, &vboCol);
    glGenBuffers(1, &vboNor);
    glGenBuffers(1, &vboIdx);

    // Set the color which clears the screen between frames
    glClearColor(0, 0, 0, 1);
    // Enable and clear the depth buffer
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);
    glDepthFunc(GL_LEQUAL);

    // Set up our shaders here
    initShader();

    // example for uploading data for drawing.
    //sampleUploadSquare();

	// ------------------------------------------------
	// PREORDAL TRAVERSAL FOR HIGHLIGHTING NODES FEATURE
	// get root node
	Node* root = scene->nodes.at(0);
	for (unsigned int i = 0; i < scene->nodes.size(); i++) {
		if (scene->nodes.at(i)->parent == NULL) {
			root = scene->nodes.at(i);
			break;
		}
	}
	//if there are other nodes with no parent, set their parent to root
	for (unsigned int i = 0; i < scene->nodes.size(); i++) {
		if (scene->nodes.at(i)->parent == NULL && scene->nodes.at(i) != root) 
			scene->nodes.at(i)->parent = root;
	}
	// traverse the nodes of the scene
	preorder(root);

	selection_stack.back()->highlight = true;
	selection_stack.pop_back();
	// -------------------------------------------------

    resize(windowWidth, windowHeight);
    old_time = clock();
}

void initShader()
{
    // Read in the shader program source files
    std::string vertSourceS = textFileRead("shaders/diff.vert.glsl");
    const char *vertSource = vertSourceS.c_str();
    std::string fragSourceS = textFileRead("shaders/diff.frag.glsl");
    const char *fragSource = fragSourceS.c_str();

    // Tell the GPU to create new shaders and a shader program
    GLuint shadVert = glCreateShader(GL_VERTEX_SHADER);
    GLuint shadFrag = glCreateShader(GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();

    // Load and compile each shader program
    // Then check to make sure the shaders complied correctly
    // - Vertex shader
    glShaderSource    (shadVert, 1, &vertSource, NULL);
    glCompileShader   (shadVert);
    printShaderInfoLog(shadVert);
    // - Diffuse fragment shader
    glShaderSource    (shadFrag, 1, &fragSource, NULL);
    glCompileShader   (shadFrag);
    printShaderInfoLog(shadFrag);

    // Link the shader programs together from compiled bits
    glAttachShader  (shaderProgram, shadVert);
    glAttachShader  (shaderProgram, shadFrag);
    glLinkProgram   (shaderProgram);
    printLinkInfoLog(shaderProgram);

    // Clean up the shaders now that they are linked
    glDetachShader(shaderProgram, shadVert);
    glDetachShader(shaderProgram, shadFrag);
    glDeleteShader(shadVert);
    glDeleteShader(shadFrag);

    // Find out what the GLSL locations are, since we can't pre-define these
    locationPos    = glGetAttribLocation (shaderProgram, "vs_Position");
    locationNor    = glGetAttribLocation (shaderProgram, "vs_Normal");
    locationCol    = glGetAttribLocation (shaderProgram, "vs_Color");
    unifViewProj   = glGetUniformLocation(shaderProgram, "u_ViewProj");
    unifModel      = glGetUniformLocation(shaderProgram, "u_Model");
    unifModelInvTr = glGetUniformLocation(shaderProgram, "u_ModelInvTr");
	unifLightPos   = glGetUniformLocation(shaderProgram, "u_LightPos");
	unifLightColor = glGetUniformLocation(shaderProgram, "u_LightColor");


    printGLErrorLog();
}

void cleanup()
{
    glDeleteBuffers(1, &vboPos);
    glDeleteBuffers(1, &vboCol);
    glDeleteBuffers(1, &vboNor);
    glDeleteBuffers(1, &vboIdx);

    glDeleteProgram(shaderProgram);

    delete geometry;
}

void keypress(unsigned char key, int x, int y)
{
	Node* selected;
	glm::mat4 r;

    switch (key) {
	case 'p':
		scene->Raycast();
		break;

    case 'q':
        cleanup();
        exit(0);
        break;

	case 'n':
		for (int i = 0; i < scene->nodes.size(); i++) 
			scene->nodes.at(i)->highlight = false;
		
		if (selection_stack.size() == 0) {
			Node* root = scene->nodes.at(0);
			for (unsigned int i = 0; i < scene->nodes.size(); i++) {
				if (scene->nodes.at(i)->parent == NULL) {
					root = scene->nodes.at(i);
					break;
				}
			}
			for (unsigned int i = 0; i < scene->nodes.size(); i++) {
				if (scene->nodes.at(i)->parent == NULL && scene->nodes.at(i) != root) 
					scene->nodes.at(i)->parent = root;
			}
			preorder(root);
		}

		selection_stack.back()->highlight = true;
		selection_stack.pop_back();
		display();
		break;

	case 'a':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->translate(-0.5,0,0);
		r = glm::mat4(1.0f);
		r[3][0] -= 0.5;
		selected->translate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'd':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->translate(0.5,0,0);
		r = glm::mat4(1.0f);
		r[3][0] += 0.5;
		selected->translate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'w':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->translate(0,0.5,0);
		r = glm::mat4(1.0f);
		r[3][1] += 0.5;
		selected->translate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 's':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->translate(0,-0.5,0);
		r = glm::mat4(1.0f);
		r[3][1] -= 0.5;
		selected->translate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'e':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->translate(0,0,0.5);
		r = glm::mat4(1.0f);
		r[3][2] += 0.5;
		selected->translate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
    case 'r':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->translate(0,0,-0.5);
		r = glm::mat4(1.0f);
		r[3][2] -= 0.5;
		selected->translate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'x':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->scale(1.5,1,1);
		r = glm::mat4(1.0f);
		r[0][0] *= 1.5;
		selected->scale_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
    case 'X':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->scale(0.5,1,1);
		r[0][0] *= 0.5;
		selected->scale_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'y':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->scale(1,1.5,1);
		r[1][1] *= 1.5;
		selected->scale_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
    case 'Y':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->scale(1,0.5,1);
		r[1][1] *= 0.5;
		selected->scale_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'z':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->scale(1,1,1.5);
		r[2][2] *= 1.5;
		selected->scale_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
    case 'Z':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		//selected->scale(1,1,0.5);
		r[2][2] *= 0.5;
		selected->scale_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'j':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		r = glm::mat4(1.0f);
		r[1][1] = cos(10 * (PI/180.0f));
		r[2][1] = -sin(10 * (PI/180.0f));
		r[1][2] = sin(10 * (PI/180.0f));
		r[2][2] = cos(10 * (PI/180.0f));
		selected->rotate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'J':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		r = glm::mat4(1.0f);
		r[1][1] = cos(-10 * (PI/180.0f));
		r[2][1] = -sin(-10 * (PI/180.0f));
		r[1][2] = sin(-10 * (PI/180.0f));
		r[2][2] = cos(-10 * (PI/180.0f));
		selected->rotate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'k':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		r = glm::mat4(1.0f);
		r[0][0] = cos(10 * (PI/180.0f));
		r[2][0] = sin(10 * (PI/180.0f));
		r[0][2] = -sin(10 * (PI/180.0f));
		r[2][2] = cos(10 * (PI/180.0f));
		selected->rotate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'K':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		r = glm::mat4(1.0f);
		r[0][0] = cos(-10 * (PI/180.0f));
		r[2][0] = sin(-10 * (PI/180.0f));
		r[0][2] = -sin(-10 * (PI/180.0f));
		r[2][2] = cos(-10 * (PI/180.0f));
		selected->rotate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'l':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		r = glm::mat4(1.0f);
		r[0][0] = cos(10 * (PI/180.0f));
		r[1][0] = -sin(10 * (PI/180.0f));
		r[0][1] = sin(10 * (PI/180.0f));
		r[1][1] = cos(10 * (PI/180.0f));
		selected->rotate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'L':
		for (int i = 0; i < scene->nodes.size(); i++) {
			if (scene->nodes.at(i)->highlight == true) 
				selected = scene->nodes.at(i);
		}
		r = glm::mat4(1.0f);
		r[0][0] = cos(-10 * (PI/180.0f));
		r[1][0] = -sin(-10 * (PI/180.0f));
		r[0][1] = sin(-10 * (PI/180.0f));
		r[1][1] = cos(-10 * (PI/180.0f));
		selected->rotate_matrix *= r;
		selected->transformation_matrix = selected->translate_matrix * (selected->rotate_matrix * (selected->scale_matrix));
		break;
	case 'f':
		scene->lightPos[0] += 0.5;
		break;
	case 'F':
		scene->lightPos[0] -= 0.5;
		break;
	case 'g':
		scene->lightPos[1] += 0.5;
		break;
	case 'G':
		scene->lightPos[1] -= 0.5;
		break;
	case 'h':
		scene->lightPos[2] += 0.5;
		break;
	case 'H':
		scene->lightPos[2] -= 0.5;
		break;

	}


    glutPostRedisplay();
}

void mousepress(int button, int state, int x, int y)
{
    // Put any mouse events here
}

void display()
{
    // Clear the screen so that we only see newly drawn images
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	clock_t newTime = clock();
    rotation += 2.5f * (static_cast<float>(newTime - old_time) / static_cast<float>(CLOCKS_PER_SEC));
    old_time = newTime;

    // Create a matrix to pass to the model matrix uniform variable in the
    // vertex shader, which is used to transform the vertices in our draw call.
    // The default provided value is an identity matrix; you'll change this.
    glm::mat4 modelmat = glm::mat4();

    // Make sure you're using the right program for rendering
    glUseProgram(shaderProgram);

    // TODO
    // Draw the two components of our scene separately, for your scenegraphs it
    // will help your sanity to do separate draws for each type of primitive
    // geometry, otherwise your VBOs will get very, very complicated fast
	
	// get root node
	Node* root = scene->nodes.at(0);

	for (unsigned int i = 0; i < scene->nodes.size(); i++) {
		if (scene->nodes.at(i)->parent == NULL) {
			root = scene->nodes.at(i);
			break;
		}
	}
	//if there are other nodes with no parent, set their parent to root
	for (unsigned int i = 0; i < scene->nodes.size(); i++) {
		if (scene->nodes.at(i)->parent == NULL && scene->nodes.at(i) != root) 
			scene->nodes.at(i)->parent = root;
	}
	// traverse the nodes of the scene
	traverse(root, modelmat);


    // Draws the sphere with the specified model transformation matrix
    //sampleDrawSquare(modelmat);

    // Move the rendering we just made onto the screen
    glutSwapBuffers();

    // Check for any GL errors that have happened recently
    printGLErrorLog();
}



void draw(Node* n, glm::mat4 model) {

	Geometry* g = n->geo;

	// upload
	const int VERTICES = g->getVertices().size();
	const int TRIANGLES = g->getIndices().size() / 3;

	static const GLsizei SIZE_POS = sizeof(glm::vec3);
    static const GLsizei SIZE_NOR = sizeof(glm::vec3);
    static const GLsizei SIZE_COL = sizeof(glm::vec3);
    static const GLsizei SIZE_TRI = 3 * sizeof(GLuint);

	// colors
	vector<glm::vec3> cols;
    for (int i = 0; i < VERTICES; i++) {
        if (n->highlight) cols.push_back(glm::vec3(1,1,1));
		else cols.push_back(n->color);
    }

	glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glBufferData(GL_ARRAY_BUFFER, VERTICES * SIZE_POS, &(g->getVertices())[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vboCol);
    glBufferData(GL_ARRAY_BUFFER, VERTICES * SIZE_POS, &(cols)[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vboNor);
    glBufferData(GL_ARRAY_BUFFER, VERTICES * SIZE_POS, &(g->getNormals())[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIdx);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, TRIANGLES * SIZE_TRI, &(g->getIndices())[0], GL_STATIC_DRAW);

	// draw
	glUseProgram(shaderProgram);

	glUniform4f(unifLightPos, scene->lightPos[0], scene->lightPos[1], scene->lightPos[2], 1);
	glUniform4f(unifLightColor, scene->lightColor[0], scene->lightColor[1], scene->lightColor[2], 1);

    const int FACES = g->getIndices().size() / 3;

    glEnableVertexAttribArray(locationPos);
    glEnableVertexAttribArray(locationCol);
    glEnableVertexAttribArray(locationNor);

    glUniformMatrix4fv(unifModel, 1, GL_FALSE, &model[0][0]);

    const glm::mat4 modelInvTranspose = glm::inverse(glm::transpose(model));
    glUniformMatrix4fv(unifModelInvTr, 1, GL_FALSE, &modelInvTranspose[0][0]);

    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glVertexAttribPointer(locationPos, 3, GL_FLOAT, false, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, vboCol);
    glVertexAttribPointer(locationCol, 3, GL_FLOAT, false, 0, NULL);

    glBindBuffer(GL_ARRAY_BUFFER, vboNor);
    glVertexAttribPointer(locationNor, 3, GL_FLOAT, false, 0, NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIdx);

    glDrawElements(GL_TRIANGLES, TRIANGLES * 3, GL_UNSIGNED_INT, 0);

    glDisableVertexAttribArray(locationPos);
    glDisableVertexAttribArray(locationCol);
    glDisableVertexAttribArray(locationNor);

    printGLErrorLog();

}


/*
void sampleUploadSquare()
{
    // Take a close look at how vertex, normal, color, and index informations are created and
    // uploaded to the GPU for drawing. You will need to do something similar to get your
    // scene graph to draw.

    // =========================== Create some data to draw ====================================
    // These four points define where the quad would be BEFORE transformations
    // this is referred to as object-space and it's best to center geometry at the origin for easier transformations.
    // Each vertex is {x,y,z,w} where w is the homogeneous coordinate

    // Number of vertices
    const int VERTICES = 4;
    // Number of triangles
    const int TRIANGLES = 2;

    // Sizes of the various array elements below.
    static const GLsizei SIZE_POS = sizeof(glm::vec3);
    static const GLsizei SIZE_NOR = sizeof(glm::vec3);
    static const GLsizei SIZE_COL = sizeof(glm::vec3);
    static const GLsizei SIZE_TRI = 3 * sizeof(GLuint);

    // Initialize an array of floats to hold our cube's position data.
    // Each vertex is {x,y,z,w} where w is the homogeneous coordinate
    glm::vec3 positions[VERTICES] = {
        glm::vec3(-1, +1, -1),
        glm::vec3(-1, -1, -1),
        glm::vec3(+1, -1, -1),
        glm::vec3(+1, +1, -1),
    };

    // Same as above for the cube's normal data.
    glm::vec3 normals[VERTICES] = {
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, 1),
    };

    // Initialize an array of floats to hold our square's color data
    // Color elements are in the range [0, 1], {r, g, b}
    glm::vec3 colors[VERTICES] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1),
        glm::vec3(1, 1, 0),
    };

    // Initialize an array of six unsigned ints to hold our square's index data
    GLuint indices[TRIANGLES][3] = {
        0, 1, 2,
        0, 2, 3,
    };

    // ================UPLOADING CODE (GENERALLY, ONCE PER CHANGE IN DATA)==============
    // Now we put the data into the Vertex Buffer Object for the graphics system to use
    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    // Use STATIC_DRAW since the square's vertices don't need to change while the program runs.
    // Take a look at STREAM_DRAW and DYNAMIC_DRAW to see when they should be used.
    // Always make sure you are telling OpenGL the right size to make the buffer. Here we need 16 floats.
    glBufferData(GL_ARRAY_BUFFER, VERTICES * SIZE_POS, &positions, GL_STATIC_DRAW);

    // Bind+upload the color data
    glBindBuffer(GL_ARRAY_BUFFER, vboCol);
    glBufferData(GL_ARRAY_BUFFER, VERTICES * SIZE_POS, &colors, GL_STATIC_DRAW);

    // Bind+upload the normals
    glBindBuffer(GL_ARRAY_BUFFER, vboNor);
    glBufferData(GL_ARRAY_BUFFER, VERTICES * SIZE_POS, &normals, GL_STATIC_DRAW);

    // Bind+upload the indices to the GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIdx);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, TRIANGLES * SIZE_TRI, &indices, GL_STATIC_DRAW);

    // Once data is loaded onto the GPU, we are done with the float arrays.
    // For your scene graph implementation, you shouldn't create and delete the vertex information
    // every frame. You would probably want to store and reuse them.
}

void sampleDrawSquare(glm::mat4 model)
{
    // Tell the GPU which shader program to use to draw things
    glUseProgram(shaderProgram);

    // Take a close look at how vertex, normal, color, and index informations
    // are created and uploaded to the GPU for drawing. You will need to do
    // something similar to get your scene graph to draw.
    model = glm::rotate(model, rotation, glm::vec3(0, 0, 1));

    // Number of faces (1 on a square)
    const int FACES = 1;
    // Number of triangles (2 per face)
    const int TRIANGLES = 2 * FACES;

    // =============================== Draw the data that we sent =================================
    // Activate our three kinds of vertex information
    glEnableVertexAttribArray(locationPos);
    glEnableVertexAttribArray(locationCol);
    glEnableVertexAttribArray(locationNor);

    // Set the 4x4 model transformation matrices
    // Pointer to the first element of the array
    glUniformMatrix4fv(unifModel, 1, GL_FALSE, &model[0][0]);
    // Also upload the inverse transpose for normal transformation
    const glm::mat4 modelInvTranspose = glm::inverse(glm::transpose(model));
    glUniformMatrix4fv(unifModelInvTr, 1, GL_FALSE, &modelInvTranspose[0][0]);

    // Tell the GPU where the positions are: in the position buffer (4 components each)
    glBindBuffer(GL_ARRAY_BUFFER, vboPos);
    glVertexAttribPointer(locationPos, 3, GL_FLOAT, false, 0, NULL);

    // Tell the GPU where the colors are: in the color buffer (4 components each)
    glBindBuffer(GL_ARRAY_BUFFER, vboCol);
    glVertexAttribPointer(locationCol, 3, GL_FLOAT, false, 0, NULL);

    // Tell the GPU where the normals are: in the normal buffer (4 components each)
    glBindBuffer(GL_ARRAY_BUFFER, vboNor);
    glVertexAttribPointer(locationNor, 3, GL_FLOAT, false, 0, NULL);

    // Tell the GPU where the indices are: in the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIdx);

    // Draw the elements. Here we are only drawing 2 triangles * 3 vertices per triangle, for a
    // total of 6 elements.
    glDrawElements(GL_TRIANGLES, TRIANGLES * 3, GL_UNSIGNED_INT, 0);

    // Shut off the information since we're done drawing.
    glDisableVertexAttribArray(locationPos);
    glDisableVertexAttribArray(locationCol);
    glDisableVertexAttribArray(locationNor);

    // Check for OpenGL errors
    printGLErrorLog();
}*/

void resize(int width, int height)
{
    // Set viewport
	glViewport(0, 0, scene->width, scene->height);

    // Get camera information
    // Add code here if you want to play with camera settings/ make camera interactive.
    glm::mat4 projection = glm::perspective(scene->fovy, scene->width / (float) scene->height, 0.1f, 100.0f);
    glm::mat4 camera = glm::lookAt(scene->pos, scene->pos + scene->viewDir, scene->viewUp);
    projection = projection * camera;

    // Upload the projection matrix, which changes only when the screen or
    // camera changes
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(unifViewProj, 1, GL_FALSE, &projection[0][0]);

    glutPostRedisplay();
}


std::string textFileRead(const char *filename)
{
    // http://insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html
    std::ifstream in(filename, std::ios::in);
    if (!in) {
        std::cerr << "Error reading file" << std::endl;
        throw (errno);
    }
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void printGLErrorLog()
{
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error " << error << ": ";
        const char *e =
            error == GL_INVALID_OPERATION             ? "GL_INVALID_OPERATION" :
            error == GL_INVALID_ENUM                  ? "GL_INVALID_ENUM" :
            error == GL_INVALID_VALUE                 ? "GL_INVALID_VALUE" :
            error == GL_INVALID_INDEX                 ? "GL_INVALID_INDEX" :
            "unknown";
        std::cerr << e << std::endl;

        // Throwing here allows us to use the debugger stack trace to track
        // down the error.
#ifndef __APPLE__
        // But don't do this on OS X. It might cause a premature crash.
        // http://lists.apple.com/archives/mac-opengl/2012/Jul/msg00038.html
        throw;
#endif
    }
}

void printLinkInfoLog(int prog)
{
    GLint linked;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return;
    }
    std::cerr << "GLSL LINK ERROR" << std::endl;

    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLogLen);

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        glGetProgramInfoLog(prog, infoLogLen, &charsWritten, infoLog);
        std::cerr << "InfoLog:" << std::endl << infoLog << std::endl;
        delete[] infoLog;
    }
    // Throwing here allows us to use the debugger to track down the error.
    throw;
}

void printShaderInfoLog(int shader)
{
    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return;
    }
    std::cerr << "GLSL COMPILE ERROR" << std::endl;

    int infoLogLen = 0;
    int charsWritten = 0;
    GLchar *infoLog;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLen);

    if (infoLogLen > 0) {
        infoLog = new GLchar[infoLogLen];
        // error check for fail to allocate memory omitted
        glGetShaderInfoLog(shader, infoLogLen, &charsWritten, infoLog);
        std::cerr << "InfoLog:" << std::endl << infoLog << std::endl;
        delete[] infoLog;
    }
    // Throwing here allows us to use the debugger to track down the error.
    throw;
}
