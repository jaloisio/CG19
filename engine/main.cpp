#ifndef XMLCheckResult
#define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("Error: %i\n", a_eResult); return a_eResult; }
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include <stdlib.h>


#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glew.h>
#include <GL/glut.h>
#endif


#include <cmath>
#include <fstream>
#include <vector>
#include <string>
#include <set>
#include <iostream>
#include "tinyxml2.h"
#include "CatmullRom.h"

using namespace tinyxml2;
using namespace std;

typedef std::string Path;


typedef struct coordinate {
	float x;
	float y;
	float z;
}Vertex;

typedef std::vector<Vertex> ControlP;

static bool operator<(const coordinate &a1, const coordinate &a2) {
    return true;
}
typedef struct pathInfo{
    Path path;
    ControlP contp;
    int nrcontp;
    bool curved;
    float traX=0,traY=0,traZ=0;
    float rotX=0,rotY=0,rotZ=0;
    float scaleX=1,scaleY=1,scaleZ=1;
    float trans_time=0;
    float rot_time=0;
} Paths;

typedef struct groupData{
    float traX, traY,traZ;
    float rotX, rotY, rotZ;
    float scaleX, scaleY, scaleZ;

}Group;

typedef std::vector<float> Model;

typedef struct modelData{
    Model model;
    float traX=0, traY=0,traZ=0;
    float rotX=0, rotY=0, rotZ=0;
    float scaleX=1, scaleY=1, scaleZ=1;
    ControlP contp;
    int nrcontp;
    bool curved;
    float trans_time=0;
    float rot_time=0;
}ModelData;

static bool operator<(const modelData &a1, const modelData &a2) {
    return true;
}

typedef std::set<ModelData> Models;
bool axes = false;
bool clicked = false;
bool lines = false;
float alpha = 0.0;
float beta = 0.0;
float dx = 0.0,dy = 0.0,dz = -1;
float dxP = 1.0,dzP = 0.0;
float px = 0.0,py = 0.0,pz = 20.0;
float k = 1;
int number_of_groups;

Paths paths [1024];

Group groups[1024];
int paths_size = 0;
Models modelz;
GLuint *buffers;
float gloY[3] = {0,1,0};

void readModels(XMLElement* elem, int nr) {
	const char* attr;
	for (XMLElement* aux = elem; aux != nullptr; aux = aux->NextSiblingElement()) {
		string nome_elem = aux->Value();

		if (nome_elem == "model") {
			attr = aux->Attribute("file");//retira o tipo
			if (attr != nullptr) {
				paths[paths_size].path = attr;//adicionar o path do ficheiro
				paths[paths_size].traX = groups[nr].traX; //Adicionar no array paths os valores armazenados
				paths[paths_size].traY = groups[nr].traY; //Adicionar no array paths os valores armazenados
				paths[paths_size].traZ = groups[nr].traZ; //Adicionar no array paths os valores armazenados
				paths[paths_size].rotX = groups[nr].rotX; //Adicionar no array paths os valores armazenados
				paths[paths_size].rotY = groups[nr].rotY; //Adicionar no array paths os valores armazenados
				paths[paths_size].rotZ = groups[nr].rotZ; //Adicionar no array paths os valores armazenados
				paths[paths_size].scaleX = groups[nr].scaleX; //Adicionar no array paths os valores armazenados
				paths[paths_size].scaleY = groups[nr].scaleY; //Adicionar no array paths os valores armazenados
				paths[paths_size].scaleZ = groups[nr].scaleZ; //Adicionar no array paths os valores armazenados
				paths_size++;
			}
		}
	}
    number_of_groups++;

}




void readPoints (XMLElement* elem) {
    const char *attr;
    for (XMLElement *aux = elem; aux != nullptr; aux = aux->NextSiblingElement()) {
        string nome_elem = aux->Value();
        if (nome_elem == "point") {
            Vertex v;
            attr = aux->Attribute("X");
            if (attr != nullptr) v.x = strtof(attr, nullptr);
            attr = aux->Attribute("Y");
            if (attr != nullptr) v.y = strtof(attr, nullptr);
            attr = aux->Attribute("Z");
            if (attr != nullptr) v.z = strtof(attr, nullptr);
            paths[number_of_groups].contp.push_back(v);
            paths[number_of_groups].nrcontp++;

        }
    }
}
void groupAux(XMLElement* elem, int nr){
    const char* attr;//Vai guardar o atributo que queremos
    bool trans,rot,scale;

    trans=rot=scale=false;
    if (nr==0){//Caso seja o primeiro "group" a ser analisado, este tem de ser inicializado, não sendo possível por a scale=0;
        groups[nr].traX=groups[nr].traY=groups[nr].traZ=0;
        groups[nr].rotX=groups[nr].rotY=groups[nr].rotZ=0;
        groups[nr].scaleX=groups[nr].scaleY=groups[nr].scaleZ=1;
    }
    else{//Caso seja um sub-"group" dentro do "group", o sub-"group" vai ter de herdar os valores do "group".
        groups[nr].traX=groups[nr-1].traX;
        groups[nr].traY=groups[nr-1].traY;
        groups[nr].traZ=groups[nr-1].traZ;
        groups[nr].rotX=groups[nr-1].rotX;
        groups[nr].rotY=groups[nr-1].rotY;
        groups[nr].rotZ=groups[nr-1].rotZ;
        groups[nr].scaleX=groups[nr-1].scaleX;
        groups[nr].scaleY=groups[nr-1].scaleY;
        groups[nr].scaleZ=groups[nr-1].scaleZ;
    }

    for(XMLElement* aux = elem;aux!= nullptr;aux = aux->NextSiblingElement()){//itera todos os model
        string nome_elem = aux->Value();

        if (nome_elem == "group") {//Caso seja "group" este iŕa ser um sub-group
             groupAux(aux->FirstChildElement(), nr + 1);

        }
        if(nome_elem=="translate" && !trans) {//Caso seja translate, vai se armazenar os valores
            trans=true;
            attr = aux->Attribute("time");
            if (attr != nullptr && strlen(attr) != 0) {
                readPoints(aux->FirstChildElement());
                paths[number_of_groups].trans_time = strtof(attr, nullptr);
                paths[number_of_groups].curved = true;
            }
            else{
                attr = aux->Attribute("X");//é retirado o atributo de X
                if (attr != nullptr && strlen(attr) != 0) groups[nr].traX += (strtof(attr, nullptr)*groups[nr].scaleX);//string to float

                attr = aux->Attribute("Y");//é retirado o atributo de Y
                if (attr != nullptr && strlen(attr) != 0) groups[nr].traY += (strtof(attr, nullptr)*groups[nr].scaleY);//string to float

                attr = aux->Attribute("Z");//é retirado o atributo de Z
                if (attr != nullptr && strlen(attr) != 0) groups[nr].traZ += (strtof(attr, nullptr)*groups[nr].scaleZ);//string to float
            }
        }
        if(nome_elem=="rotate"&& !rot){//Caso seja rotate, vai se armazenar os valores
            rot= true;
            attr= aux-> Attribute("time");//é retirado o atributo do time
            if (attr!=nullptr && strlen(attr)!=0) paths[number_of_groups].rot_time = strtof(attr,nullptr);//string to floa
			attr= aux-> Attribute("X");//é retirado o atributo de X
            if (attr!=nullptr) if(strlen(attr)!=0) groups[nr].rotX += strtof(attr,nullptr);//string to float
			else groups[nr].rotX = 0;
			attr= aux-> Attribute("Y");//é retirado o atributo de Y
            if(attr!=nullptr) if(strlen(attr)!=0) groups[nr].rotY += strtof(attr,nullptr);//string to float
			else groups[nr].rotY = 0;
			attr= aux-> Attribute("Z");//é retirado o atributo de Z
            if(attr!=nullptr) if(strlen(attr)!=0) groups[nr].rotZ += strtof(attr,nullptr);//string to float
			else groups[nr].rotZ = 0;
		}
        if(nome_elem=="scale"&&!scale){//Caso seja scale, vai se armazenar os valores
            scale=true;
            attr= aux-> Attribute("X");//é retirado o atributo de X
            if (attr != nullptr) {
                if (strlen(attr) != 0) groups[nr].scaleX *= strtof(attr, nullptr);//string to float
            }
            else groups[nr].scaleX *= 1;
            attr= aux-> Attribute("Y");//é retirado o atributo de Y
            if (attr != nullptr) {
                if (strlen(attr) != 0) groups[nr].scaleY *= strtof(attr, nullptr);//string to float
            }
            else groups[nr].scaleY *= 1;
            attr = aux->Attribute("Z");//é retirado o atributo de Z
            if (attr != nullptr) {
                if (strlen(attr) != 0) groups[nr].scaleZ *= strtof(attr, nullptr);//string to float
            }
            else groups[nr].scaleZ *= 1;

        }

		if (nome_elem == "models") {
			readModels(aux->FirstChildElement() ,nr);
		}
    }

}

void readXML(){
    XMLDocument doc ;
    int i = 0;
    doc.LoadFile("../../Files/Config.xml");// carrega o ficheiro XML
    XMLElement* root = doc.FirstChildElement();// Aponta para o elemento raiz= "scene"
    for(XMLElement* elem = root->FirstChildElement();elem != nullptr; elem = elem->NextSiblingElement()) {//precorre os model
        string nome_elem = elem->Value();
        if(nome_elem == "group") {
			groupAux(elem->FirstChildElement(),0);//chama a função auxiliar para poder armazenar a informação de cada grupo
		}
        else{
            paths_size=0;
            return;
        }
    }
    paths_size;
}

void preparaBuffers() {
    int i = 0;
    for(ModelData m : modelz) {
        glBindBuffer(GL_ARRAY_BUFFER,buffers[i]);//Após a crição do buffer, precisamos de ligar o buffer com o devido id antes de este ser usado.
        float *vertexB = &m.model[0];
        glBufferData(GL_ARRAY_BUFFER,sizeof(float) * m.model.size(),vertexB,GL_STATIC_DRAW);// Sendo o buffer inicializado, é possivel copiar os dados para o buffer-
        i++;
    }
}

void loadXML() {
	float x, y, z;
	int i = 0,j=0;
	string c1, c2, c3;
	ModelData model;
	for (i=0;i<paths_size;i++) {

	    Path p = paths[i].path;
		model.traX = paths[i].traX;
        model.traY = paths[i].traY;
        model.traZ = paths[i].traZ;
        model.rotX = paths[i].rotX;
        model.rotY = paths[i].rotY;
        model.rotZ = paths[i].rotZ;
        model.scaleX = paths[i].scaleX;
        model.scaleY = paths[i].scaleY;
        model.scaleZ = paths[i].scaleZ;
        model.curved = paths[i].curved;
        model.trans_time=paths[i].trans_time;
        model.rot_time=paths[i].rot_time;
        model.contp=paths[i].contp;
        model.nrcontp=paths[i].nrcontp;
		ifstream file (p);//leitura do fichero
        if (file.is_open()) {//Caso o ficheiro abra
			while (getline(file, c1) && getline(file, c2) && getline(file, c3)) {// obter as 3 primeiras linha sendo estas as coordenadas(x,y,z)
				x = strtof(c1.c_str(),nullptr);//obter um apontador para o array c1 através do uso c_str
				y = strtof(c2.c_str(),nullptr);//obter um apontador para o array c2 através do uso c_str
				z = strtof(c3.c_str(),nullptr);//obter um apontador para o array c3 através do uso c_str
				model.model.push_back(x);
                model.model.push_back(y);
                model.model.push_back(z);

			}
			file.close();
		}
		modelz.insert(model);
		model.model.clear();
	}

}

void drawTheFiles(float t) {

    float r[3];
    float de[3];
    float p[3];
    float z[3];
    float mad[16];
    int j=0;

    for (ModelData m : modelz) {

        glPushMatrix();
        float controlPoints[m.nrcontp][3];
        for (int i = 0; i < m.nrcontp; i++) {
            controlPoints[i][0] = m.contp[i].x;
            controlPoints[i][1] = m.contp[i].y;
            controlPoints[i][2] = m.contp[i].z;
        }
        if (m.curved) {
            renderCatmullRomCurve(controlPoints, m.nrcontp);
            getGlobalCatmullRomPoint(glutGet(GLUT_ELAPSED_TIME) / (1000 * m.trans_time), p, r, controlPoints,
                                     m.nrcontp);
            *de = *r;
            glTranslatef(p[0], p[1], p[2]);
            normalize_vector(de);
            cross_product(de, gloY, z);
            normalize_vector(z);
            cross_product(z, de, gloY);
            normalize_vector(gloY);
            build_Rotation_Matrix(de, gloY, z, mad);
            glMultMatrixf(mad);
        }

        glTranslatef(m.traX, m.traY, m.traZ);

        Model model = m.model;
        if (!m.curved)
            glTranslatef(m.traX, m.traY, m.traZ);
        glScalef(m.scaleX, m.scaleY, m.scaleZ);
        if (m.rot_time == 0) {
            glRotatef(m.rotX, 1, 0, 0);
            glRotatef(m.rotY, 0, 1, 0);
            glRotatef(m.rotZ, 0, 0, 1);
        } else {
            float alpha = (glutGet(GLUT_ELAPSED_TIME) * 4) / 36;
            glRotatef(m.rotX * alpha, 1, 0, 0);
            glRotatef(m.rotY * alpha, 0, 1, 0);
            glRotatef(m.rotZ * alpha, 0, 0, 1);
        }
        glBindBuffer(GL_ARRAY_BUFFER, buffers[j]);
        glVertexPointer(3, GL_FLOAT, 0, 0);
        glDrawArrays(GL_TRIANGLES, 0, model.size());
        j++;
        glPopMatrix();

    }
}

void drawAxes() {
	glBegin(GL_LINES);
	// Eixo X
	glColor3f(1, 0, 0); //Vermelho
	glVertex3f(0, 0, 0);
	glVertex3f(100, 0, 0);

	// Eixo Y
	glColor3f(1, 1, 0);//Amarelo
	glVertex3f(0, 0, 0);
	glVertex3f(0, 100, 0);

	// Eixo Z
	glColor3f(0.75, 1, 0);//Lima
	glVertex3f(0, 0, 0);
	glVertex3f(0, 0, 100);
	glEnd();
}



void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window with zero width).
	if(h == 0) h = 1;

	// compute window's aspect ratio
	float ratio = w * 1.0 / h;

	// Set the projection matrix as current
	glMatrixMode(GL_PROJECTION);
	// Load Identity Matrix
	glLoadIdentity();

	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);

	// Set perspective
	gluPerspective(45.0f ,ratio, 1.0f ,1000.0f);

	// return to the model view matrix mode
	glMatrixMode(GL_MODELVIEW);
}

void mouseMove(int x, int y) {
	if (clicked) {
		float w = glutGet(GLUT_WINDOW_WIDTH);
		float h = glutGet(GLUT_WINDOW_HEIGHT);

		float cx = x - (w / 2);
		float cy = (y - (h / 2));

		float aStep = cx / w;
		float bStep = cy / h;

		alpha = aStep * (2*M_PI);
		beta = bStep * (2*M_PI);

		if (beta >= (M_PI / 2)) beta = (M_PI / 2) - 0.0001;
		else if (beta <= (M_PI / 2)) beta = -(M_PI / 2) + 0.0001;

		dx = sin(alpha);
		dy = sin(beta);
		dz = -cos(alpha);
		dxP = sin(alpha + (M_PI / 2));
		dzP = -cos(alpha + (M_PI / 2));

		glutPostRedisplay();
		
	}
}


void mouseClick(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		clicked = true;
	}
	else if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) clicked = false;
}


void renderScene() {

    static float t=0;
	//glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// set the camera
	glLoadIdentity();
	gluLookAt(px,py,pz,
		      px + dx,py+dy,pz+dz,
			  0.0f,1.0f,0.0f);

	if (axes) drawAxes();

	if (lines) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

// put drawing instructions here
	drawTheFiles(t);


	// End of frame
	glutSwapBuffers();
}


void process_keys(unsigned char key, int x, int y) {

    switch (key) {
	    case'a':
		    px -= dxP * k;
	    	pz -= dzP * k;
    		glutPostRedisplay();
    		break;
    	case'd':
    		px += dxP * k;
	    	pz += dzP * k;
	    	glutPostRedisplay();
	    	break;
        case'w':
            py += dxP * k;
            glutPostRedisplay();
            break;
        case's':
            py -= dxP * k;
            glutPostRedisplay();
            break;
	    case '+':
	    	px += dx * k;
	    	py += dy * k;
    		pz += dz * k;
    		glutPostRedisplay();
	    	break;
	    case '-':
	    	px -= dx * k;
    		py -= dy * k;
    		pz -= dz * k;
    		glutPostRedisplay();
    		break;
    	case 'f':
		    axes = !axes;
		    break;
	    case 'l':
	    	lines = !lines;
	    	break;
        case 'u':
            dx += 0.5f;
            break;
        case 'j':
            dx -= 0.5f;
            break;
        case 'i':
            dy += 0.5f;
            break;
        case 'k':
            dy -= 0.5f;
            break;

	}
	glutPostRedisplay();
}

int main(int argc, char **argv) {

  readXML();
  loadXML();

  buffers = new GLuint[paths_size];

// put GLUT init here
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(1600, 800);
	glutCreateWindow("Engine");

    glewInit();
    glEnableClientState(GL_VERTEX_ARRAY);// activate vertex position array
    glGenBuffers(paths_size,buffers);//the first one is the number of buffer objects to create, and the second parameter is the address of a GLuint variable or array to store a single ID or multiple IDs
    preparaBuffers();
// put callback registration here
	glutDisplayFunc(renderScene);
    glutIdleFunc(renderScene);
	glutReshapeFunc(changeSize);

	glutKeyboardFunc(process_keys);
	glutMotionFunc(mouseMove);
	glutPassiveMotionFunc(mouseMove);
	glutMouseFunc(mouseClick);

// OpenGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);


// enter GLUT's main loop
	glutMainLoop();

	return 1;
}
