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
    unsigned processed_elements = 0;
    queue<unsigned char> queue1, queue2;
    bool firstQueue = true; // first and second queues alternate
    bool receive = true;
    unsigned char *send = nullptr;
    auto *mpi_Request = (MPI_Request *) calloc(count, sizeof(MPI_Request));

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
            cout << "DEBUG: RECV CISLO " << element << "RANK DOSTAL : " << procs_id << endl;
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
            int queue_id;
            queue<unsigned char> act_queue;

            if(queue1.front() > queue2.front()) {
                //queue_send_n(Q2, 1, Q1_id, send_buffers, mpi_requests);
                processed_elements_q1++;
                queue_id = 1;
                act_queue = queue1;
                queue1.pop();
            }
            else {
                //ueue_send_n(Q1, 1, Q1_id, send_buffers, mpi_requests);
                processed_elements_q2++;
                queue_id = 2;
                act_queue = queue2;
                queue2.pop();
                if(processed_elements_q2 != max_queue_len) {
                    MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id -1, QUEUE_2, MPI_COMM_WORLD, &recv_status);
                    queue2.push(element);
                    cout << "DEBUG: RECV CISLO " << element << "RANK DOSTAL : " << procs_id << endl;
                }
            }

            *send = act_queue.front();
            MPI_Isend(send,1, MPI_UNSIGNED_CHAR, procs_id+1, queue_id, MPI_COMM_WORLD,mpi_Request);
            cout << "DEBUG: SEND CISLO " << send << "RANK POSIELA : " << procs_id << endl;

        }

        while(!queue1.empty()) {
            *send = queue1.front();
            queue1.pop();
            MPI_Isend(send,1, MPI_UNSIGNED_CHAR, procs_id+1, 1, MPI_COMM_WORLD,mpi_Request);
            cout << "DEBUG: SEND CISLO " << send << "RANK POSIELA : " << procs_id << endl;
        }

        processed_elements_q2 += queue2.size();
        while(!queue2.empty()){
            *send = queue2.front();
            queue2.pop();
            MPI_Isend(send,1, MPI_UNSIGNED_CHAR, procs_id+1, 2, MPI_COMM_WORLD,mpi_Request);
            cout << "DEBUG: SEND CISLO " << send << "RANK POSIELA : " << procs_id << endl;
        }

        MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id -1, QUEUE_2, MPI_COMM_WORLD, &recv_status);
        queue2.push(element);
        cout << "DEBUG: RECV CISLO " << element << "RANK DOSTAL : " << procs_id << endl;
        processed_elements_q2 += queue2.size();

        send_while(queue2,2, mpi_Request);

        firstQueue = !firstQueue;
        processed_elements += 2*max_queue_len;

    }

    free(mpi_Request);
    free(send);

}

/* unordered seq, number of numbers in seq, procs_id  */
void pipeline_merge_sort(queue<unsigned  char> seq, unsigned count){
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
        merge(count);
    }
}

int main(int argc, char** argv) {
    int size;
    static const unsigned count = 16; //TODO zobrat ako parameter? popr spocitat zo suboru
    unsigned char buffer[count];
    FILE *fp;
    //queue<unsigned char> input_seq;

    // MPI INIT
    MPI_Init(&argc, &argv);
    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    // Get the id of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &procs_id);

    // read file to queue
    fp = fopen("numbers","rb");
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
        for(int i = count-1; i >= 0; i--) {
            send = buffer[i];
            MPI_Send(&send, 1, MPI_UNSIGNED_CHAR, procs_id + 1, 0, MPI_COMM_WORLD);
        }
        cout << endl;
    }
    else {
        //pipeline merge sort -> postupne
        for(int i = 0; i < count; i++) {
            unsigned char element;
            MPI_Status recv_status;
            MPI_Recv(&element, 1, MPI_UNSIGNED_CHAR, procs_id-1, 0, MPI_COMM_WORLD, &recv_status);
            //queue2.push(element);
            printf("DEBUG: RECV CISLO %d RANK DOSTAL: %d\n", element, procs_id);
        }
        //cout << "DEBUG: RECV CISLO " << (unsigned)element << " RANK DOSTAL : " << procs_id << endl;
    }

    //input_seq = reverse_queue_from_array(buffer, count);

    // check if number of processors is according to log(count)/log(2) + 1
    //pipeline_merge_sort(input_seq, count);

    MPI_Barrier(MPI_COMM_WORLD);
    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
