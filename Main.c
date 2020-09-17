#include "Trace.h"

#include "Mem_System.h"

extern TraceParser *initTraceParser(const char * mem_file);
extern bool getRequest(TraceParser *mem_trace);

extern MemorySystem *initMemorySystem();
extern unsigned pendingRequests(MemorySystem *mem_system);
extern bool access(MemorySystem *mem_system, Request *req);
extern void tickEvent(MemorySystem *mem_system);



void setZero(uint64_t* arr){
    for (int i = 0; i < 8; i++){
        arr[i] = 0;
        // printf("%d ", arr[i]);
    }
    // printf("\n");
}


double max(double *arr){
    double m = 0;
    for (int i = 0; i < 8; i++) 
        if (arr[i] > m) 
            m = arr[i]; 
    return m;
}


double min(double *arr){
    double m = arr[0];
    for (int i = 1; i < 8; i++) 
        if (arr[i] < m) 
            m = arr[i]; 
    return m;
}


int main(int argc, const char *argv[])
{	
    if (argc != 2)
    {
        printf("Usage: %s %s\n", argv[0], "<mem-file>");

        return 0;
    }
    const char* mfile = argv[1];
    // Initialize a CPU trace parser
    TraceParser *mem_trace = initTraceParser(mfile);

    // Initialize the memory system
    MemorySystem *mem_system = initMemorySystem(0, FRFCFS);
    // printf("%u\n", controller->bank_shift);
    // printf("%u\n", controller->bank_mask);

    uint64_t cycles = 0;
    uint64_t total = 0;

    bool stall = false;
    bool end = false;

    uint64_t *parallelMcArr = (uint64_t *)malloc(8 * sizeof(uint64_t));
    // uint64_t *parallelLateArr = (uint64_t *)malloc(8 * sizeof(uint64_t));
    uint64_t *singleMcArr = (uint64_t *)malloc(8 * sizeof(uint64_t));
    double *out = (double *)malloc(8 * sizeof(double));
    setZero(parallelMcArr);
    setZero(singleMcArr);
    // setZero(parallelLateArr);

    printf("Started serving request\n");
    while (!end || pendingRequests(mem_system))
    {
        if (!end && !stall)
        {
            end = !(getRequest(mem_trace));
            total ++;
        }

        if (!end)
        {
            stall = !(access(mem_system, mem_trace->cur_req));
	    
            // printf("%u ", mem_trace->cur_req->core_id);
            // printf("%u ", mem_trace->cur_req->req_type);
            // printf("%"PRIu64" \n", mem_trace->cur_req->memory_address);
        }

        tickEvent(mem_system);
        ++cycles;
    }
    total --;
    printf("Served reqs = %lu\n", total); 
    printf("End Execution Time: ""%"PRIu64"\n", cycles);   
    for (int i = 0; i < NUM_OF_CHANNELS; i++){
        for (int j = 0; j < 8; j++){
            parallelMcArr[j] += mem_system->controllers[i]->accumulated[j];
            // parallelLateArr[j] += mem_system->controllers[i]->latency[j];
        }
    }
    for (int j = 0; j < 4; j++){
        free(mem_system->controllers[j]->bank_status);
        // free(mem_system->controllers[j]->conflict_queue);
        free(mem_system->controllers[j]->waiting_queue);
        free(mem_system->controllers[j]->pending_queue);
        // free(mem_system->controllers[j]->late_queue);
        // free(mem_system->controllers[j]->latency);
        free(mem_system->controllers[j]->bl->blist);
        free(mem_system->controllers[j]->bl);
        free(mem_system->controllers[j]->accumulated);
        free(mem_system->controllers[j]->last);
        free(mem_system->controllers[j]);
    }
    // free(mem_system);
    // free(mem_trace);

    for (int j = 0; j < 8; j++){
        printf("Memory cycles value of Core-[%d] running with other cores is %"PRIu64" \n",j , parallelMcArr[j]);
        // printf(" ---Latency is %"PRIu64" \n", parallelLateArr[j]);
    }

    printf("\n-----------------------------------------------------\n\n");
    fflush(stdout);

    uint64_t total2 = 0;
    for (int i = 0; i < 8; i ++){
        uint64_t local_req_served = 0;
        TraceParser *mem_trace = initTraceParser(mfile);

        int debug = 0;
        if (i == 4){
            debug = 0;
        }
        MemorySystem *mem_system = initMemorySystem(debug, FRFCFS);
        // for (int i = 0; i < 4; i ++){
        //     for (int j = 0; j < 8; j ++){
        //         printf("%d ", mem_system->controllers[i]->latency[j]);
        //     }
        //     printf("\n");
        // }
        // printf("here2 \n");
        fflush(stdout);

        stall = false;
        end = false;

        uint64_t cycle2 = 0;
        uint64_t num_req = 0;
        
        printf("\n\nCORE-[%d] # Started serving request\n", i);
        while (!end || pendingRequests(mem_system))
        {
            if (!end && !stall)
            {
                // printf("here1 \n");
                // fflush(stdout);
                end = !(getRequestCond(mem_trace, i));
                // end = !(getRequest(mem_trace));
                num_req ++;
                // printf("here2 \n");
                // fflush(stdout);
            }

            if (!end)
            {
            //     printf("here3 \n");
            //     fflush(stdout);
                stall = !(access(mem_system, mem_trace->cur_req));
                // printf("here4 \n");
                // fflush(stdout);    
                // printf("%u ", mem_trace->cur_req->core_id);
                // printf("%u ", mem_trace->cur_req->req_type);
                // printf("%"PRIu64" \n", mem_trace->cur_req->memory_address);
            }

            tickEvent(mem_system);
            cycle2 ++;
        }
        num_req --;
        total2 += num_req;
        printf("CORE-[%d] # Served [""%"PRIu64"] requests\n", i, num_req);
        printf("CORE-[%d] # Execution Time: ""%"PRIu64"\n",i , cycle2);   
        for (int j = 0; j < NUM_OF_CHANNELS; j++)
        {
            singleMcArr[i] += mem_system->controllers[j]->accumulated[i];
            printf("Memory cycles value of core %d at channel %d is ""%"PRIu64"\n",i, j , mem_system->controllers[j]->accumulated[i]);
            // printf(" ---Latency value is ""%"PRIu64"\n", mem_system->controllers[j]->latency[i]);
            // printf(" ---For ""%"PRIu64" reqs, stuck ""%"PRIu64" times\n", mem_system->controllers[j]->req_served, mem_system->controllers[j]->req_stuck);
            local_req_served += mem_system->controllers[j]->req_served;
        }
        // printf("FINAL-[""%"PRIu64"] vs [""%"PRIu64"]\n", local_req_served, num_req); 
        assert(local_req_served == num_req);
        out[i] = (double)((double)parallelMcArr[i]/(double)singleMcArr[i]);
        // free(mem_trace);
        // free(mem_system);
        for (int j = 0; j < 4; j++){
            free(mem_system->controllers[j]->bank_status);
            // free(mem_system->controllers[j]->conflict_queue);
            free(mem_system->controllers[j]->waiting_queue);
            free(mem_system->controllers[j]->pending_queue);
            // free(mem_system->controllers[j]->late_queue);
            // free(mem_system->controllers[j]->latency);
            free(mem_system->controllers[j]->bl->blist);
            free(mem_system->controllers[j]->bl);
            free(mem_system->controllers[j]->accumulated);
            free(mem_system->controllers[j]->last);
            free(mem_system->controllers[j]);
        }
        
    }
    // printf("Served ""%"PRIu64" - ""%"PRIu64" reqs\n", total, total2);
    // free(mem_trace);
    

    // Check if the divided instructions total up to the original one.
    assert(total == total2);

    printf("\n\n V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V-V \n");

    for (int j = 0; j < 8; j++){
        printf("Slowdown value of core %d is %f\n",j , out[j]);
    }

    printf("UNFairness value is %f\n", (double)(max(out)/min(out)));
    // TODO, de-allocate memory
    /*
    free(controller->bank_status);
    free(controller->waiting_queue);
    free(controller->pending_queue);
    free(controller);
    */
    
}
