/* * * * * * * * * * * * * * * * * * * * * * * * *
 * YAEOS' phase 2 implementation proposed by     *
 * - Francesco Fornari 							 *
 * - Gabriele Fulgaro							 *
 * - Mattia Polverini							 *
 * 												 *
 * Operating System course						 *
 * A.A. 2017/2018 								 *
 * Alma Mater Studiorum - University of Bologna  *
 * * * * * * * * * * * * * * * * * * * * * * * * */


#include "libuarm.h"
#include "pcb.h"
#include "initial.h"
#include "syscall.h"
#include "scheduler.h"

void debugger(){};


void scheduler(){
	extern pcb_t *readyQueue, *currentPCB;
	extern int processCount, softBlock;
	
	if (processCount){
		if (currentPCB == NULL) {
			if (readyQueue != NULL) {
				currentPCB = removeProcQ(&readyQueue);
			}
			else {
				if (softBlock) {
					//~ tprint("wait scheduler\n");
					setSTATUS(STATUS_ALL_INT_ENABLE(getSTATUS()));
					WAIT();
				}
				else PANIC();
			}
		}
		LDST(&currentPCB->p_s);
		
		//SYSCALL(SEMV, (unsigned int)readyQueue[turn], 0, 0);
		//setTIMER(100000UL);
		//((void (*)(void))readyQueue[turn--]->p_s.pc)();
		//tprint("test\n");
	}
	tprint("no process count\n");
	HALT();
}
