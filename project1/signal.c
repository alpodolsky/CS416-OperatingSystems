/* signal.c
 *
 * Group Members Names and NetIDs:
 *   1.Alex Podolsky, adp169    
 *   2.Al Manrique, am2240
 *
 * ILab Machine Tested on:
 *  ilab1
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

/* Part 1 - Step 2 to 4: Do your tricks here
 * Your goal must be to change the stack frame of caller (main function)
 * such that you get to the line after "r2 = *( (int *) 0 )"
 */
void segment_fault_handler(int signum) {
    printf("I am slain!\n");
    
    /* Implement Code Here */
    int* p= &signum;
    //i feel like i couldve incremented this by another value
    //but i found it from printing an excessive amount of addresses starting at rbp's
    //in gdb and counted the difference in position from signum's address to what i think is rip
    p=p+51;
    //dereference p for its value and move it past the offending instruction
    //0x00000000004005bd <+42>:    mov    (%rax),%eax <-- offending instruction
    //0x00000000004005bf <+44>:    mov    %eax,-0x4(%rbp) <-- this instruction is fine
    //0x00000000004005c2 <+47>:    mov    $0x40067c,%edi <-- or this one
    //after some playing it seems any value from 2 to 5 would work
    *(p)+=0x2;
}

int main(int argc, char *argv[]) {

    int r2 = 0;

    /* Part 1 - Step 1: Registering signal handler */
    /* Implement Code Here */
    signal(SIGSEGV,segment_fault_handler);
    r2 = *( (int *) 0 ); // This will generate segmentation fault

    printf("I live again!\n");

    return 0;
}
