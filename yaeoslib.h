#ifndef YAEOSLIB_H
#define YAEOSLIB_H

#include <uARMtypes.h>
#include <const.h>

typedef struct pcb_t {
	struct pcb_t *p_next;
	struct pcb_t *p_parent;
	struct pcb_t *p_first_child;
	struct pcb_t *p_sib;
	state_t p_s;
	int priority;
	int *p_semKey;	
} pcb_t;

typedef struct semd_t {
	struct semd_t *s_next;
	int *s_key;
	struct pcb_t *s_procQ;
} semd_t;

/**** PCB queue management ****/

/* Initialize pcbFree so that it contains all the elements of pcbFree_table.
   It will be called only once during data structure initialization. */
void initPcbs();

/* Insert the PCB pointed by *p in the free PCB list */
void freePcb(pcb_t *p);

/* Remove a element from free PCB list, initialize it and return it.
   Return NULL if the free PCB list is empty. */
pcb_t *allocPcb();

/* Insert the element pointed by *p in the process queue pointed by **head.
   Insertion have to respect the priority of each PCB.
   Process queue must be descending sorted according to PCB's priority. */
void insertProcQ(pcb_t **head, pcb_t *t);

/* Return the element in the head of queue pointed by pointer passed as pointer, without removing it.
   Return NULL if the queue is empty. */
pcb_t headProcQ(pcb_t *head);

/* Remove the first element in the queue pointed by the argument.
   Return a poiter to the removed element or NULL if the queue is empty. */
pcb_t* removeProcQ(pcb_t **head);

/* Remove the element pointed by *p from the queue pointed by the first argument.
   Return a pointer to p if it is present, NULL otherwise. */
pcb_t* outProcQ(pcb_t **head, pcb_t *p);

/* For each element in the queue pointed by head, call the function fun(...) */
void forallProcQ(pcb_t *head, void *fun(pcb_t *pcb, void *), void *arg);


/**** PCB tree management ****/

/* Insert the PCB pointed by p as a child of PCB pointed by parent */
void insertChild(pcb_t *parent, pcb_t *p);

/* Remove the first child of the PCB pointed by p and return its pointer. 
   Return NULL if p has no child */
pcb_t *removeChild(pcb_t *p);

/* Remove PCB pointed by p from the child list of his father.
   If PCB has no father, it returns NULL.
   Return a pointer to the removed element otherwise. */
pcb_t *outChild(pcb_t *p);


/**** ASHT - Active Semaphore Hash Table Management ****/

/* Insert PCB pointed by p in the queue of blocked process from semaphore with key as key.
   If the semaphore is not in the queue, it allocates a new one from the free list and insert into ASHD with all the field setted accordingly.
   If it is not possible, it returns -1. 0 otherwise. */

int insertBlocked(int *key, pcb_t *p);

/* Returns the pointer to the PCB of first blocked process on semaphore key. 
   If semaphore does not exist it returns NULL. */
pcb_t *headBlocked(int *key);
pcb_t* removeBlocked(int *key);
void forallBlocked(int *key, void (*fun)(pcb_t *pcb, void *), void *arg);
outChildBlocked(pcb_t *p);
void initASL();
#endif