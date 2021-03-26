#include <iostream>
#include <mpi.h>
#include <queue>
#include <zconf.h>

using namespace std;

static int procs_id;

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

void send_while(queue<unsigned char> queue, int queue_id, MPI_Request* request){

    unsigned char* send;
    while(!queue.empty()){
        *send = queue.front();
        queue.pop();
        MPI_Isend(send,1, MPI_UNSIGNED_CHAR, procs_id+1, queue_id, MPI_COMM_WORLD,request);
        cout << "DEBUG: SEND CISLO " << send << "RANK POSIELA : " << procs_id << endl;
    }
}

/* middle processors */
void merge(unsigned count){
    unsigned max_queue_len;
    unsigned queue_id = 0; // to which queue send next
    unsigned processed_elements = 0, processed_q1 = 0, processed_q2 = 0;

    queue<unsigned char> queue1, queue2;

    unsigned char element;
    unsigned char send_element;
    MPI_Status recv_status;


    max_queue_len = 1 << (procs_id-1);

    /*for(int i = 0; i < count; i++) {
        MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id-1, QUEUE_1, MPI_COMM_WORLD, &recv_status);
        //queue2.push(element);
        //MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id-1, 1, MPI_COMM_WORLD, &recv_status);
        printf("DEBUG: RECV CISLO %d RANK DOSTAL: %d\n", element, procs_id);
    }*/

    unsigned max_elements = max_queue_len*2;
    while(processed_elements < count){
        if(queue1.size() < max_queue_len) {
            for (int i = 0; i < max_queue_len; i++) {
                MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id - 1, QUEUE_1, MPI_COMM_WORLD, &recv_status);
                printf("DEBUG: TAG 0 RECV CISLO %d RANK DOSTAL: %d\n", element, procs_id);
                queue1.push(element);
            }
        }

        if(queue2.empty()) {
            MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id - 1, QUEUE_2, MPI_COMM_WORLD, &recv_status);
            queue2.push(element);
            printf("DEBUG:  TAG 1 RECV CISLO %d RANK DOSTAL: %d\n", element, procs_id);
        }

        if(queue1.size() <= max_queue_len && queue2.size() == 1) {
            for(unsigned m = 0; m < max_elements; m++ ){
                if(queue1.front() > queue2.front()) {
                    send_element = queue1.front();
                    queue1.pop();
                    processed_q1++;
                }
                else {
                    send_element = queue2.front();
                    queue2.pop();
                    processed_q2++;
                }
                MPI_Send(&send_element, 1, MPI_UNSIGNED_CHAR, procs_id + 1, queue_id % QUEUE_COUNT, MPI_COMM_WORLD);
                printf("DEBUG:  TAG %d SEND CISLO %d RANK POSIELA: %d\n",queue_id% QUEUE_COUNT, send_element, procs_id);
            }
            queue_id++;
            processed_elements += processed_q1 + processed_q2;
        }
    }

}

int main(int argc, char** argv) {
    int size;
    static const unsigned count = 8; //TODO zobrat ako parameter? popr spocitat zo suboru
    unsigned char buffer[count];
    FILE *fp;
    char filename[] = "numbers";
    //queue<unsigned char> input_seq;

    // MPI INIT
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // Get the id of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &procs_id);

    // read file to queue
    fp = fopen(filename,"rb");
    if (!fp) {
        perror("fopen");
        return -1;
    }
    fread(buffer, sizeof(unsigned char),count,fp);
    fclose(fp);


    if (procs_id == 0){
        int send;
        for(unsigned char i : buffer) {
            printf("%d ", i);
        }
        cout << endl;
        for(int i = count-1; i >= 0; i--) {
            send = buffer[i];
            MPI_Send(&send, 1, MPI_UNSIGNED_CHAR, procs_id + 1, (i+1) % QUEUE_COUNT, MPI_COMM_WORLD);
        }
    }
    else {
        printf("RANK %d \n", procs_id);
        merge(count);
    }

    // check if number of processors is according to log(count)/log(2) + 1
    //pipeline_merge_sort(input_seq, count);

    MPI_Barrier(MPI_COMM_WORLD);
    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
