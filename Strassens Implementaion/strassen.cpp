#include <stdlib.h> 
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <cstdlib>
#include <random>

using namespace std;

int** new_matrix(int dimmensions);
int** conventional(int **a, int **b, int dimmensions);
int** strassens_only(int **a, int **b, int dimmensions);
int** strassens_crossover(int **a, int **b, int dimmensions);
int** add(int **a, int **b, int dimmensions);
int** subtract(int **a, int **b, int dimmensions);
int** padding(int **a, int dimmensions, int padded_dimmensions);
double tstamp();
double elapsed(double time);
int** triangles(double probability);
int random(double probability);

//Helpers for generating random values between 0 and 1
std::random_device rd;
std::default_random_engine eng(rd());
std::uniform_real_distribution<float> distr(0, 1);


int main(int argc, char* argv[]) {

    //Get matrix dimmensions and create those sized matricies, 2 starting matricies and 1 resulting matrice
    int matrix_dimmensions = atoi(argv[2]);
    int **matrix1 = new_matrix(matrix_dimmensions);
    int **matrix2 = new_matrix(matrix_dimmensions);
    int** matrix3;


    //Open our input file
    string file_line;
    ifstream input_file (argv[3]);
    int file_number;

    //Start with first line of input file and fill in each of the two matricies in O(n^2) time. 
    if (input_file.is_open()){
        for (int i = 0; i < matrix_dimmensions; i++){
            for (int p = 0; p < matrix_dimmensions; p++){
                std::getline (input_file,file_line);
                file_number = std::stoi (file_line);
                matrix1[i][p] = file_number;
            }
        }

        for (int i = 0; i < matrix_dimmensions; i++){
            for (int p = 0; p < matrix_dimmensions; p++){
                std::getline (input_file,file_line);
                file_number = std::stoi (file_line);
                matrix2[i][p] = file_number;
            }
        }
        input_file.close();
        
    }

    //Use Strassens_only (No crossover if 32 or less. Otherwise use Strassens with crossover point)
    if (matrix_dimmensions > 32){
        matrix3 = strassens_crossover(matrix1,matrix2, matrix_dimmensions);
    }
    else {
        matrix3 = strassens_only(matrix1,matrix2, matrix_dimmensions);

    }


    //Print values diagnally followed by a trailing new line. 
    for (int i = 0; i < matrix_dimmensions; i++){
        printf("%i\n", matrix3[i][i]);
    }
    printf("\n");
}

//Create a new matrice (list of lists) based on given dimmensions and return pointer to that matrice
int** new_matrix(int dimmensions){
    int **a = new int*[dimmensions];

    for (int i = 0; i < dimmensions; i++){
        a[i] = new int[dimmensions];
    }
    return a;
}

//Conventional matrice multiplication (O(n^3))
int** conventional(int **a, int **b, int dimmensions){
    int **c = new_matrix(dimmensions);

    //We will do dimmensions^3 additions and dimmensions^3 multiplications so total is 2(dimmensions^3)
    for (int i = 0; i < dimmensions; i++){
        for (int j = 0; j < dimmensions; j++){
            for (int k = 0; k < dimmensions; k++){
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }

    return c;
}

//Same as strassens_conventional except strassens_crossover starts using conventional at n=32
int** strassens_only(int **a, int **b, int dimmensions){
    //Crossover point to conventional. Set to 2 for Strassens only
    if (dimmensions <= 2){
        return conventional(a,b,dimmensions);
    }
    else {
        if (dimmensions % 2 != 0){
            a = padding(a, dimmensions, dimmensions+1);
            b = padding(b, dimmensions, dimmensions+1);
            dimmensions = dimmensions+1;
        }
        int **c = new_matrix(dimmensions);

        //Cost 1
        int new_dimmensions = dimmensions/2;

        //Cost 8*(n/2)
        int **a00 = new_matrix(new_dimmensions);
        int **a01 = new_matrix(new_dimmensions);
        int **a10 = new_matrix(new_dimmensions);
        int **a11 = new_matrix(new_dimmensions);
        int **b00 = new_matrix(new_dimmensions);
        int **b01 = new_matrix(new_dimmensions);
        int **b10 = new_matrix(new_dimmensions);
        int **b11 = new_matrix(new_dimmensions);


        int m;
        int n;
        for(int i = 0; i < new_dimmensions; i++){
            //Cost n/2
            m = new_dimmensions+i;
            for(int j = 0; j < new_dimmensions; j++){
                //Cost (n/2)^2
                n = new_dimmensions+j;
                a00[i][j] = a[i][j];
                a01[i][j] = a[i][n];
                a10[i][j] = a[m][j];
                a11[i][j] = a[m][n];
                b00[i][j] = b[i][j];
                b01[i][j] = b[i][n];
                b10[i][j] = b[m][j];
                b11[i][j] = b[m][n];
            }
        }

        //Cost 7*previous strassen + 18*(n/2) + 18*3*(n/2)^2
        int **P1 = strassens_only(a00, subtract(b01, b11, new_dimmensions), new_dimmensions);
        int **P2 = strassens_only(add(a00, a01, new_dimmensions), b11, new_dimmensions);
        int **P3 = strassens_only(add(a10, a11, new_dimmensions), b00, new_dimmensions);
        int **P4 = strassens_only(a11, subtract(b10, b00, new_dimmensions), new_dimmensions);
        int **P5 = strassens_only(add(a00, a11, new_dimmensions), add(b00, b11, new_dimmensions), new_dimmensions);
        int **P6 = strassens_only(subtract(a01, a11, new_dimmensions), add(b10, b11, new_dimmensions), new_dimmensions);
        int **P7 = strassens_only(subtract(a00, a10, new_dimmensions), add(b00,b01,new_dimmensions), new_dimmensions);

        int **c00 = subtract(add(add(P5, P4, new_dimmensions), P6, new_dimmensions), P2, new_dimmensions);
        int **c01 = add(P1, P2, new_dimmensions);
        int **c10 = add(P3, P4, new_dimmensions);
        int **c11 = subtract(subtract(add(P5,P1, new_dimmensions), P3, new_dimmensions), P7, new_dimmensions);

        //Cost (n/2)^2+(n/2)
        for (int i = 0; i < new_dimmensions; i++){
            m = new_dimmensions+i;
            for(int j = 0; j < new_dimmensions; j++){
                n = new_dimmensions+j;
                c[i][j] = c00[i][j];
                c[i][n] = c01[i][j];
                c[m][j] = c10[i][j];
                c[m][n] = c11[i][j];
            }
        }
        
        return c;
        //Total cost: 50(n/2)^2+28(n/2)+1
    }
}

//Same as strassens_conventional except strassens_crossover starts using conventional at n=32
int** strassens_crossover(int **a, int **b, int dimmensions){
    //Crossover point to conventional
    if (dimmensions <= 32){
        return conventional(a,b,dimmensions);
    }
    else {
        //Pad our matrices if uneven number
        if (dimmensions % 2 != 0){
            a = padding(a, dimmensions, dimmensions+1);
            b = padding(b, dimmensions, dimmensions+1);
            dimmensions = dimmensions+1;
        }
        int **c = new_matrix(dimmensions);

        //Cost 1
        int new_dimmensions = dimmensions/2;

        //Create 8 sub matrices so each of our two matrices can be divided in 4.
        //Cost 8*(n/2)
        int **a00 = new_matrix(new_dimmensions);
        int **a01 = new_matrix(new_dimmensions);
        int **a10 = new_matrix(new_dimmensions);
        int **a11 = new_matrix(new_dimmensions);
        int **b00 = new_matrix(new_dimmensions);
        int **b01 = new_matrix(new_dimmensions);
        int **b10 = new_matrix(new_dimmensions);
        int **b11 = new_matrix(new_dimmensions);

        //Move values from our two large matrices into the 8 small ones
        int m;
        int n;
        for(int i = 0; i < new_dimmensions; i++){
            //Cost n/2
            m = new_dimmensions+i;
            for(int j = 0; j < new_dimmensions; j++){
                //Cost (n/2)^2
                n = new_dimmensions+j;
                a00[i][j] = a[i][j];
                a01[i][j] = a[i][n];
                a10[i][j] = a[m][j];
                a11[i][j] = a[m][n];
                b00[i][j] = b[i][j];
                b01[i][j] = b[i][n];
                b10[i][j] = b[m][j];
                b11[i][j] = b[m][n];
            }
        }

        //Cost 7*previous strassen + 18*(n/2) + 18*3*(n/2)^2
        int **P1 = strassens_crossover(a00, subtract(b01, b11, new_dimmensions), new_dimmensions);
        int **P2 = strassens_crossover(add(a00, a01, new_dimmensions), b11, new_dimmensions);
        int **P3 = strassens_crossover(add(a10, a11, new_dimmensions), b00, new_dimmensions);
        int **P4 = strassens_crossover(a11, subtract(b10, b00, new_dimmensions), new_dimmensions);
        int **P5 = strassens_crossover(add(a00, a11, new_dimmensions), add(b00, b11, new_dimmensions), new_dimmensions);
        int **P6 = strassens_crossover(subtract(a01, a11, new_dimmensions), add(b10, b11, new_dimmensions), new_dimmensions);
        int **P7 = strassens_crossover(subtract(a00, a10, new_dimmensions), add(b00,b01,new_dimmensions), new_dimmensions);

        int **c00 = subtract(add(add(P5, P4, new_dimmensions), P6, new_dimmensions), P2, new_dimmensions);
        int **c01 = add(P1, P2, new_dimmensions);
        int **c10 = add(P3, P4, new_dimmensions);
        int **c11 = subtract(subtract(add(P5,P1, new_dimmensions), P3, new_dimmensions), P7, new_dimmensions);


        //Put the values we got from our 4 matrices back into the large matrice to send back
        //Cost (n/2)^2+(n/2)
        for (int i = 0; i < new_dimmensions; i++){
            m = new_dimmensions+i;
            for(int j = 0; j < new_dimmensions; j++){
                n = new_dimmensions+j;
                c[i][j] = c00[i][j];
                c[i][n] = c01[i][j];
                c[m][j] = c10[i][j];
                c[m][n] = c11[i][j];
            }
        }
        
        return c;
        //Total cost: 50(n/2)^2+28(n/2)+1
    }
}

//Add one matrice with another. For use with Strassens.
int** add(int **a, int **b, int dimmensions){
    int **c = new_matrix(dimmensions);

    for (int i = 0; i < dimmensions; i++){
        for (int j = 0; j < dimmensions; j++){
            c[i][j] = a[i][j] + b[i][j];
        }
    }

    return c;
}

//Subtract one matrice from another. For use with Strassens
int** subtract(int **a, int **b, int dimmensions){
    int **c = new_matrix(dimmensions);

    for (int i = 0; i < dimmensions; i++){
        for (int j=0; j < dimmensions; j++){
            c[i][j] = a[i][j] - b[i][j];
        }
    }
    return c;
}

//Take a matrice with a certain number of dimmensions and add rows/columns of 0's so it has the padded number of dimmensions
int** padding(int **a, int dimmensions, int padded_dimmension){

    int **b = new_matrix(padded_dimmension);

    for (int i=0; i < dimmensions; i++){
        for (int j=0; j < dimmensions; j++){
            b[i][j] = a[i][j];
        }
    }

    return b;
}

//    Return the current absolute time as a real number of seconds. For comparing times between conventional and strassens.
double tstamp() {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec + now.tv_nsec * 1e-9;
}

//    Return the number of seconds that have elapsed since `time`.
double elapsed(double time) {
    return tstamp() - time;
}


//Create matrice of 1024x1024 and randomly assign edges based on given probability
int** triangles(double probability){
    int **adjacency_matrix = new_matrix(1024);

    for (int i = 0; i < 1024; i++){
        for (int j = 0; j < 1024; j++){
            if (j < i){
                adjacency_matrix[i][j]= adjacency_matrix[j][i];
            }
            else if (i == j){
                adjacency_matrix[i][j] = 0;
            }
            else {
                adjacency_matrix[i][j] = random(probability);
            }
        }
    }
    return adjacency_matrix;
}

//Utilize random number generator to determine 1 or 0 based on probability
int random(double probability){

    //Generate random number 1 to 100
    double number = distr(eng);

    if (number <= probability) 
        return 1; 

    else{
        return 0; 
    }
}