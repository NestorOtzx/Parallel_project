#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#define MAXR 200
#define MAXC 200

int mat[MAXR][MAXC]; //matriz usada para indicar si una celda está ocupada o no

using namespace std;

int GEN_PROC_RABBITS; //number of generations until a rabbit can procreate
int GEN_PROC_FOXES; //number of generations until a fox can procreate
int GEN_FOOD_FOXES; //number of generations for a fox to die of starvation
int N_GEN; //number of generations for the simulation
int R; //number of rows of the matrix representing the ecosystem
int C; //number of columns of the matrix representing the ecosystem
int N; //number of objects in the initial ecosystem

stringstream readinput()
{
    string filename;
    cout<<"Ingrese el nombre del archivo: ";
    getline(cin, filename);
    ifstream file(filename);
    while (!file) {
        if (!file)
        {
            cerr << "No se pudo abrir el archivo:" << filename << endl;
        }
        cout<<"Ingrese el nombre del archivo: ";
        getline(cin, filename);
        ifstream file(filename);
    }
    stringstream buffer;
    buffer << file.rdbuf();  // Leer todo el contenido del archivo al stringstream
    return buffer;
}

int main() {
    
    stringstream input = readinput();
    input>>GEN_PROC_RABBITS>>GEN_PROC_FOXES>>GEN_FOOD_FOXES>>N_GEN>>R>>C>>N;
    cout<<"DATOS LEIDOS: \n"<<GEN_PROC_RABBITS<<" "<<GEN_PROC_FOXES<<" "<<GEN_FOOD_FOXES<<" "<<N_GEN<<" "<<R<<" "<<C<<" "<<N<<endl;
    for (int n = 0; n<N; n++)
    {
        string type;
        int posx, posy;
        input>>type>>posx>>posy;
        cout<<type<<" "<<posx<<" "<<posy<<" "<<endl;
    }
    return 0;
}
