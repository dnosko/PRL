/* Daša Nosková - xnosko05
 *   Mesh Multiplication
 *       PRL 2021
 */

#include <iostream>
#include<string>
#include <fstream>
#include <vector>
#include <mpi.h>

/********************** TAGS **********************/
#define C1 1
#define C2 2
#define R1 3
#define R2 4
#define FIRST_ROWS 5 // first rows
#define FIRST_COLS 6 // first cols

using namespace std;

static int procs_id;
static int world_rank;

vector<int> loadMatrix(string filename, int *rows, int *cols){
    ifstream file;
    file.open(filename);
    if (!file.is_open())
        cout << "Error while opening file";

    string line;
    int number;
    int element;
    vector<int> matrix;

    // read number of rows/cols
    file >> number;

    while (file >> element) {
        //cout << element;
        if (element == '\n')
            continue;
        matrix.push_back(element);
    }

    //int num = (int)(number);
    if (*rows == 0){ // mat1
        *rows = number;
        //cout << *rows << endl;
        *cols = (int) matrix.size() / *rows;
    }
    else{ //mat2
        *cols = number;
        *rows = (int) matrix.size() / *cols;
    }

    file.close();
    return matrix;
}

void distributeMatrixValues(vector<int> matrix, int tag, int rows, int cols, int cols_final){
    int mat_idx;
    int send_to;
    for (int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            mat_idx = j + (i* cols);
            // next process id = j + (i*cols)
            send_to = (tag == FIRST_ROWS) ? (i*cols_final) : j ;
            MPI_Send(&matrix[mat_idx], 1, MPI_INT, send_to, tag, MPI_COMM_WORLD);
        }
    }
}


int main(int argc, char** argv) {
    int count;
    MPI_Request  requests[count];
    MPI_Status recv_status;

    // MPI INIT
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &world_rank);
    // Get the id of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &procs_id);

    int rows_mat1;
    int cols_mat1;
    int rows_mat2;
    int cols_mat2;
    vector<int> matrix1, matrix2;
    int matMul = 0;
    int i_index, j_index, matPos;
    

    if(procs_id == 0) { // first process, load matrixes
        rows_mat1 = 0;
        cols_mat1 = -1;
        matrix1 = loadMatrix("mat1", &rows_mat1, &cols_mat1);
        rows_mat2 = -1;
        cols_mat2 = 0;
        matrix2 = loadMatrix("mat2", &rows_mat2, &cols_mat2);
        for(int i = 1; i < world_rank; i++) {
            MPI_Send(&cols_mat1, 1, MPI_INT, i, C1, MPI_COMM_WORLD);
            MPI_Send(&cols_mat2, 1, MPI_INT, i, C2, MPI_COMM_WORLD);
            MPI_Send(&rows_mat1, 1, MPI_INT, i, R1, MPI_COMM_WORLD);
            MPI_Send(&rows_mat2, 1, MPI_INT, i, R2, MPI_COMM_WORLD);
        }
        //cout << "proc 0" << endl;
    }
    else {
        MPI_Recv(&cols_mat1, 1, MPI_INT, 0, C1, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&cols_mat2, 1, MPI_INT, 0, C2, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&rows_mat1, 1, MPI_INT, 0, R1, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&rows_mat2, 1, MPI_INT, 0, R2, MPI_COMM_WORLD, &recv_status);
    }

    //matPos = j + (i*cols_mat2);

    // send parts of matrix to first rows / cols -> tags firstRows firstCols

    if(procs_id == 0) {
        distributeMatrixValues(matrix1, FIRST_ROWS, rows_mat1, cols_mat1, cols_mat2);
        distributeMatrixValues(matrix2, FIRST_COLS, cols_mat2, rows_mat2, cols_mat2);
    }

    i_index = procs_id / cols_mat2;
    j_index = procs_id % cols_mat2;
    int element = -1;
    if(i_index == 0){
        for(int i = 0; i < rows_mat2; i++){
            MPI_Recv(&element,1,MPI_INT,)
        }
    }
    // recv on first



    return 0;
}
