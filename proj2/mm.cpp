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
#define NEXT_ROW 7
#define NEXT_COL 8

using namespace std;

static int procs_id;
static int world_rank;

int countIindex(int id, int cols){
    return procs_id / cols;
}

int countJindex(int id, int cols){
    return id % cols;
}

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

void distributeMatrixValues(vector<int>* matrix, int tag, int rows, int cols, int cols_final){
    int mat_idx;
    int send_to;
    int count = rows*cols;
    int el;
    MPI_Request request[count];
    for (int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            mat_idx = j + (i* cols);
            //count--;
            // next process id = j + (i*cols)
            //cout << "here" <<endl;
            //cout << matrix[mat_idx] << endl;
            send_to = (tag == FIRST_ROWS) ? (i*cols_final) : j ;
            el = matrix->at(mat_idx);
            MPI_Isend(&matrix->at(mat_idx), 1, MPI_INT, send_to, tag, MPI_COMM_WORLD,request);
        }
    }
}

vector<int> receiveDistibutedMatrix(int tag, int count, MPI_Status recv_status, int source) {
    int element;
    vector<int> return_vector;
    for(int i = 0; i < count; i++){
        MPI_Recv(&element,1,MPI_INT,source,tag,MPI_COMM_WORLD, &recv_status);
        printf("Procesor %d dostal cislo %d od %d \n",procs_id, element, source);
        return_vector.push_back(element);
    }
    return return_vector;
}

int multiply(int rows, int cols, vector<int>* row, vector<int>* col) {
    int mul = 0;
    int a, b;
    int prev_proc;
    int indexJ = countJindex(procs_id, cols);
    int indexI = countIindex(procs_id, cols);

    MPI_Status recv_status;

    for(int i = 0; i < cols; i++) {
        if(!indexI) { // first row
            a = row->at(0);
            row->erase(row->begin());
        }
        else {
            prev_proc = (indexJ-1) + (indexI*cols);
            MPI_Recv(&a, 1, MPI_INT, prev_proc, NEXT_COL, MPI_COMM_WORLD, &recv_status);
        }

        if(!indexJ) { // first column
            b = col->at(0);
            col->erase(col->begin());
        }
        else {
            prev_proc = indexJ + ((indexI-1)*cols);
            MPI_Recv(&b, 1, MPI_INT, prev_proc, NEXT_ROW, MPI_COMM_WORLD, &recv_status);
        }

        mul += a*b;

        printf("%d: prvy riadok: A=%d\n",procs_id,a);
        printf("%d: prvy stlpec: B=%d\n",procs_id,b);

        int next_proc;
        if (indexI < rows -1) {
            next_proc = (indexJ+1) + (indexI*cols);
            MPI_Send(&a, 1, MPI_INT, next_proc, NEXT_COL, MPI_COMM_WORLD);
        }
        if (indexJ < cols -1) {
            next_proc = indexJ + ((indexI+1)*cols);
            MPI_Send(&b, 1, MPI_INT, next_proc, NEXT_ROW, MPI_COMM_WORLD);
        }
    }

    return mul;
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
    vector<int> col, row; // each  processor has only one column and one row of values
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
    }
    else {
        MPI_Recv(&cols_mat1, 1, MPI_INT, 0, C1, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&cols_mat2, 1, MPI_INT, 0, C2, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&rows_mat1, 1, MPI_INT, 0, R1, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&rows_mat2, 1, MPI_INT, 0, R2, MPI_COMM_WORLD, &recv_status);
    }

    //matPos = j + (i*cols_mat2);

    // send parts of matrix to first rows / cols -> tags firstRows firstCols

    if (procs_id == 0) {
        int mat_idx;
        int r = 0;
        for (int i = 0; i < rows_mat1; i++) {
            for (int j = 0; j < cols_mat1; j++) {
                mat_idx = j + (i * cols_mat1);
                // next process id = j + (i*cols)
                cout << "here" << endl;
                cout << matrix1[mat_idx] << endl;
                //send_to = (tag == FIRST_ROWS) ? (i*cols_final) : j ;
                MPI_Isend(&matrix1[mat_idx], 1, MPI_INT, i * cols_mat2, FIRST_ROWS, MPI_COMM_WORLD, requests);
                r++;
            }
        }

        for (int i = 0; i < cols_mat2; i++) {
            for (int j = 0; j < rows_mat2; j++) {
                mat_idx = i + (j * cols_mat2);
                // next process id = j + (i*cols)
                cout << matrix2[mat_idx] << endl;
                //send_to = (tag == FIRST_ROWS) ? (i*cols_final) : j ;
                MPI_Isend(&matrix2[mat_idx], 1, MPI_INT, j, FIRST_COLS, MPI_COMM_WORLD, requests);
            }
        }
        //distributeMatrixValues(&matrix1, FIRST_ROWS, rows_mat1, cols_mat1, cols_mat2);
        //distributeMatrixValues(&matrix2, FIRST_COLS, cols_mat2, rows_mat2, cols_mat2);
    }

    i_index = countIindex(procs_id, cols_mat2);
    j_index = countJindex(procs_id, cols_mat2);

    if(i_index == 0)
        row = receiveDistibutedMatrix(FIRST_ROWS, rows_mat2,recv_status, 0);

    if(j_index == 0)
        col = receiveDistibutedMatrix(FIRST_COLS, cols_mat1,recv_status, 0);

    multiply(rows_mat1, cols_mat2, &row, &col);

    MPI_Barrier(MPI_COMM_WORLD);
    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
