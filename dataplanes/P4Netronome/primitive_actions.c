#include <nfp.h>

#include <pif_plugin.h>
#include <pif_plugin_metadata.h>
#include <pif_registers.h>
#include <pif_pkt.h>


#define SEM_COUNT 256
    
__declspec(ctm export aligned(64)) int my_semaphore = 1;

__declspec(ctm export aligned(64)) long long int my_data = 0;

__declspec(imem export aligned(64)) int global_semaphores[SEM_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

//__declspec(shared scope(global) export imem aligned(64)) int global_semaphores[SEM_COUNT] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

// Just for debugging.
//__declspec(shared scope(global) export imem aligned(64)) long long int global_counters[SEM_COUNT];

void semaphore_down(volatile __declspec(mem addr40) void * addr) {
	/* semaphore "DOWN" = claim = wait */
	unsigned int addr_hi, addr_lo;
	__declspec(read_write_reg) int xfer;
	SIGNAL_PAIR my_signal_pair;
	addr_hi = ((unsigned long long int)addr >> 8) & 0xff000000;
	addr_lo = (unsigned long long int)addr & 0xffffffff;
	do {
		xfer = 1;
		__asm {
            mem[test_subsat, xfer, addr_hi, <<8, addr_lo, 1],\
                sig_done[my_signal_pair];
            ctx_arb[my_signal_pair]
        }
	} while (xfer == 0);
}

void semaphore_up(volatile __declspec(mem addr40) void * addr) {
	/* semaphore "UP" = release = signal */
	unsigned int addr_hi, addr_lo;
	__declspec(read_write_reg) int xfer;
	addr_hi = ((unsigned long long int)addr >> 8) & 0xff000000;
	addr_lo = (unsigned long long int)addr & 0xffffffff;

    __asm {
        mem[incr, --, addr_hi, <<8, addr_lo, 1];
    }

}


// lock a semaphore based on an ID in the metadata.
int pif_plugin_semaphore_lock(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *data){
    // Get the ID of the semaphore to lock from the packet header.
    __declspec(local_mem) int sem_sid;
    sem_sid = (int)pif_plugin_meta_get__sm__sid(headers);
    
    // Lock that semaphore.
    semaphore_down( &global_semaphores[sem_sid]);
    
    return PIF_PLUGIN_RETURN_FORWARD;
}
// Release a semaphore based on an ID in the metadata.
int pif_plugin_semaphore_release(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *data){
    // Get the ID of the semaphore to lock from the packet header.
    // Would like to store this in metadata, but..
    __declspec(local_mem) int sem_sid;
    sem_sid = (int)pif_plugin_meta_get__sm__sid(headers);
    
    // Release that semaphore.
    semaphore_up( &global_semaphores[sem_sid]);

    return PIF_PLUGIN_RETURN_FORWARD;
}



void copyRegister(){
    volatile __declspec( mem addr40) unsigned int *payload;
    int i, copyCt, mu_offset;    
    mu_offset =(256 << pif_pkt_info_global.ctm_size);
    payload = (__declspec( mem addr40) unsigned int *)(((uint64_t)pif_pkt_info_global.muptr << 11) + mu_offset);    
    // Manually copy 300 words to payload (i.e. 50 flow records @ 6 words each).
    copyCt = 300;
    for (i=0; i<copyCt; i++){
        payload[i] = pif_register_outBuf_reg[i].value;
    }    

}

// Dump 300 words of the output buffer to the payload of the current packet. 
int pif_plugin_dump_outbuf_300(EXTRACTED_HEADERS_T *headers, MATCH_DATA_T *data){
    copyRegister();
    return PIF_PLUGIN_RETURN_FORWARD;
}
