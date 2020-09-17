#ifndef __CONTROLLER_HH__
#define __CONTROLLER_HH__

#include "Bank.h"
#include "Queue.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//Hoang Nguyen GEGE
// ./Main sample.workload 2 DRAM FCFS
extern void initBank(Bank *bank);

// Queue operations
extern Queue* initQueue();
extern void pushToQueue(Queue *q, Request *req);
extern void migrateToQueue(Queue *q, Node *_node);
extern void deleteNode(Queue *q, Node *node);

// CONSTANTS
static unsigned MAX_WAITING_QUEUE_SIZE = 64;
// static unsigned BLOCK_SIZE = 128; // cache block size
//Proj2
static unsigned BLOCK_SIZE = 64; // cache block size
static unsigned NUM_OF_BANKS = 16; // number of banks
static unsigned NUM_OF_CHANNELS = 4; // 4 channels/controllers in total

static unsigned CLEARING_INTERVAL = 10000;
static unsigned BLACKLIST_THRES = 4;

// DRAM Timings
const char *DRAM = "DRAM";
static unsigned DRAM_NCLKS_CHANNEL = 15;
static unsigned DRAM_NCLKS_READ = 53;
static unsigned DRAM_NCLKS_WRITE = 53;

//PCM Timings
const char *PCM = "PCM";
static unsigned PCM_NCLKS_READ = 57;
static unsigned PCM_NCLKS_WRITE = 162;

static const char *FCFS = "FCFS";
static const char *FRFCFS = "FRFCFS";
static const char *BLISS = "BLISS";



typedef struct BlackList
{
    int prev_coreid;

    uint64_t req_served;

    int blacklist_th;

    Node *n;

    int *blist;

}BlackList;


// Controller definition
typedef struct Controller
{
    // The memory controller needs to maintain records of all bank's status
    Bank *bank_status;

    // Current memory clock
    uint64_t cur_clk;

    // Channel status
    uint64_t channel_next_free;

    // A queue contains all the requests that are waiting to be issued.
    Queue *waiting_queue;

    // A queue contains all the requests that have already been issued but are waiting to complete.
    Queue *pending_queue;

    // Queue for shorter run time to calculate total latency. Only push to this queue reqs with latency > 0
    // Queue *late_queue;

    // Queue for request that encouters bank conflict
    // Queue *conflict_queue;

    unsigned numb_banks;
    unsigned crtl_id;

    /* For decoding */
    unsigned bank_shift;
    uint64_t bank_mask;
    
    int debug;
    void (*servletPtr)();

    unsigned nclks_read;
    unsigned nclks_write;
    BlackList* bl;
    // uint64_t* latency;
    uint64_t* accumulated;
    uint64_t* last;
    uint64_t req_served;
    // uint64_t req_stuck;
}Controller;


BlackList* initBL(){
    BlackList* bl = (BlackList *)malloc(sizeof(BlackList));
    // bl->n = (Node *)malloc(sizeof(Node));
    // bl->n->core_id = -1;
    bl->req_served = 0;
    bl->blacklist_th = BLACKLIST_THRES;
    bl->prev_coreid = -1;
    bl->blist = (int *)malloc(8 * sizeof(int));
    // memset(bl->blist, 0, 8);
    bl->blist[0] = 0;
    bl->blist[1] = 0;
    bl->blist[2] = 0;
    bl->blist[3] = 0;
    bl->blist[4] = 0;
    bl->blist[5] = 0;
    bl->blist[6] = 0;
    bl->blist[7] = 0;
    return bl;
}


// If the#Requests Servedexceeds aBlacklisting Threshold(setto 4 in most of our evaluations):
    // The application with IDApplication IDis blacklisted(classified as interference-causing).
    // The#Requests Servedcounter is reset to zero.
void checkTh(BlackList* bl){
    if (bl->req_served > bl->blacklist_th){
        // printf("Served value is ""%"PRIu64"\n", bl->req_served);
        bl->blist[bl->prev_coreid] = 1;
        bl->req_served = 0;
    }
    
}


//If the application IDs of the two requests are thesame, the #Requests Served counter is incremented.
// If the application IDs of the two requests are not thesame, the#Requests Servedcounter is reset to zero
    // and theApplication IDregister is updated with theapplication ID of the request that is being issued.
void checkBl(Node* node, Controller* ctrl){
    if ((ctrl->cur_clk % CLEARING_INTERVAL) == 0){
        // printf("---------------------------------CLEANING-------------------------------------\n");
        ctrl->bl->req_served = 0;
        ctrl->bl->blacklist_th = BLACKLIST_THRES;
        ctrl->bl->prev_coreid = -1;
        ctrl->bl->blist[0] = 0;
        ctrl->bl->blist[1] = 0;
        ctrl->bl->blist[2] = 0;
        ctrl->bl->blist[3] = 0;
        ctrl->bl->blist[4] = 0;
        ctrl->bl->blist[5] = 0;
        ctrl->bl->blist[6] = 0;
        ctrl->bl->blist[7] = 0;

        
    }
    if (ctrl->bl->prev_coreid == node->core_id){
        ctrl->bl->req_served++;
    } else {
        ctrl->bl->req_served = 0;
        ctrl->bl->prev_coreid = node->core_id;
        // bl->n = node;
    }
    checkTh(ctrl->bl);
}


bool isBl(Node* n, Controller* ctrl){
    // printf("Blist value of core %d is ""%"PRIu64"\n", n->core_id, ctrl->bl->blist[n->core_id]);

    if (ctrl->bl->blist[n->core_id] != 0){

        return true;
    }
    return false;
}


// Costs:
    // one register to storeApplication ID
    // one counter for#Requests Served
    // one register to store the Blacklisting Threshold that determines  when  an  application  should  beblacklisted 
    // a blacklist bit vector to indicate the blacklist status ofeach application (one bit for each hardware context
    // determine when an application’s#Requests Served exceeds the Blacklisting Threshold 
        // and set the bit corre-sponding to the application in the Blacklistbit vector.
    // prioritize non-blacklisted applications’ requests.

void chooseServlet(Controller * controller, const char* SERVE_TYPE);


Controller *initController(unsigned NUM_BANKS, const char* MEM_TYPE, const char* SERVE_TYPE, unsigned _ctrl_id, int debug)
{
    if (NUM_BANKS > 32){
        printf("Received %u banks, DOES NOT SUPPORT MORE THAN 16 BANKS\n", NUM_BANKS);
        exit(1);
    }
    Controller *controller = (Controller *)malloc(sizeof(Controller));
    controller->bl = initBL();
    controller->debug = debug;
    chooseServlet(controller, SERVE_TYPE);

    controller->numb_banks = NUM_BANKS;
    controller->bank_status = (Bank *)malloc(NUM_BANKS * sizeof(Bank));
    // controller->latency = (uint64_t *)malloc(8 * sizeof(uint64_t));
    // // memset(controller->latency, 0, 8);
    // controller->latency[0] = 0;
    // controller->latency[1] = 0;
    // controller->latency[2] = 0;
    // controller->latency[3] = 0;
    // controller->latency[4] = 0;
    // controller->latency[5] = 0;
    // controller->latency[6] = 0;
    // controller->latency[7] = 0;
    
    controller->accumulated = (uint64_t *)malloc(8 * sizeof(uint64_t));
    controller->accumulated[0] = 0;
    controller->accumulated[1] = 0;
    controller->accumulated[2] = 0;
    controller->accumulated[3] = 0;
    controller->accumulated[4] = 0;
    controller->accumulated[5] = 0;
    controller->accumulated[6] = 0;
    controller->accumulated[7] = 0;

    controller->last = (uint64_t *)malloc(8 * sizeof(uint64_t));
    controller->last[0] = 0;
    controller->last[1] = 0;
    controller->last[2] = 0;
    controller->last[3] = 0;
    controller->last[4] = 0;
    controller->last[5] = 0;
    controller->last[6] = 0;
    controller->last[7] = 0;
    
    controller->channel_next_free = 0;
    controller->crtl_id = _ctrl_id;
    controller->req_served = 0;
    // controller->req_stuck = 0;
    // printf("Initiated controller with %u banks\n", NUM_BANKS);
    if (strcmp(MEM_TYPE, DRAM) == 0){
        controller->nclks_read  = DRAM_NCLKS_READ;
        controller->nclks_write = DRAM_NCLKS_WRITE;
    }
    else if (strcmp(MEM_TYPE, PCM) == 0){
        controller->nclks_read  = PCM_NCLKS_READ;
        controller->nclks_write = PCM_NCLKS_WRITE;
    }
    else
    {
        assert(false);
    }
    
    for (int i = 0; i < NUM_BANKS; i++)
    {
        initBank(&((controller->bank_status)[i]));
    }
    controller->cur_clk = 0;

    controller->waiting_queue = initQueue();
    controller->pending_queue = initQueue();
    // controller->late_queue = initQueue();
    // controller->conflict_queue = initQueue();
    // controller->bank_shift = log2(BLOCK_SIZE);
    // controller->bank_mask = (uint64_t)NUM_BANKS - (uint64_t)1;
    //Proj2
    controller->bank_shift = log2(BLOCK_SIZE) + log2(NUM_OF_CHANNELS);
    controller->bank_mask = (uint64_t)NUM_OF_BANKS - (uint64_t)1;

    return controller;
}


unsigned ongoingPendingRequests(Controller *controller)
{
    // printf("----Controller.h/ongoingPendingRequests")
    unsigned num_requests_left = controller->waiting_queue->size + 
                                 controller->pending_queue->size;

    return num_requests_left;
}


bool send(Controller *controller, Request *req)
{
    // printf("----Controller.h/send")
    if (controller->waiting_queue->size == MAX_WAITING_QUEUE_SIZE)
    {
        return false;
    }

    // Decode the memory address
    req->bank_id = ((req->memory_address) >> controller->bank_shift) & controller->bank_mask;
    req->latency = 0;
    // Push to queue
    pushToQueue(controller->waiting_queue, req);

    return true;
}


bool noConflict(Controller* controller, Node* first){
    int target_bank_id = first->bank_id;
    if ((controller->bank_status)[target_bank_id].next_free <= controller->cur_clk && 
            controller->channel_next_free <= controller->cur_clk){ 
        return true;
    }
    return false;
}

bool waitingToPendingJob (Controller *controller, Node *first){
    int target_bank_id = first->bank_id;
    // if ((controller->bank_status)[target_bank_id].next_free <= controller->cur_clk)
    // {

    //Proj2
    if (noConflict(controller, first))
    {
        first->begin_exe = controller->cur_clk;
        if (first->req_type == READ)
        {
            first->end_exe = first->begin_exe + (uint64_t)controller->nclks_read;
        }
        else if (first->req_type == WRITE)
        {
            first->end_exe = first->begin_exe + (uint64_t)controller->nclks_write;
        }
        // The target bank is no longer free until this request completes.
        (controller->bank_status)[target_bank_id].next_free = first->end_exe;

        //Proj2
        controller->channel_next_free = controller->cur_clk + DRAM_NCLKS_CHANNEL;

        checkBl(first, controller);
        migrateToQueue(controller->pending_queue, first);
        controller->accumulated[first->core_id] = controller->cur_clk - controller->last[first->core_id] + controller->accumulated[first->core_id];
        controller->last[first->core_id] = controller->cur_clk;
        // if (first->latency > 0){
        //     controller->latency[first->core_id] += first->latency;
        //     // migrateToQueue(controller->late_queue, first);
        // }
        // if (first->conflict == 1){
        //     migrateToQueue(controller->conflict_queue, first);
        // }
        deleteNode(controller->waiting_queue, first);
        return true;
    }
    return false;
}


Node* pendingToPe(Controller *controller){
    // Step two, serve pending requests
    
    if (controller->pending_queue->size)
    {
        
        Node *first = controller->pending_queue->first;
        if (first->end_exe <= controller->cur_clk)
        {
            // duration = first->end_exe - first->begin_exe;
            if ((controller->debug) && (controller->crtl_id == 0) && (first->core_id == 4))
            {
                printf("Core %d, Controller %d\n", first->core_id, controller->crtl_id);
                // printf("Clk: ""%"PRIu64"\n", controller->cur_clk);
                // printf("Address: ""%"PRIu64"\n", first->mem_addr);
                // printf("Bank ID: %d\n", first->bank_id);
                // printf("Begin execution: ""%"PRIu64"\n", first->begin_exe);
                // printf("End execution: ""%"PRIu64"\n\n", first->end_exe);

                //Proj2
                printf("Clk: ""%"PRIu64"\n", controller->cur_clk);
                fflush(stdout);
                printf("Address: ""%"PRIu64"\n", first->mem_addr);
                printf("Channel ID: %d\n", first->channel_id);
                printf("Bank ID: %d\n", first->bank_id);
                printf("Begin execution: ""%"PRIu64"\n", first->begin_exe);
                printf("End execution: ""%"PRIu64"\n\n", first->end_exe);
            }
            Node* out = first;
            deleteNode(controller->pending_queue, first);
            controller->req_served ++;
            return out;

        }
    }
}


// Conductor for FCFS process
void fcfsServlet (Controller *controller) 
{
    // Step two, serve pending requests
    pendingToPe (controller);

    // Step three, find a request to schedule
    if (controller->waiting_queue->size)
    {
        // Implementation One - FCFS
        Node *first = controller->waiting_queue->first;
        int target_bank_id = first->bank_id;
        // uint64_t duration = 0;

        if (!waitingToPendingJob(controller, first)){
            propLatency(controller->waiting_queue);
            first->conflict = 1;
        }
    }
}


// Conductor for FRFCFS process
void frfcfsServlet (Controller *controller) 
{
    // Step two, serve pending requests
    pendingToPe (controller);

    // Step three, find a request to schedule
    if (controller->waiting_queue->size)
    {
        // Implementation One - FCFS
        Node *first = controller->waiting_queue->first;
        // uint64_t duration = 0;
        Node* second = (Node *)malloc(sizeof(Node));
        second = NULL;
        if (!waitingToPendingJob(controller, first)){
            int gotone = 0;
            if (first->next != NULL){
                second = first->next;
            }

            // check if any other instruction in the waiting Queue points to an avaiable bank
            while(second != NULL){  
                
                if (!waitingToPendingJob(controller, second)){
                    second = second->next;
                } else{
                    gotone = 1;
                    break;
                }
            }

            if (gotone == 0){
                propLatency(controller->waiting_queue);
                first->conflict = 1;
            }

        }
    }
    
}


// Conductor for BLISS process
    // if the application IDs of the two requests are thesame, 
    //     the Requests Servedcounter is incremented. 
    // If the application IDs of the two requests are not thesame, 
    //     the Requests Servedcounter is reset to zero
    //     and the Application ID register is updated with the application ID of the request that is being issued.
    // If the RequestsServed exceeds a Blacklisting Threshold(setto 4 in most of our evaluations):
    //     The application with ID ApplicationID is blacklisted(classified as interference-causing).
    //     The RequestsServed counter is reset to zero.
    // The blacklist information is cleared periodically after every Clearing Interval(10,000 cycles in our major evaluations).
void blissServlet (Controller *controller) 
{   
    
    // Step two, serve pending requests
    pendingToPe (controller);
    
    // Step three, find a request to schedule
    if (controller->waiting_queue->size)
    {
        // Implementation One - FCFS
        Node *first = controller->waiting_queue->first;
        int gotone = 0;
        Node* temp = (Node *)malloc(sizeof(Node));
        Node* second = (Node *)malloc(sizeof(Node));
        // uint64_t duration = 0;
        
        temp = first;
        second = first;
        while (second != NULL){
            if (!isBl(second, controller) && noConflict(controller, second)){

                if( controller->bl->blist[second->core_id] != 0){
                    for (int i = 0; i < 8; i++){
                        printf("[%d - %d] ", second->core_id, controller->bl->blist[second->core_id]);
                    }
                    printf("\n");
                    assert(false);
                }
               
                // assert(controller->bl->blist[second->core_id] == 0);
                temp = second;
                gotone = -1;
                break;

            } else if (noConflict(controller, second) && (gotone == 0)) {
                gotone = 1;
                temp = second;
                
                if( controller->bl->blist[second->core_id] != 1){
                    for (int i = 0; i < 8; i++){
                    printf("[%d - %d] ", second->core_id, controller->bl->blist[second->core_id]);
                    }
                    printf("\n");
                    assert(false);
                }
                
                // assert(controller->bl->blist[second->core_id] == 1);
                second = second->next;
            } else {
               
                second = second->next;
            }
            
        }
        // if(gotone == -1){
        //     printf("GEGEGEGE\n");
        //     fflush(stdout);
        // } else if(gotone == 1){
        //     printf("NORENORE\n");
        //     fflush(stdout);
        // }
        if (!waitingToPendingJob(controller, temp)){
            // controller->req_stuck ++;
            propLatency(controller->waiting_queue);
            // first->latency ++;
            // first->conflict = 1;
        }
        
    }
}


void chooseServlet(Controller * controller, const char* SERVE_TYPE) 
{
    if (strcmp(SERVE_TYPE, FCFS) == 0){    
        // printf("Chose FCFS \n");
        controller->servletPtr = &fcfsServlet; 
    }
    else if (strcmp(SERVE_TYPE, FRFCFS) == 0) {
        // printf("Chose FRFCFS \n");
        controller->servletPtr = &frfcfsServlet;
    }
    else if (strcmp(SERVE_TYPE, BLISS) == 0){
        // printf("Chose BLISS \n");
        controller->servletPtr = &blissServlet; 
    } else {
        assert(false);
    }
}


void tick(Controller *controller)
{
    // printf("----Controller.h/tick")
    // Step one, update system stats
    ++(controller->cur_clk);
    // printf("Clk: ""%"PRIu64"\n", controller->cur_clk);
    for (int i = 0; i < controller->numb_banks; i++)
    {
        ++(controller->bank_status)[i].cur_clk;
        // printf("%"PRIu64"\n", (controller->bank_status)[i].cur_clk);
    }
    // printf("\n");

    // uint64_t duration = 0;
    // Step two, serve pending requests
    controller->servletPtr(controller);

}


// We measure fairness using theunfairness indexproposed in [25,8].13
// This is the ratio between the maximum memory-related slow-down and the minimum memory-related slowdown among all threadssharing the DRAM system. 
// The memory related slowdown of a threadi is the memory stall time per instruction it experiences 
        // when running together with other threads 
    // divided by the memory stall time per in-struction it experiences when running alone on the same system.
#endif
