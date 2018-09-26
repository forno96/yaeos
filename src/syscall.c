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


#include <libuarm.h>
#include <arch.h>

#include "pcb.h"
#include "asl.h"

#include "initial.h"
#include "syscall.h"
#include "interrupts.h"
#include "scheduler.h"


void createprocess(){
	//~ tprint("createprocess\n");
	extern pcb_t *currentPCB, *readyQueue;
	extern unsigned int processCount;
	pcb_t *childPCB = allocPcb();
	if (childPCB == NULL) currentPCB->p_s.a1 = -1;
	else {
		SVST((state_t *)currentPCB->p_s.a2,&childPCB->p_s);
		childPCB->p_priority = currentPCB->p_s.a3;
		insertChild(currentPCB, childPCB);
		*(pcb_t **)currentPCB->p_s.a4 = childPCB;
		insertProcQ(&readyQueue,childPCB);
		processCount++;
		currentPCB->p_s.a1 = 0;
	}
}

void terminator(pcb_t *head, pcb_t *tmp){
	extern pcb_t *currentPCB, *readyQueue;
	extern unsigned int processCount, softBlock;
	extern int semDev[MAX_DEVICES];
	extern int semWaitChild;
	
	if (tmp == NULL) tmp = head;
	
	if (tmp->p_first_child != NULL) terminator(tmp->p_first_child, tmp);
	if ((tmp->p_sib != NULL) && (tmp != head)) terminator(tmp->p_sib, tmp);
	
	if (tmp->p_semKey != NULL) {
		if ((tmp->p_semKey >= &semDev[0]) && (tmp->p_semKey <= &semDev[MAX_DEVICES])) softBlock--;
		if((*tmp->p_semKey) < 0) (*tmp->p_semKey)++;
		outChildBlocked(tmp);
	}
	else if (tmp != currentPCB) outProcQ(&readyQueue, tmp);
	else currentPCB = NULL;
	
	if ((tmp->p_parent != NULL) && (tmp->p_parent->p_semKey == &semWaitChild)) {
		outChildBlocked(tmp->p_parent);
		insertProcQ(&readyQueue,tmp->p_parent);
		semWaitChild++;
	}
	
	outChild(tmp);
	freePcb(tmp);
	processCount--;
}

void terminateprocess(){	// TODO: terminateprocess was rewrited, I have to work on it
	//~ tprint("terminateprocess\n");
	extern pcb_t *currentPCB;
	
	pcb_t *head;
	
	if ((pcb_t *)currentPCB->p_s.a2 == NULL) head = currentPCB;
	else head = (pcb_t *)currentPCB->p_s.a2;
	
	terminator(head, NULL);
	
	if ((currentPCB != NULL) && (head->p_parent != NULL)) {
		currentPCB->p_s.a1 = 0;
	}
}

// REMEMBER: sendACK() don't use this function, the trouble is not here!!!
void semv(){
	extern pcb_t *currentPCB, *readyQueue;
	extern unsigned int softBlock;
	extern int semDev[MAX_DEVICES];
	extern cpu_t checkpoint;
	
	int *value = (int *)currentPCB->p_s.a2;
	if ((*value)++ < 1) {
		pcb_t *tmp = removeBlocked(value);
		insertProcQ(&readyQueue, tmp);
		if ((value >= semDev) && (value <= &semDev[MAX_DEVICES - 1])) softBlock--;
		
		currentPCB->kernel_time += getTODLO() - checkpoint;
		checkpoint = getTODLO();
	}
}

void semp(){
	extern pcb_t *currentPCB;
	extern unsigned int softBlock;
	extern int semDev[MAX_DEVICES];
	extern cpu_t checkpoint;
	
	int *value = (int *)currentPCB->p_s.a2;
	if (--(*value) < 0) {
		if (insertBlocked(value, currentPCB)) PANIC();
		if ((value >= semDev) && (value <= &semDev[MAX_DEVICES - 1])) softBlock++;
		
		currentPCB->kernel_time += getTODLO() - checkpoint;
		checkpoint = getTODLO();
		
		currentPCB = NULL;
	}
}

int spechdl(){
	tprint("spechdl\n");
	extern pcb_t *currentPCB;
	unsigned int area;
	if (currentPCB->p_s.a2 == SPECSYSBP) area = SYSBK_NEWAREA;
	else if (currentPCB->p_s.a2 == SPECTLB)  area = TLB_NEWAREA;
	else if (currentPCB->p_s.a2 == SPECPGMT)  area = PGMTRAP_NEWAREA;
	else return -1;
	currentPCB->p_s.a3 = area;
	area = currentPCB->p_s.a4;
	return 0;
}

void gettime(){
	//~ tprint("gettime\n");
	extern pcb_t *currentPCB;
	extern cpu_t checkpoint;
	
	currentPCB->kernel_time += getTODLO() - checkpoint;
	
	*(cpu_t *)currentPCB->p_s.a2 = currentPCB->user_time;
	*(cpu_t *)currentPCB->p_s.a3 = currentPCB->kernel_time;
	*(cpu_t *)currentPCB->p_s.a4 = (getTODLO() - currentPCB->activation_time);
}


void waitclock(){
	//~ tprint("waitclock\n");
	extern pcb_t *currentPCB;
	extern int semDev[MAX_DEVICES];
	extern unsigned int softBlock;
	
	currentPCB->p_s.a2 = (unsigned int)&semDev[CLOCK_SEM];
	semp();
}

void iodevop(){	// TODO: check and fix iodevop()
	//~ tprint("iodevop\n");
	extern pcb_t *currentPCB;
	extern int semDev[MAX_DEVICES];
	extern unsigned int softBlock;
	unsigned int *cmd = 0;
	
	int lineNo = LINENO(currentPCB->p_s.a3);
	
	if ((lineNo <= IL_TIMER) || (lineNo > INT_TERMINAL)) PANIC();
	int devNo = DEVICENO(currentPCB->p_s.a3);
	devreg_t *device = (devreg_t *)DEV_REG_ADDR(lineNo, devNo);
	
	if ((lineNo >= IL_DISK) && (lineNo < IL_TERMINAL)) {
		devNo = EXT_IL_INDEX(lineNo) * DEV_PER_INT + DEVICENO(currentPCB->p_s.a3);
		cmd = &device->dtp.command;
	}
	else {
		devNo = EXT_IL_INDEX(INT_TERMINAL) * DEV_PER_INT + TERMNO(currentPCB->p_s.a3);
		cmd = (TERMTYPE(currentPCB->p_s.a3)) ? &device->term.transm_command : &device->term.recv_command;
	}
	
	currentPCB->p_s.a2 = (unsigned int)&semDev[devNo];
	semp();
	*cmd = ((state_t *)SYSBK_OLDAREA)->a2;
}

void getpids(){
	//~ tprint("getpids\n");
	extern pcb_t *currentPCB;
	if (currentPCB->p_parent == NULL) {
		//~ tprint("root\n");
		if ((pcb_t **)currentPCB->p_s.a2 != NULL) *(pcb_t **)currentPCB->p_s.a2 = NULL;
		if ((pcb_t **)currentPCB->p_s.a3 != NULL) *(pcb_t **)currentPCB->p_s.a3 = NULL;
	}
	else {
		//~ tprint("process\n");
		if ((pcb_t **)currentPCB->p_s.a2 != NULL) *(pcb_t **)currentPCB->p_s.a2 = currentPCB;
		if ((pcb_t **)currentPCB->p_s.a3 != NULL) {
			if (currentPCB->p_parent->p_parent == NULL) *(pcb_t **)currentPCB->p_s.a3 = NULL;	//if parent is root
			else *(pcb_t **)currentPCB->p_s.a3 = currentPCB->p_parent;
		}
	}
}

void waitchild(){
	//~ tprint("waitchild\n");
	extern unsigned int softBlock;
	extern pcb_t *currentPCB;
	extern int semWaitChild;
	if (currentPCB->p_first_child != NULL){
		currentPCB->p_s.a2 = (unsigned int)&semWaitChild;
		semp();
	}
}
