#include<stdio.h>
#include<stdlib.h>
#include<string.h>
FILE *writeSnapshot;
FILE *writeError;
void printRegister();
void printStage();
void printOpcode(int, int, int, char*);
void WB();
void MEM();
void EX();
void ID();
void IF();
int toBigEndian(unsigned int);

unsigned int carryAddress, tmpp;
int reg[100];
int num;
unsigned int pipeline[5];
