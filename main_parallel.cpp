#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <omp.h>
#include <mutex>

#define MAXR 200
#define MAXC 200

using namespace std;

char mat[MAXR][MAXC]; //matriz usada para indicar si una celda está ocupada o no

int GEN_PROC_RABBITS; //number of generations until a rabbit can procreate
int GEN_PROC_FOXES; //number of generations until a fox can procreate
int GEN_FOOD_FOXES; //number of generations for a fox to die of starvation
int N_GEN; //number of generations for the simulation
int R; //number of rows of the matrix representing the ecosystem
int C; //number of columns of the matrix representing the ecosystem
int N; //number of objects in the initial ecosystem


int rabbitsID = 0; //curr max id of a rabbit
int foxesID = 0; //curr max id of a fox

struct Fox{
    int birthgen;
    int lasteat;
    int x;
    int y;
};

struct Rabbit{
    int birthgen;
    int x;
    int y;
};

unordered_map<int, Fox> foxes;
unordered_map<int, Rabbit> rabbits;

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

void printMat(int R, int C, int gen = -1){
    cout<<"Generation "<<gen<<endl;
    for (int i = 0; i<C+2; i++)
    {
        cout<<"-";
    }
    cout<<"   ";
    for (int i = 0; i<C+2; i++)
    {
        cout<<"-";
    }
    cout<<" ";
    for (int i = 0; i<C+2; i++)
    {
        cout<<"-";
    }
    cout<<endl;
    char matc1[R][C];
    char matc2[R][C];
    for (int i = 0; i<R; i++)
    {
        for (int j = 0; j<C; j++)
        {
            matc1[i][j] = mat[i][j];
            matc2[i][j] = mat[i][j];
        }
    }
    for (auto rabbit : rabbits)
    {
        matc1[rabbit.second.x][rabbit.second.y] = '0'+(gen-rabbit.second.birthgen)%(GEN_PROC_RABBITS+1);
        matc2[rabbit.second.x][rabbit.second.y] = 'R';
    }
    for (auto fox : foxes)
    {
        matc1[fox.second.x][fox.second.y] = '0' + (gen-fox.second.birthgen)%(GEN_PROC_FOXES+1);
        matc2[fox.second.x][fox.second.y] = '0' + (gen-1-fox.second.lasteat);
    }

    for (int i = 0; i<R; i++)
    {
        cout<<"|";
        for (int j = 0; j<C; j++)
        {
            cout<<mat[i][j];
        }
        cout<<"|   ";
        cout<<"|";
        for (int j = 0; j<C; j++)
        {
            cout<<matc1[i][j];
        }
        cout<<"| ";
        cout<<"|";
        for (int j = 0; j<C; j++)
        {
            cout<<matc2[i][j];
        }
        cout<<"|"<<endl;
    }
    for (int i = 0; i<C+2; i++)
    {
        cout<<"-";
    }
    cout<<"   ";
    for (int i = 0; i<C+2; i++)
    {
        cout<<"-";
    }
    cout<<" ";
    for (int i = 0; i<C+2; i++)
    {
        cout<<"-";
    }
    cout<<endl;
    cout<<endl;
}

void printData()
{
    cout<<"Foxes:"<<endl;
    for (auto fox : foxes)
    {
        cout<<" pos: ("<<fox.second.x<<", "<<fox.second.y<<") ";
        cout<<"last eat: "<<fox.second.lasteat<<" birth gen: "<<fox.second.birthgen<<endl;
    }
    cout<<"Rabbits:"<<endl;
    for (auto& rabbit : rabbits)
    {
        cout<<" pos: ("<<rabbit.second.x<<", "<<rabbit.second.y<<") ";
        cout<<" birth gen: "<<rabbit.second.birthgen<<endl;
    }
}

void generation(int remaininGens)
{
    int currentGen = N_GEN-remaininGens;
    printMat(R,C, currentGen);

    if (remaininGens == 0)
    {
        cout<<"end of generations"<<endl;
    }else{
        //move rabbits
        vector<array<int, 3>> movements = {};
        vector<int> deleteID = {};
        
        auto rabbitsC = unordered_map<int, Rabbit>(rabbits);
        auto foxesC = unordered_map<int, Fox>(foxes);
        
        #pragma omp parallel for
        for (int i = 0; i < rabbitsC.size(); ++i)
        {
            auto it = next(rabbitsC.begin(), i);
            int id = it->first;
            Rabbit& rabbit = it->second;
            int x = rabbit.x, y = rabbit.y;

            vector<pair<int,int>> dirs = {{x-1, y}, {x, y+1}, {x+1, y}, {x, y-1}};
            vector<pair<int,int>> fdirs;

            for (int d = 0; d < dirs.size(); ++d) {
                int dx = dirs[d].first, dy = dirs[d].second;
                if (dx >= 0 && dy >= 0 && dx < R && dy < C && mat[dx][dy] == ' ') {
                    fdirs.push_back(dirs[d]);
                }
            }

            if (!fdirs.empty()) {
                int chosen = (currentGen + x + y) % fdirs.size();
                int dx = fdirs[chosen].first, dy = fdirs[chosen].second;

                int othermovment = -1;
                #pragma omp critical(movements_read)
                {
                    for (int m = 0; m < movements.size(); ++m) {
                        if (movements[m][1] == dx && movements[m][2] == dy) {
                            othermovment = m;
                            break;
                        }
                    }
                }

                if ((currentGen + 1 - rabbit.birthgen) % (GEN_PROC_RABBITS + 1) == 0) {
                    Rabbit nr;
                    nr.x = x;
                    nr.y = y;
                    nr.birthgen = currentGen + 1;
                    #pragma omp critical(insert_rabbit)
                    {
                        rabbits.insert({rabbitsID, nr});
                        rabbitsID++;
                    }
                }

                if (othermovment >= 0) {
                    #pragma omp critical(update_conflict)
                    {
                        int otherrabbit = movements[othermovment][0];
                        if ((currentGen - rabbit.birthgen) % (GEN_PROC_RABBITS + 1) >
                            (currentGen - rabbits[otherrabbit].birthgen) % (GEN_PROC_RABBITS + 1)) {
                            deleteID.push_back(otherrabbit);  
                            movements[othermovment][0] = id;
                        } else {
                            deleteID.push_back(id);  
                            movements[othermovment][0] = otherrabbit;
                        }
                    }
                } else {
                    #pragma omp critical(push_movement)
                    {
                        movements.push_back({id, dx, dy});
                    }
                }
            }
        }
        //commit movements
        for (int i = 0; i< deleteID.size(); i++)
        {
            //cout<<"delete rabbit in "<<rabbits[deleteID[i]].x<<", "<<rabbits[deleteID[i]].y<<endl;
            rabbits.erase(deleteID[i]);
        }
        for (int i = 0; i < movements.size(); i++)
        {
            Rabbit rabbit = rabbits[movements[i][0]];
            rabbit.x = movements[i][1];
            rabbit.y = movements[i][2];
            rabbits[movements[i][0]] = rabbit;
        }
        //actualizar matriz
        #pragma omp parallel for collapse(2)
        for (int i = 0; i < MAXR; i++) {
            for (int j = 0; j < MAXC; j++) {
                if (mat[i][j] != '*') {
                    mat[i][j] = ' ';
                }
            }
        }

        #pragma omp parallel for
        for (int i = 0; i < foxes.size(); ++i) {
            auto it = next(foxes.begin(), i);
            mat[it->second.x][it->second.y] = 'F';
        }

        #pragma omp parallel for
        for (int i = 0; i < rabbits.size(); ++i) {
            auto it = next(rabbits.begin(), i);
            mat[it->second.x][it->second.y] = 'R';
        }

        //move foxes
        movements = {};
        deleteID = {};
        
        #pragma omp parallel for
        for (int i = 0; i < foxesC.size(); ++i)
        {
            auto it = next(foxesC.begin(), i);
            int id = it->first;
            Fox& fox = it->second;
            int x = fox.x, y = fox.y;

            vector<pair<int,int>> dirs = {{x-1, y}, {x, y+1}, {x+1, y}, {x, y-1}};
            vector<pair<int,int>> fdirs;

            for (int d = 0; d < dirs.size(); ++d) {
                int dx = dirs[d].first, dy = dirs[d].second;
                if (dx >= 0 && dy >= 0 && dx < R && dy < C && mat[dx][dy] == 'R') {
                    fdirs.push_back(dirs[d]);
                }
            }

            if (fdirs.empty()) {
                for (int d = 0; d < dirs.size(); ++d) {
                    int dx = dirs[d].first, dy = dirs[d].second;
                    if (dx >= 0 && dy >= 0 && dx < R && dy < C && mat[dx][dy] == ' ') {
                        fdirs.push_back(dirs[d]);
                    }
                }
            }

            if (!fdirs.empty()) {
                int chosen = (currentGen + x + y) % fdirs.size();
                int dx = fdirs[chosen].first, dy = fdirs[chosen].second;

                int othermovement = -1;
                #pragma omp critical(movement_read)
                {
                    for (int m = 0; m < movements.size(); ++m) {
                        if (movements[m][1] == dx && movements[m][2] == dy) {
                            othermovement = m;
                            break;
                        }
                    }
                }

                if ((currentGen + 1 - fox.birthgen) % (GEN_PROC_FOXES + 1) == 0 &&
                    (currentGen - fox.lasteat < GEN_FOOD_FOXES || mat[dx][dy] == 'R')) 
                {
                    Fox nf;
                    nf.x = x;
                    nf.y = y;
                    nf.birthgen = currentGen + 1;
                    nf.lasteat = currentGen;

                    #pragma omp critical(insert_fox)
                    {
                        foxes.insert({foxesID, nf});
                        foxesID++;
                    }
                }

                if (othermovement >= 0) {
                    #pragma omp critical(conflict_resolution)
                    {
                        int otherfox = movements[othermovement][0];
                        int a = (currentGen - fox.birthgen) % (GEN_PROC_FOXES + 1);
                        int b = (currentGen - foxes[otherfox].birthgen) % (GEN_PROC_FOXES + 1);

                        if (a > b) {
                            deleteID.push_back(otherfox);
                            movements[othermovement][0] = id;
                        } else if (a == b) {
                            if (currentGen - fox.lasteat > currentGen - foxes[otherfox].lasteat) {
                                deleteID.push_back(id);
                                movements[othermovement][0] = otherfox;
                            } else {
                                deleteID.push_back(otherfox);
                                movements[othermovement][0] = id;
                            }
                        } else {
                            deleteID.push_back(id);
                            movements[othermovement][0] = otherfox;
                        }
                    }
                } else {
                    #pragma omp critical(push_movement)
                    {
                        movements.push_back({id, dx, dy});
                    }
                }
            }
        }
        //commit movements
        for (int i = 0; i< deleteID.size(); i++) //cant be parallelized
        {
            //cout<<"delete fox in "<<foxes[deleteID[i]].x<<", "<<foxes[deleteID[i]].y<<endl;
            foxes.erase(deleteID[i]);
        }
        
        for (int i = 0; i < movements.size(); i++)
        {
            Fox fox = foxes[movements[i][0]];
            //if couldn't find nothing to eat and will die if doesn't eat
            if (mat[movements[i][1]][movements[i][2]] != 'R' && currentGen-fox.lasteat >= GEN_FOOD_FOXES)
            {
                continue;
            }
            fox.x = movements[i][1];
            fox.y = movements[i][2];
            if (mat[fox.x][fox.y] == 'R') //kill the rabbit in that cel
            {
                int r;
                for (auto& rabbit : rabbits) //search rabbit object
                {
                    if (rabbit.second.x == fox.x && rabbit.second.y == fox.y)
                    {
                        r = rabbit.first;
                        break;
                    }
                }
                rabbits.erase(r);
                fox.lasteat = currentGen;
            }
            foxes[movements[i][0]] = fox;
        }
        deleteID = {};
        for (auto& fox: foxes) //can be parallelized, but it's not worth it because of deleteID vector
        {
            //cout<<"fox "<<fox.first<<" lasteat: "<<fox.second.lasteat<<" currgen:" << currentGen<<endl;
            if (currentGen-fox.second.lasteat >= GEN_FOOD_FOXES) //fox die of starvation
            {
                //cout<<"will delete "<<fox.first<<endl;
                deleteID.push_back(fox.first);
            }
        }
        for (int i = 0; i<deleteID.size(); i++) //can be parallelized, but it's not worth it because of foxes vector
        {
            Fox f = foxes[deleteID[i]];
            //cout<<"killing fox "<<deleteID[i]<<"x: "<<f.x<<" y: "<<f.y<<endl;
            foxes.erase(deleteID[i]);
        }
        deleteID = {};
        
        //actualizar la matriz
        #pragma omp parallel for collapse(2)
        for (int i = 0; i < MAXR; i++) {
            for (int j = 0; j < MAXC; j++) {
                if (mat[i][j] != '*') {
                    mat[i][j] = ' ';
                }
            }
        }

        #pragma omp parallel for
        for (int i = 0; i < foxes.size(); ++i) {
            auto it = next(foxes.begin(), i);
            mat[it->second.x][it->second.y] = 'F';
        }

        #pragma omp parallel for
        for (int i = 0; i < rabbits.size(); ++i) {
            auto it = next(rabbits.begin(), i);
            mat[it->second.x][it->second.y] = 'R';
        }
        generation(remaininGens-1);
    }
}

int main() {
    for (int i = 0; i<MAXR; i++)
    {
        for (int j = 0; j<MAXC; j++)
        {
            mat[i][j] = ' ';
        }
    }
    
    stringstream input = readinput();
    input>>GEN_PROC_RABBITS>>GEN_PROC_FOXES>>GEN_FOOD_FOXES>>N_GEN>>R>>C>>N;
    //cout<<"DATOS LEIDOS: \n"<<GEN_PROC_RABBITS<<" "<<GEN_PROC_FOXES<<" "<<GEN_FOOD_FOXES<<" "<<N_GEN<<" "<<R<<" "<<C<<" "<<N<<endl;
    int rockcount = 0;
    for (int n = 0; n<N; n++)
    {
        string type;
        int posx, posy;
        input>>type>>posx>>posy;
        //cout<<type<<" "<<posx<<" "<<posy<<" "<<endl;
        if (type == "ROCK")
        {
            mat[posx][posy] = '*';   
            rockcount++;
        }else{
            mat[posx][posy] = type[0];
            if (type[0] == 'R' || type[0] == 'r')
            {
                Rabbit r;
                r.x = posx;
                r.y = posy;
                r.birthgen = 0;
                rabbits.insert({rabbitsID, r});
                rabbitsID++;
            }else{
                Fox f;
                f.x = posx;
                f.y = posy;
                f.birthgen = 0;
                f.lasteat = -1;
                foxes.insert({foxesID, f});
                foxesID++;
            }
        }
    }
    
    generation(N_GEN);
    cout<<GEN_PROC_RABBITS<<" "<<GEN_PROC_FOXES<<" "<<GEN_FOOD_FOXES<<" "<<0<<" "<<R<<" "<<C<<" "<<rockcount+foxes.size()+rabbits.size()<<endl;
    for (int i = 0; i<R; i++)
    {
        for (int j = 0; j<C; j++)
        {
            if (mat[i][j] == '*')
            {
                cout<<"ROCK "<<i<<" "<<j<<endl;
            }else if (mat[i][j] == 'R')
            {
                cout<<"RABBIT "<<i<<" "<<j<<endl;
            }else if (mat[i][j] == 'F')
            {
                cout<<"FOX "<<i<<" "<<j<<endl;
            }
        }
    }
    return 0;
}
