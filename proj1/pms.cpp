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
        //printf("DEBUG: TAG %d SEND CISLO %d RANK POSIELA: %d\n", queue_id, send,
        //       procs_id);
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

    unsigned max_queue_len_next = 1 << (procs_id);


    unsigned recv = 0;
    int compared = -1; // no number is being compared if -1, if else then send this number
    int compared_queue_number = -1; // number of the compared queue from which to send the number
    unsigned last_procs = world_rank -1;
    bool start_cpu = false;


    unsigned next_q1 = 0, next_q2 = 1;
    unsigned take_from_Q1 = 0;
    unsigned max_elements = max_queue_len*2;
    unsigned tag = 0;
    unsigned change_after = (1 << (procs_id-1));
    unsigned counter = change_after;
    unsigned change_tag = -1;

    bool start_processor = false;

    unsigned Q1_send = max_queue_len;
    unsigned Q2_send = max_queue_len;

    bool start_last_process = false;
    while(processed_elements < count){ //TODO zmenit lebo na zaciatku posielam vsetko naraz
        //cout << "############### NEW CYCLE " << cycle << " ################" << endl;
        //recv elements
        if(recv < count) {

            if(counter == 0) {
                tag = !tag;
                counter = change_after;
            }
            //tag = (recv+1) % (1 << (procs_id));
            //cout  <<"COUNTER "<< counter << " change_after " << change_after << " RANK " << procs_id << endl;
            MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id - 1, MPI_ANY_TAG, MPI_COMM_WORLD, &recv_status);
            //printf("DEBUG: TAG %d RECV CISLO %d RANK DOSTAL: %d\n", tag, element, procs_id);

            if(tag == 0){
                queue1.push(element);
            }
            else {
                queue2.push(element);
            }
            recv++;
            counter--;
            //printf("%d Q1FRONT \n", queue1.front());
            //printf("%d Q2FRONT \n", queue2.front());
        }

        /***                  sending                ***/
        if(procs_id == last_procs) {
            if(queue1.size() == max_queue_len && queue2.size() == 1) {
                start_last_process = true;
                if (queue1.front() > queue2.front()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                    take_from_Q1 = QUEUE_2;
                    compared = 1;
                    //compared_queue_number = 2;
                }
                else {
                    //printf("Q1 front %d Q2 front %d\n", queue1.front(), queue2.front());
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                    take_from_Q1 = QUEUE_1;
                    compared = 2;
                    //compared_queue_number = 1;
                }
            }
            else if(start_last_process) {
                if (queue1.empty()) {
                   // cout << "Q1 EMPTY" << endl;
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                } else if (queue2.empty()) {
                    //cout << "Q2 EMPTY" << endl;
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                } else if (queue1.front() > queue2.front()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                    take_from_Q1 = QUEUE_2;
                    compared = 1;
                    //compared_queue_number = 2;
                } else if (queue1.front() < queue2.front()) {
                    send_data(&queue2, 1, queue_id, requests);
                    //printf("Q2 front %d Q1 front %d\n", queue2.front(), queue1.front());
                    ++processed_q2;
                    take_from_Q1 = QUEUE_1;
                    compared = 2;
                    //compared_queue_number = 1;
                }
            }
            //printf("OUT LAST Q2 front %d Q1 front %d\n",queue2.front(), queue1.front());
        }
        else {
            // if there were numbers from previous comparision thne push those
            if(Q1_send == 0 && Q2_send == 0){
                Q1_send = max_queue_len;
                Q2_send = max_queue_len;
            }

            if(Q1_send == 0 && Q2_send != 0) {
                //printf("Q full\n");
                send_data(&queue2, 1, queue_id, requests);
                ++processed_q2;
                --Q2_send;
            }
            else if(Q2_send == 0 && Q1_send != 0) {
                //printf("Q1 full\n");
                send_data(&queue1, 1, queue_id, requests);
                ++processed_q1;
                --Q1_send;
            }
            else if ((queue1.size() <= max_queue_len && !queue2.empty())) {
                start_processor = true;
                //cout << "SEND compare Q2 size " << queue2.size() << endl;
                //printf("SEND compare Q2 size %d,  Q1 size %d\n",queue2.size(), queue1.size());
                if (queue1.empty()) {
                    //cout << "Q1 EMPTY" << endl;
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                    --Q2_send;
                } else if (queue2.empty()) {
                    //cout << "Q2 EMPTY" << endl;
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                    --Q1_send;
                }
                else if (queue1.front() > queue2.front()) {
                    send_data(&queue1, 1, queue_id, requests);
                    ++processed_q1;
                    --Q1_send;
                    take_from_Q1 = QUEUE_2;
                    compared = max_queue_len;
                    //printf("COMPARED %d\n", compared);
                    compared_queue_number = 2;
                } else {
                    send_data(&queue2, 1, queue_id, requests);
                    ++processed_q2;
                    --Q2_send;
                    take_from_Q1 = QUEUE_1;
                    compared = max_queue_len;
                    //printf("COMPARED %d\n", compared);
                    compared_queue_number = 1;
                }
            }

        }

        processed_elements = processed_q1 + processed_q2;

    }


    free(requests);
}

int main(int argc, char** argv) {
    static const unsigned count = 16; //TODO zobrat ako parameter? popr spocitat zo suboru
    unsigned char buffer[count]; //= //{100,231,99,169,124,151,103,12};//{1,5,3,2, 8, 7, 4, 6}; //{12,103,151,124,169,99,231,100};//;;
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

    int cycle = 0;

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
