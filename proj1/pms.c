#include <stdio.h>
#include <mpi.h>

int main(int argc, char** argv) {

    int size, procs_id;
    static const size_t BufferSize = 16;
    unsigned char buffer[BufferSize];
    FILE *fp;

    // read file
    fp = fopen("../numbers","rb");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    fread(buffer, sizeof(unsigned char),BufferSize,fp);
    for(int i = 0; i<BufferSize; i++)
        printf("%d ", buffer[i]);

    MPI_Init(NULL, NULL);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Get the id of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &procs_id);


    /* first processor will write unordered sequence to output
     * sorted sequence is written one by one in each line in ascending order
     */

    fclose(fp);
    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
