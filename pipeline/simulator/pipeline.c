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
int dMemory[5000];
int i=0,l,k,tmpSigned=0,sp,opcode=0,funct=0,rs=0,rd=0,cycle=0,Cshamt=0;
unsigned int CimmediateUnsugned = 0 ;
short Cimmediate = 0;

//Temp
int WB_TMP=0,MEM_TMP=0,EX_TMP=0,EX_TMP=0,ID_TMP=0;
//Opcode
int WB_OPCODE=0,MEM_OPCODE=0,EX_OPCODE=0,ID_OPCODE=0;
//Funct
int WB_FUNCT=0, MEM_FUNCT = 0, EX_FUNCT = 0 , ID_FUNCT = 0;
//Rs Rt Rd
int WB_RS = 0, WB_RT = 0 , WB_RD = 0;
int EX_RS = 0, EX_RT = 0 , EX_RD = 0;
int MEM_RS = 0, MEM_RT = 0, MEM_RD = 0;
//
int ID_Cshamt = 0, EX_Cshamt = 0, MEM_Cshamt = 0, WB_Cshamt = 0;
int ID_Cimmediate = 0, EX_Cimmediate = 0, MEM_Cimmediate = 0, WB_Cimmediate = 0;
int ID_CimmediateUnsigned = 0, EX_CimmediateUnsigned = 0, MEM_CimmediateUnsigned = 0, WB_CimmediateUnsigned = 0;
int ID_Caddress = 0, EX_Caddress = 0, MEM_Caddress = 0, WB_Caddress = 0;
bool flush= FALSE,stall = FALSE, halt = FALSE;
int fwd_EX_DM_rs_toID=0,fwd_EX_DM_rt_toID=0;
int fwd_EX_DM_rs_toEX=0,fwd_EX_DM_rt_toEX=0;
int fwd_DM_WB_rs_toEX=0,fwd_DM_WB_rt_toEX=0;
int BUF_ID_RS=0,BUF_ID_RT=0;
int branch=0,curpc;


int * WB(int input[5]){
    WB_Caddress = MEM_Caddress;
    WB_CimmediateUnsigned = MEM_CimmediateUnsigned;
    WB_cimmediate = MEM_Cimmediate;
    WB_Cshamt = MEM_Cshamt;
    WB_TMP = MEM_TMP;
    WB_FUNCT = MEM_FUNCT;
    WB_OPCODE = MEM_OPCODE;
    pipeline[4] = pipeline[3];
    input[4] = input[3];
    WB_RD = MEM_RD;
    WB_RT = MEM_RT;
    if(WB_OPCODE == 0x00){
        if(WB_FUNCT != 0x08 &&((pipeline[4]&0xFC1FFFFF)!=0)){
            if(WB_RD==0) fprintf(writeError, "In cycle %d: Write $0 Error\n",cycle);
            else reg[WB_RD] = input[4];
        }
    }
    else if(WB_OPCODE==0x03){
        reg[31] = input[4];
    }
    else if((WB_OPCODE!=0x04)&&(WB_OPCODE!=0x05)&&(WB_OPCODE!=0x2B)&&(WB_OPCODE!=0x28)&&(WB_OPCODE!=0x29)&&(WB_OPCODE!=0x02)&&(WB_OPCODE!=0x3F)){
        if(WB_RT==0) fprintf(writeError, "In cycle %d: Write $0 Error\n", cycle);
        else reg[WB_RT] = input[4];
    }
    return input;
}


int * MEM(int input[5]){
    MEM_Caddress = EX_Caddress;
    MEM_CimmediateUnsig = EX_CimmediateUnsigned;
    MEM_Cimmediate = EX_Cimmediate;
    MEM_Cshamt = EX_Cshamt;
    MEM_TMP = EX_TMP;
    MEM_FUNCT = EX_FUNCT;
    MEM_OPCODE = EX_OPCODE;
    pipeline[3] = pipeline[2];
    input[3] = imput[2];
    MEM_RS = EX_RS;
    MEM_RT = EX_RT;
    MEM_RD = EX_RD;
    if((MEM_OPCODE == 0x08)||(MEM_OPCODE == 0x09)||(MEM_OPCODE == 0x0F)||(MEM_OPCODE == 0x0C)||(MEM_OPCODE == 0x0D)||(MEM_OPCODE == 0x0E)||(MEM_OPCODE) == 0x0A){
        MEM_RD = -1;
    }else if((MEM_OPCODE == 0x02) || (MEM_OPCODE == 0x3F)){
        MEM_RT = -1;
        MEM_RD = -1;
    }else if(MEM_OPCODE == 0x03){
        MEM_RT = 31;
        MEM_RD = 31;
    }else if(MEM_OPCODE==0X00){
        MEM_RT = -1;
    }else if(MEM_OPCODE == 0x23){ //load word
        int temp = reg[MEM_RS]+MEM_Cimmediate;
        int temp1 = temp>>31;
        int rsTemp = reg[MEM_RS]>>31;
        int cTemp = MEM_Cimmediate>>15;
        if(temp<0||(temp+3<0)||(temp>1023)||(temp+3)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt = true;
        }
        if(temp%4 != 0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n" , cycle);
            halt = true;
        }
        if(halt==false){
            input[3] = dMemory[MEM_TMP/4];
            MEM_RD = -1;
        }
    }else if(MEM_OPCODE==0x21){ //load half
        int temp = reg[MEM_RS]+MEM_Cimmediate;
        int temp1 = temp>>31;
        int rsTemp = reg[MEM_RS]>>31;
        int cTemp = MEM_Cimmediate>>15;
        if(temp<0||(temp+1<0)||(temp>1023)||(temp+1)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt = true;
        }
        if(temp%2 != 0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n" , cycle);
            halt = true;
        }
        if(halt==false){
            if(MEM_TMP%4==0){
                input[3] = dMemory[MEM_TMP/4]>>16;
            }else if(MEM_TMP%4==2){
                input[3] = (dMemory[MEM_TMP/4]<<16)>>16;
            }
            MEM_RD = -1;
        }

    }else if(MEM_OPCODE==0x25){ // load half unsigned
        int temp = reg[MEM_RS]+MEM_Cimmediate;
        int temp1 = temp>>31;
        int rsTemp = reg[MEM_RS]>>31;
        int cTemp = MEM_Cimmediate>>15;
        if(temp<0||(temp+1<0)||(temp>1023)||(temp+1)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt = true;
        }
        if(temp%2 != 0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n" , cycle);
            halt = true;
        }
        if(halt==false){
            if(MEM_TMP%4==0){
                input[3] = (dMemory[MEM_TMP/4]>>16) & 0x0000FFFF;
            }else if(MEM_TMP%4==2){
                input[3] = dMemory[MEM_TMP/4] & 0x0000FFFF;
            }
            MEM_RD = -1;
        }

    }else if(MEM_OPCODE == 0x20){ // load b
        int temp = reg[MEM_RS]+MEM_Cimmediate;
        int temp1 = temp>>31;
        int rsTemp = reg[MEM_RS]>>31;
        int cTemp = MEM_Cimmediate>>15;
        if(temp<0||temp>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt =true;
        }
        if(halt == false){
            if(MEM_TMP%4 == 0){
                input[3] = dMemory[MEM_TMP/4]>>24;
            }else if(MEM_TMP%4 ==1 ){
                input[3] = (dMemory[MEM_TMP]<<8)>>24;
            }else if(MEM_TMP%4 ==2){
                input[3] = (dMemory[MEM_TMP]<<16)>>24;
            }else if(MEM_TMP%4 ==3){
                input[3] = dMemory[MEM_TMP];
            }
            MEM_RD = -1;
        }
    }else if(MEM_OPCODE == 0x24){ // lbu
        int temp = reg[MEM_RS]+MEM_Cimmediate;
        int temp1 = temp>>31;
        int rsTemp = reg[MEM_RS]>>31;
        int cTemp = MEM_Cimmediate>>15;
        if(temp<0||temp>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt =true;
        }
        if(halt == false){
            if(MEM_TMP%4 == 0){
                input[3] = (dMemory[MEM_TMP/4]>>24) & 0x000000FF;
            }else if(MEM_TMP%4 ==1 ){
                input[3] = ((dMemory[MEM_TMP]<<8)>>24) & 0x000000FF;
            }else if(MEM_TMP%4 ==2){
                input[3] = ((dMemory[MEM_TMP]<<16)>>24) & 0x000000FF;
            }else if(MEM_TMP%4 ==3){
                input[3] = dMemory[MEM_TMP] & 0x000000FF;
            }
            MEM_RD = -1;
        }

    }else if(MEM_OPCODE == 0x2B){ //SW
        int temp = reg[MEM_RS]+MEM_Cimmediate;
        int temp1 = temp>>31;
        int rsTemp = reg[MEM_RS]>>31;
        int cTemp = MEM_Cimmediate>>15;
        if(temp<0||(temp+3<0)||(temp>1023)||(temp+3)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt = true;
        }
        if(temp%4 != 0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n" , cycle);
            halt = true;
        }
        if(halt==false){
            dMemory[MEM_TMP/4] = input[3];
            MEM_RT = -1;
            MEM_RD = -1;
        }

    }else if(MEM_OPCODE == 0x29){ //SH
        int temp = reg[MEM_RS]+MEM_Cimmediate;
        int temp1 = temp>>31;
        int rsTemp = reg[MEM_RS]>>31;
        int cTemp = MEM_Cimmediate>>15;
        if(temp<0||(temp+1<0)||(temp>1023)||(temp+1)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt = true;
        }
        if(temp%2 != 0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n" , cycle);
            halt = true;
        }
        if(halt==false){
            if(MEM_TMP%4==0){
                dMemory[MEM_TMP/4] = (dMemory[MEM_TMP/4] & 0x0000FFFF) + (input[3]<<16);
            }else if(MEM_TMP%4==2){
                dMemory[MEM_TMP/4] = (dMemory[MEM_TMP/4] & 0x0000FFFF) +input[3];
            }
            MEM_RD = -1;
            MEM_RT = -1;
        }

    }else if(MEM_OPCODE == 0x28){ // SB
        int temp = reg[MEM_RS]+MEM_Cimmediate;
        int temp1 = temp>>31;
        int rsTemp = reg[MEM_RS]>>31;
        int cTemp = MEM_Cimmediate>>15;
        if(temp<0||temp>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt =true;
        }
        if(halt == false){
            if(MEM_TMP%4 == 0){
                dMEmory[MEM_TMP/4] = (dMemory[MEM_TMP/4] & 0x00FFFFFF) + (input[3] << 24);
            }else if(MEM_TMP%4 ==1 ){
                dMEmory[MEM_TMP/4] = (dMemory[MEM_TMP/4] & 0xFF00FFFF) + (input[3] << 16);
            }else if(MEM_TMP%4 ==2){
                dMEmory[MEM_TMP/4] = (dMemory[MEM_TMP/4] & 0xFFFF00FF) + (input[3] << 8);
            }else if(MEM_TMP%4 ==3){
                dMEmory[MEM_TMP/4] = (dMemory[MEM_TMP/4] & 0xFFFFFF00) + (input[3]);
            }
            MEM_RD = -1;
            MEM_RT = -1;
        }

    }
    return input;
}