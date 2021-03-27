#include <iostream>
#include <mpi.h>
#include <queue>
#include <zconf.h>
#include <assert.h>

using namespace std;

static int procs_id;
static int world_rank;

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

void send_data(queue<unsigned char>* queue, int n, unsigned queue_id, MPI_Request *requests){

    /*  last processor prints to output*/
    if (procs_id == world_rank - 1) {
        for (int i = 0; i < n; ++i) {
            printf("%d\n", (int)  queue->front());
            queue->pop();
        }
        return;
    }

    unsigned char send;
    for(int i = 0; i < n; i++) {
        send = queue->front();
        queue->pop();
        MPI_Isend(&send,1, MPI_UNSIGNED_CHAR, procs_id+1, queue_id, MPI_COMM_WORLD, requests);
        printf("DEBUG:  TAG %d SEND CISLO %d RANK POSIELA: %d\n", queue_id, send,
               procs_id);
    }
}

void print_queue_while_not_empty(queue<unsigned char> *queue){

    while(!queue->empty()){
        printf("%d\n", (int)  queue->front());
        queue->pop();
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
    auto *requests = (MPI_Request *) calloc(count, sizeof(MPI_Request));

    unsigned start_cycle = (1 <<(procs_id -1)) + (procs_id -1); // 2^(i-1) + i -1
    unsigned end_cycle = (count -1) + start_cycle; // (n-1) + 2^(i-1) + i -1

    unsigned end_alg = (1 << (world_rank-1)) + count + (world_rank -1); //algorithm ends in xth cycle

    max_queue_len = 1 << (procs_id-1);

    /*for(int i = 0; i < count; i++) {
        MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id-1, QUEUE_1, MPI_COMM_WORLD, &recv_status);
        //queue2.push(element);
        //MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id-1, 1, MPI_COMM_WORLD, &recv_status);
        printf("DEBUG: RECV CISLO %d RANK DOSTAL: %d\n", element, procs_id);
    }*/

    unsigned recv = 0;
    unsigned compared = 0;
    unsigned last_procs = world_rank -1;
    bool start_cpu = false;
    unsigned queue_id_send = 0;
    unsigned max_elements = max_queue_len*2;
    for(unsigned i = 0; i < end_alg; i++){ //TODO zmenit lebo na zaciatku posielam vsetko naraz
        cout << "NEW CYCLE" << endl << endl;
        //recv elements
        if(recv < count) {
            MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id - 1, queue_id, MPI_COMM_WORLD, &recv_status);
            recv++;
            printf("DEBUG: TAG %d RECV CISLO %d RANK DOSTAL: %d\n", queue_id, element, procs_id);
            if (queue_id == QUEUE_1)
                queue1.push(element);
            else
                queue2.push(element);
            //send_data(&queue1,1, queue_id, requests);
            queue_id = (queue_id + 1) % QUEUE_COUNT;
        }

        // send elements
        if (compared < max_queue_len || procs_id == last_procs){
            //if queeue empty, prinnt from another
            if(procs_id == last_procs ) {
                if (queue1.empty()) {
                    print_queue_while_not_empty(&queue2);
                } else if (queue2.empty()) {
                    print_queue_while_not_empty(&queue1);
                }
            }
            if (numQ1 == 0) {
                send_data(&queue2,1,queue_id, requests);
            }
            else if(numQ2 == 0){
                send_data(&queue1,1,queue_id, requests);
            }
            else {
                if (queue1.front() > queue2.front()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                    queue_id_send = QUEUE_1;
                } else {
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                    queue_id_send = QUEUE_2;
                }
            }
            compared++;
        }
        else {
            if(queue_id_send == QUEUE_1) {
                send_data(&queue2,1, queue_id, requests);
            }
        }
        /*
        if(queue1.size() == max_queue_len && queue2.size() == 1){
            start_cpu = true;
            for(unsigned m = 0; m < max_queue_len; m++) {
                if (queue1.front() > queue2.front()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                } else {
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                }
            }
            // processed += processed + max_queue_len;
        }*/
        /*queue_id_send = (queue_id_send + 1) % QUEUE_COUNT;
        if (start_cpu){
            if(queue2.empty()){
                send_data(&queue1,1,queue_id, requests);
                ++processed_q1;
            }
        }*/
        /*else if(queue2.empty()){
            send_data(&queue1,1,queue_id, requests);
            ++processed_q1;
        }*/



    }
        /*if(queue1.size() < max_queue_len) {
            unsigned Q_size = queue1.size();
            unsigned get_q1 = max_queue_len - Q_size;
            for (unsigned i = 0; i < get_q1; i++) {
                MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id - 1,QUEUE_1, MPI_COMM_WORLD, &recv_status);
                printf("DEBUG: TAG 0 RECV CISLO %d RANK DOSTAL: %d\n", element, procs_id);
                queue1.push(element);
            }
        }

        if(queue2.empty()) {
            MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id - 1, QUEUE_2, MPI_COMM_WORLD, &recv_status);
            queue2.push(element);
            printf("DEBUG:  TAG 1 RECV CISLO %d RANK DOSTAL: %d\n", element, procs_id);
        }*/


    free(requests);
}

int main(int argc, char** argv) {
    static const unsigned count = 4; //TODO zobrat ako parameter? popr spocitat zo suboru
    unsigned char buffer[count];
    FILE *fp;
    char filename[] = "numbers";
    MPI_Request  requests[count];
    //queue<unsigned char> input_seq;

    // MPI INIT
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &world_rank);
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
        int send, j = 0;
        for(unsigned char i : buffer) {
            printf("%d ", i);
        }
        cout << endl;
        for(int i = count-1; i >= 0; i--) {
            send = buffer[i];
            MPI_Isend(&send, 1, MPI_UNSIGNED_CHAR, procs_id + 1, (i+1)% QUEUE_COUNT, MPI_COMM_WORLD, &requests[j]);
            //printf("num %d tag %d \n",send, (i+1)%QUEUE_COUNT);
            j++;
        }
        MPI_Waitall(count, requests, MPI_STATUSES_IGNORE);
    }
    else {
        merge(count);
    }

    // check if number of processors is according to log(count)/log(2) + 1
    //pipeline_merge_sort(input_seq, count);

    MPI_Barrier(MPI_COMM_WORLD);
    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
