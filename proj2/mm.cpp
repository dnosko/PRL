/* Daša Nosková - xnosko05
 *   Mesh Multiplication
 *       PRL 2021
 */

#include <iostream>
#include<string>
#include <fstream>
#include <vector>
#include <mpi.h>
#include <unistd.h>

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
        printf("in RCV proc %d\n",procs_id);
        MPI_Recv(&element,1,MPI_INT,source,tag,MPI_COMM_WORLD, &recv_status);
        printf("RECV: P %d FROM: %d M: %d RCV\n ", procs_id, source, element);
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
    MPI_Request request;

    int count = 0;
    int unprocessed = cols;

    //printf("COLS SIZE %d\n",cols);
    while(unprocessed) {
        printf("PROCESS %d in MUL JINDEX %d IINDEX %d \n",procs_id, indexJ, indexI);
        if(indexI == 0) { // first row
            //printf("I index: %d sizeof a %zu size of b %zu\n", indexI, row->size(), col->size());
            b = col->at(0);
            //printf("B in first row %d\n",b);
            col->erase(col->begin());
        }
        else {
            prev_proc = (indexJ) + ((indexI-1)*cols);
            printf("A:%d first row: prev_proc %d NEXT_COL \n",a, prev_proc);
            MPI_Recv(&b, 1, MPI_INT, prev_proc, NEXT_ROW, MPI_COMM_WORLD, &recv_status);
            printf("RECV: P %d FROM: %d M: %d MUL B\n ", procs_id, prev_proc, b);
        }

        if(indexJ == 0) { // first column
            //printf("J index: %d \n", indexJ);
            a = row->at(0);
            row->erase(row->begin());
            //printf("A in first col %d\n",a);
        }
        else {
            //cout << "*********************here J" << endl;
            prev_proc = (indexJ-1) + ((indexI)*cols);
            //printf("B:%d first col: prev_proc %d NEXT_ROW \n",b, prev_proc);
            MPI_Recv(&a, 1, MPI_INT, prev_proc, NEXT_COL, MPI_COMM_WORLD, &recv_status);
            printf("RECV: P %d FROM: %d M: %d MUL A\n ", procs_id, prev_proc, a);
        }

        mul = mul + (a*b);
        unprocessed--;
        printf("P %d: muliplying %d * %d += %d\n",procs_id, a,b,mul);

        //printf("%d: prvy riadok: A=%d\n",procs_id,a);
        //printf("%d: prvy stlpec: B=%d\n",procs_id,b);

        int next_proc;
        printf("index i %d index J %d proc %d\n",indexI, indexJ,procs_id);
        if (indexJ < cols-1) {
            next_proc = (indexJ+1) + (indexI*cols);
            if(next_proc >= world_rank) break;
            printf("next_proc %d\n", next_proc);
            printf("SEND: P %d TO: %d M: %d MUL A\n", procs_id, next_proc, a);
            MPI_Send(&a, 1, MPI_INT, next_proc, NEXT_COL, MPI_COMM_WORLD);
            //MPI_Isend(&a, 1, MPI_INT, next_proc, NEXT_COL, MPI_COMM_WORLD, &request);
        }
        if (indexI < rows-1) {
            next_proc = indexJ + ((indexI+1)*cols);
            if(next_proc >= world_rank) break;
            printf("SEND: P %d TO: %d M: %d MUL B\n", procs_id, next_proc, b);
            //MPI_Isend(&b, 1, MPI_INT, next_proc, NEXT_ROW, MPI_COMM_WORLD, &request);
            MPI_Send(&b, 1, MPI_INT, next_proc, NEXT_ROW, MPI_COMM_WORLD);
        }
    }

    return mul;
}


int main(int argc, char** argv) {

    // MPI INIT
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &world_rank);
    // Get the id of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &procs_id);

    //cout << "proc" << procs_id << endl;
    MPI_Request  requests[world_rank];
    MPI_Status recv_status;

    int rows_mat1;
    int cols_mat1;
    int rows_mat2;
    int cols_mat2;
    vector<int> matrix1, matrix2;
    vector<int> col, row; // each  processor has only one column and one row of values
    vector<int> matMul;
    int i_index, j_index, matPos;
    

    if(procs_id == 0) { // first process, load matrixes
        rows_mat1 = 0;
        cols_mat1 = -1;
        matrix1 = loadMatrix("mat1", &rows_mat1, &cols_mat1);
        /*** DEBUG
        matrix1.push_back(1);
        matrix1.push_back(2);
        matrix1.push_back(3);
        matrix1.push_back(4);
        rows_mat1 = 2;
        cols_mat1 = 2;
         ******/
        rows_mat2 = -1;
        cols_mat2 = 0;

        matrix2 = loadMatrix("mat2", &rows_mat2, &cols_mat2);
        /*************
        matrix2.push_back(5);
        matrix2.push_back(6);
        matrix2.push_back(7);
        matrix2.push_back(8);
        rows_mat2 = 2;
        cols_mat2 = 2;
        **********/
        for(int i = 1; i < world_rank; i++) {
            MPI_Send(&cols_mat1, 1, MPI_INT, i, C1, MPI_COMM_WORLD);
            MPI_Send(&cols_mat2, 1, MPI_INT, i, C2, MPI_COMM_WORLD);
            MPI_Send(&rows_mat1, 1, MPI_INT, i, R1, MPI_COMM_WORLD);
            MPI_Send(&rows_mat2, 1, MPI_INT, i, R2, MPI_COMM_WORLD);
        }


        //MPI_Waitall(world_rank, requests, MPI_STATUSES_IGNORE);
    }
    else {
        MPI_Recv(&cols_mat1, 1, MPI_INT, 0, C1, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&cols_mat2, 1, MPI_INT, 0, C2, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&rows_mat1, 1, MPI_INT, 0, R1, MPI_COMM_WORLD, &recv_status);
        MPI_Recv(&rows_mat2, 1, MPI_INT, 0, R2, MPI_COMM_WORLD, &recv_status);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    cout << "AFET BAR *******************" << procs_id << endl;

    //matPos = j + (i*cols_mat2);

    // send parts of matrix to first rows / cols -> tags firstRows firstCols
    cout << "PROCESS" << procs_id << endl;
    i_index = countIindex(procs_id, cols_mat2);
    j_index = countJindex(procs_id, cols_mat2);

    if (procs_id == 0) {
        int mat_idx;
        int r = 0;
        int matsize1 = rows_mat1*cols_mat1;
        int matsize2 = rows_mat2*cols_mat2;
        MPI_Request  req_mat1[matsize1], req_mat2[matsize2];
        for (int i = 0; i < rows_mat1; i++) {
            for (int j = 0; j < cols_mat1; j++) {
                mat_idx = j + (i * cols_mat1);
                // next process id = j + (i*cols)
                //cout << "here";
                //cout << matrix1[mat_idx] << endl;
                //send_to = (tag == FIRST_ROWS) ? (i*cols_final) : j ;
                if(i*cols_mat2 == 0) {
                    //mat_idx = i*
                    //printf("**** ROW 0: %d idx %d \n", matrix1[mat_idx], mat_idx);
                    row.push_back(matrix1[mat_idx]);
                    --matsize1;
                    continue;
                }
                printf("COLS MAT2 %d\n",cols_mat2);
                //MPI_Send(&matrix1[mat_idx], 1, MPI_INT, i * cols_mat2, FIRST_ROWS, MPI_COMM_WORLD);
                MPI_Isend(&matrix1[mat_idx], 1, MPI_INT, i * cols_mat2, FIRST_ROWS, MPI_COMM_WORLD, &req_mat1[--matsize1]);
                printf("SEND: P %d TO: %d M: %d\n", procs_id, i*cols_mat2, matrix1[mat_idx]);
                r++;
            }
        }

        for (int i = 0; i < cols_mat2; i++) {
            for (int j = 0; j < rows_mat2; j++) {
                mat_idx = i + (j * cols_mat2);
                // next process id = j + (i*cols)
                //printf("**** COLS %d: %d idx %d \n", i,matrix2[mat_idx], mat_idx);
                //send_to = (tag == FIRST_ROWS) ? (i*cols_final) : j ;
                if(i == 0) {
                    //printf("**** COL 0: %d idx %d \n", matrix2[mat_idx], mat_idx);
                    col.push_back(matrix2[mat_idx]);
                    --matsize2;
                    continue;
                }
                //MPI_Send(&matrix2[mat_idx], 1, MPI_INT, j, FIRST_COLS, MPI_COMM_WORLD);
                MPI_Isend(&matrix2[mat_idx], 1, MPI_INT, i, FIRST_COLS, MPI_COMM_WORLD, &req_mat2[--matsize2]);
                printf("SEND: P %d TO: %d M: %d\n", procs_id, i, matrix2[mat_idx]);
            }
        }
        //distributeMatrixValues(&matrix1, FIRST_ROWS, rows_mat1, cols_mat1, cols_mat2);
        //distributeMatrixValues(&matrix2, FIRST_COLS, cols_mat2, rows_mat2, cols_mat2);
        MPI_Waitall(matsize1, req_mat1, MPI_STATUSES_IGNORE);
        MPI_Waitall(matsize2, req_mat2, MPI_STATUSES_IGNORE);

    }
    else {
        if (i_index == 0) {
            printf("**** IINDEX RCV COLS: PROCS %d \n",procs_id);
            // TODO ostatne procesy predbiehaju ten prvy takze ked ziara recv tak este nebol send a je deadlock
            col = receiveDistibutedMatrix(FIRST_COLS, rows_mat2, recv_status, 0);
        }
        if (j_index == 0) {
            printf("**** JINDEX RCV ROWS: PROCS %d \n",procs_id);
            row = receiveDistibutedMatrix(FIRST_ROWS, cols_mat1, recv_status, 0);
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    cout << "SECOND BAR ************************" << endl;

    int unprocessed = cols_mat2 * rows_mat1;
    //printf("P %d i: %d j: %d\n",procs_id, i_index, j_index );
    /*while(unprocessed) {
        if (i_index == 0) {
            // TODO ostatne procesy predbiehaju ten prvy takze ked ziara recv tak este nebol send a je deadlock
            row = receiveDistibutedMatrix(FIRST_ROWS, rows_mat2, recv_status, 0);
            printf("P %d rows size %zu \n",procs_id, row.size() );
        }

        if (j_index == 0)
            col = receiveDistibutedMatrix(FIRST_COLS, cols_mat1, recv_status, 0);
        //cout << "proces " << procs_id << endl;
        multiply(rows_mat1, cols_mat2, &row, &col);
        unprocessed = 0;
        cout << "proces afte mul " << procs_id << endl;
    }*/
    //while(procs_id == 3) continue;
    //cout << "HERE";
    //printf("size rows %zu, cols %zu\n", row.size(),col.size());
    int mul = multiply(rows_mat1, cols_mat2, &row, &col);
    cout << "proces afte mul " << procs_id << endl;
    MPI_Barrier(MPI_COMM_WORLD);
    matMul.push_back(mul);
    if(procs_id != 0){
        MPI_Send(&mul,1,MPI_INT,0,10, MPI_COMM_WORLD);
    }
    else {
        for(int i = 1; i < world_rank; i++){
            //cout << "WAIT " << endl;
            MPI_Recv(&mul, 1, MPI_INT, i, 10, MPI_COMM_WORLD, &recv_status);
            matMul.push_back(mul);
        }
        for(int i = 0; i< matMul.size(); i++){
            cout << matMul[i];
            if ((i+1) % cols_mat2 == 0) cout << "\n";
            else cout << " ";
        }
    }

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
