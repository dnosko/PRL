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
    return id / cols;
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
        matrix.push_back(element);
    }

    if (*rows == 0){ // mat1
        *rows = number;
        *cols = (int) matrix.size() / *rows;
    }
    else{ //mat2
        *cols = number;
        *rows = (int) matrix.size() / *cols;
    }

    file.close();
    return matrix;
}

vector<int> receiveDistibutedMatrix(int tag, int count, MPI_Status recv_status, int source) {
    int element;
    vector<int> return_vector;
    for(int i = 0; i < count; i++){
        MPI_Recv(&element,1,MPI_INT,source,tag,MPI_COMM_WORLD, &recv_status);
        return_vector.push_back(element);
    }
    return return_vector;
}

int multiply(int process,int rows, int cols, vector<int>* row, vector<int>* col) {
    int mul = 0;
    int a, b;
    int prev_proc;
    int indexJ = countJindex(procs_id, cols);
    int indexI = countIindex(procs_id, cols);

    MPI_Status recv_status;
    int unprocessed = process;

    while(unprocessed) {
        unprocessed--;
        if(indexI == 0) { // first row
            b = col->at(0);
            col->erase(col->begin());
        }
        else {
            prev_proc = (indexJ) + ((indexI-1)*cols);
            MPI_Recv(&b, 1, MPI_INT, prev_proc, NEXT_ROW, MPI_COMM_WORLD, &recv_status);
        }

        if(indexJ == 0) { // first column
            a = row->at(0);
            row->erase(row->begin());
        }
        else {
            prev_proc = (indexJ-1) + ((indexI)*cols);
            MPI_Recv(&a, 1, MPI_INT, prev_proc, NEXT_COL, MPI_COMM_WORLD, &recv_status);
        }

        mul = mul + (a*b);

        int next_proc;

        if (indexJ < cols-1) {
            next_proc = (indexJ+1) + (indexI*cols);
            if(next_proc >= world_rank) break;
            MPI_Send(&a, 1, MPI_INT, next_proc, NEXT_COL, MPI_COMM_WORLD);
        }
        if (indexI < rows-1) {
            next_proc = indexJ + ((indexI+1)*cols);
            if(next_proc >= world_rank) break;
            MPI_Send(&b, 1, MPI_INT, next_proc, NEXT_ROW, MPI_COMM_WORLD);
        }
    }

    return mul;
}

void printToOutput(int rows, int cols, vector<int> mul){
    if(procs_id == 0) {
        printf("%d:%d\n", rows, cols);
        for (int i = 0; i < mul.size(); i++) {
            printf("%d", mul[i]);
            if ((i + 1) % cols == 0) cout << "\n";
            else cout << " ";
        }
    }
}


int main(int argc, char** argv) {

    // MPI INIT
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &world_rank);
    // Get the id of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &procs_id);

    MPI_Request  requests[world_rank];
    MPI_Status recv_status;

    int rows_mat1;
    int cols_mat1;
    int rows_mat2;
    int cols_mat2;
    vector<int> matrix1, matrix2;
    vector<int> col, row; // each  processor has only one column and one row of values
    vector<int> matMul;
    int i_index, j_index;
    

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

    i_index = countIindex(procs_id, cols_mat2);
    j_index = countJindex(procs_id, cols_mat2);

    if (procs_id == 0) {
        int mat_idx;
        int r = 0;
        int matsize1 = rows_mat1*cols_mat1;
        int matsize2 = rows_mat2*cols_mat2;

        for (int i = 0; i < rows_mat1; i++) {
            for (int j = 0; j < cols_mat1; j++) {
                mat_idx = j + (i * cols_mat1);
                if(i*cols_mat2 == 0) { // P0
                    row.push_back(matrix1[mat_idx]);
                    --matsize1;
                    continue;
                }
                MPI_Send(&matrix1[mat_idx], 1, MPI_INT, i * cols_mat2, FIRST_ROWS, MPI_COMM_WORLD);
                r++;
            }
        }

        for (int i = 0; i < rows_mat2; i++) {
            for (int j = 0; j < cols_mat2; j++) {
                mat_idx = j + (i * cols_mat2);

                if(j == 0) {
                    col.push_back(matrix2[mat_idx]);
                    --matsize2;
                    continue;
                }
                MPI_Send(&matrix2[mat_idx], 1, MPI_INT, j, FIRST_COLS, MPI_COMM_WORLD);
            }
        }
    }
    else {
        if (i_index == 0) {
            col = receiveDistibutedMatrix(FIRST_COLS, rows_mat2, recv_status, 0);
        }
        if (j_index == 0) {
            row = receiveDistibutedMatrix(FIRST_ROWS, cols_mat1, recv_status, 0);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    int mul = multiply(cols_mat1, rows_mat1, cols_mat2, &row, &col);

    MPI_Barrier(MPI_COMM_WORLD);

    if(procs_id != 0){
        MPI_Send(&mul,1,MPI_INT,0,10, MPI_COMM_WORLD);
    }
    else {
        matMul.push_back(mul);
        for(int i = 1; i < world_rank; i++){
            MPI_Recv(&mul, 1, MPI_INT, i, 10, MPI_COMM_WORLD, &recv_status);
            matMul.push_back(mul);
        }
    }

    printToOutput(rows_mat1,cols_mat2,matMul);

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
