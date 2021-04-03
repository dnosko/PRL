#include <iostream>
#include <mpi.h>
#include <queue>
#include <fstream>

using namespace std;

static int procs_id;
static int world_rank;


#define  QUEUE_COUNT 2 // number of queues


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
    }
}

/* middle processors */
void merge(unsigned count){
    unsigned max_queue_len;
    unsigned queue_id = 0; // to which queue send next
    unsigned processed_elements = 0, processed_q1 = 0, processed_q2 = 0;

    queue<unsigned char> queue1, queue2;

    unsigned char element;
    MPI_Status recv_status;
    auto *requests = (MPI_Request *) calloc(count, sizeof(MPI_Request));

    max_queue_len = 1 << (procs_id-1);

    unsigned recv = 0;
    unsigned last_procs = world_rank -1;

    unsigned tag = 0;
    unsigned change_after = (1 << (procs_id-1));
    unsigned counter = change_after;

    unsigned Q1_send = max_queue_len;
    unsigned Q2_send = max_queue_len;

    bool start_last_process = false;
    while(processed_elements < count){
        //recv elements
        if(recv < count) {

            if(counter == 0) {
                tag = !tag;
                counter = change_after;
            }
            MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_status);

            if(tag == 0){
                queue1.push(element);
            }
            else {
                queue2.push(element);
            }
            recv++;
            counter--;
        }

        /*******************   sending  ********************/
        if(procs_id == last_procs) {
            if(queue1.size() == max_queue_len && queue2.size() == 1) {
                start_last_process = true;
                if (queue1.front() > queue2.front()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                }
                else {
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                }
            }
            else if(start_last_process) {
                if (queue1.empty()) {
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                } else if (queue2.empty()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                } else if (queue1.front() > queue2.front()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                } else if (queue1.front() <= queue2.front()) {
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                }
            }
        }
        else {
            // if there were numbers from previous sequence thne push those
            if(Q1_send == 0 && Q2_send == 0){
                Q1_send = max_queue_len;
                Q2_send = max_queue_len;
            }

            if(Q1_send == 0 && Q2_send != 0) {
                send_data(&queue2, 1, queue_id, requests);
                ++processed_q2;
                --Q2_send;
            }
            else if(Q2_send == 0 && Q1_send != 0) {
                send_data(&queue1, 1, queue_id, requests);
                ++processed_q1;
                --Q1_send;
            }
            else if ((queue1.size() <= max_queue_len && !queue2.empty())) {
                if (queue1.empty()) {
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                    --Q2_send;
                } else if (queue2.empty()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                    --Q1_send;
                }
                else if (queue1.front() > queue2.front()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                    --Q1_send;
                } else {
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                    --Q2_send;
                }
            }

        }

        processed_elements = processed_q1 + processed_q2;

    }


    free(requests);
}

int main(int argc, char** argv) {
    int count ;
    FILE *fp;
    char filename[] = "numbers";

    // get file size
    ifstream in_file(filename, ios::binary);
    in_file.seekg(0, ios::end);
    count = in_file.tellg();

    unsigned char buffer[count];
    MPI_Request  requests[count];

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
            j++;
        }
        MPI_Waitall(count, requests, MPI_STATUSES_IGNORE);
    }
    else {
         merge(count);
    }

    MPI_Barrier(MPI_COMM_WORLD);
    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
