#ifndef INITIAL_H
#define INITIAL_H

#define LINE_NO(addr) (((addr - DEV_REG_START) / DEV_REGBLOCK_SIZE ) - 1 + DEV_IL_START)

typedef unsigned int memaddr;
typedef unsigned int cpu_t;

extern void test();
void newArea(memaddr address, void handler());

#endif