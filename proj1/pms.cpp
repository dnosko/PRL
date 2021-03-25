#include <iostream>
#include <mpi.h>
#include <queue>

using namespace std;

enum {
    QUEUE_1 , //?? TODO od 1 alebo od 0 ??
    QUEUE_2,
    QUEUE_COUNT
};

/* creates reverse queue from array*/
// TODO DELETE
queue<unsigned char> reverse_queue_from_array(unsigned char *array, int numbers){
    queue<unsigned  char> q;
    for(int i=(numbers-1); i>=0; i--) {
        q.push(array[i]);
    }

    return q;
}

/* middle processors */
void merge(int procs_id, unsigned count){
    unsigned max_queue_len;
    unsigned processed_elements = 0;
    queue<unsigned char> queue1, queue2;
    bool firstQueue = true; // first and second queues alternate
    bool receive = true;

    // max len = 2^(id - 1)
    max_queue_len = 1 << (procs_id - 1);
    while (processed_elements < count) {
        // TODO ?? Q1 a Q2 pointre vymena????
        int received = 0;
        unsigned processed_elements_q1 = 0, processed_elements_q2 = 0;
        MPI_Status recv_status;
        unsigned char element;

        // fill queues until conditions for queues are met
        while(receive) {

            MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id -1, received % QUEUE_COUNT, MPI_COMM_WORLD, &recv_status);
            if (firstQueue)
                queue1.push(element);
            else
                queue2.push(element);

            firstQueue = !firstQueue;

            if(queue1.size() < max_queue_len)
                continue;

            if(queue2.empty())
                continue;

            receive = !receive;
        }

        // compare elements and sent greater to next processor
        while(processed_elements_q1 < max_queue_len && processed_elements_q2 < max_queue_len) {
            if(queue1.front() > queue2.front()) {
                //queue_send_n(Q2, 1, Q1_id, send_buffers, mpi_requests);
                processed_elements_q1++;
            }
            else {
                //ueue_send_n(Q1, 1, Q1_id, send_buffers, mpi_requests);
                processed_elements_q2++;
                if(processed_elements_q2 != max_queue_len) {
                    MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id -1, QUEUE_2, MPI_COMM_WORLD, &recv_status);
                    queue2.push(element);
                }
            }
        }

    }


}

/* unordered seq, number of numbers in seq, procs_id  */
void pipeline_merge_sort(queue<unsigned  char> seq, unsigned count, int procs_id){
    int act_number;
    int r;
    MPI_Request send_requests[count];

    if (procs_id == 0) { // first processor
        for (unsigned i = 0; i < count - 1; ++i) {
            act_number = seq.front();
            seq.pop();
            MPI_Isend(&act_number,1, MPI_INT, procs_id+1,i % QUEUE_COUNT, MPI_COMM_WORLD,&send_requests[i] );
        }

        r = MPI_Waitall(count, send_requests, MPI_STATUSES_IGNORE);
        if (r != MPI_SUCCESS)
            MPI_Abort(MPI_COMM_WORLD, -EPIPE);
    }
    else {
        merge(procs_id, count);
    }
}

int main(int argc, char** argv) {
    int size;
    int procs_id;
    static const unsigned count = 16; //TODO zobrat ako parameter? popr spocitat zo suboru
    unsigned char buffer[count];
    FILE *fp;
    queue<unsigned char> input_seq;

    // MPI INIT
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // Get the id of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &procs_id);

    // read file to queue
    fp = fopen("../numbers","rb");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    fread(buffer, sizeof(unsigned char),count,fp);
    fclose(fp);


    if (procs_id == 0){
        for(unsigned char i : buffer)
            printf("%d ", i);
    }

    input_seq = reverse_queue_from_array(buffer, count);

    // check if number of processors is according to log(count)/log(2) + 1
    pipeline_merge_sort(input_seq, count, procs_id);


    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
