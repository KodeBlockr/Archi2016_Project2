#include<stdio.h>
#include<stdlib.h>
#include<string.h>
FILE *writeSnapshot;
FILE *writeError;
void printRegister();
void printStage();
void printOpcode(int, int, int, char*);
int * WB(int input[5]);
int * MEM(int input[5]);
int * EX(int input[5]);
int * ID(int input[5]);
int * isFwd(int buff[2], int fwd_Input[5], int what);
int toBigEndian(unsigned int);

unsigned int carryAddress, tmpp;
int reg[100];
int num;
unsigned int pipeline[5];
int dMemory[5000];
int i=0,l,k,tmpSigned=0,sp,opcode=0,funct=0,rs=0,rd=0,cycle=0,Cshamt=0;
unsigned int CimmediateUnsigned = 0 ;
short Cimmediate = 0;
#define _RS 0
#define _RT 1
//Temp
int WB_TMP=0,MEM_TMP=0,EX_TMP=0,EX_TMP=0,ID_TMP=0;
//Opcode
int WB_OPCODE=0,MEM_OPCODE=0,EX_OPCODE=0,ID_OPCODE=0, IF_OPCODE = 0;
//Funct
int WB_FUNCT=0, MEM_FUNCT = 0, EX_FUNCT = 0 , ID_FUNCT = 0;
//Rs Rt Rd
int WB_RS = 0, WB_RT = 0 , WB_RD = 0;
int EX_RS = 0, EX_RT = 0 , EX_RD = 0;
int MEM_RS = 0, MEM_RT = 0, MEM_RD = 0;
int ID_RS = 0, ID_RT = 0 , ID_RD = 0;
//
int ID_Cshamt = 0, EX_Cshamt = 0, MEM_Cshamt = 0, WB_Cshamt = 0;
int ID_Cimmediate = 0, EX_Cimmediate = 0, MEM_Cimmediate = 0, WB_Cimmediate = 0;
int ID_CimmediateUnsigned = 0, EX_CimmediateUnsigned = 0, MEM_CimmediateUnsigned = 0, WB_CimmediateUnsigned = 0;
int ID_Caddress = 0, EX_Caddress = 0, MEM_Caddress = 0, WB_Caddress = 0;
bool flush= FALSE,stall = FALSE, halt = FALSE;
bool fwd_EX_DM_rs_toID=false,fwd_EX_DM_rt_toID=false;
bool fwd_EX_DM_rs_toEX=false,fwd_EX_DM_rt_toEX=false;
bool fwd_DM_WB_rs_toEX=false,fwd_DM_WB_rt_toEX=false;
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

int * isFwd(int buff[2], int fwd_Input[5], int what){
    if(what != 1){
    if(EX_RS!=0){
        if(((EX_RS==MEM_RD)&&((MEM_OPCODE==0x00)&&(MEM_FUNCT!=0x08)))||((EX_RS==MEM_RT)&&((MEM_OPCODE==0x08)||(MEM_OPCODE==0x09)||(MEM_OPCODE==0x0F)||(MEM_OPCODE==0x0C)||(MEM_OPCODE==0x0D)||(MEM_OPCODE==0x0E)||(MEM_OPCODE==0x0A)||(MEM_OPCODE==0x03)))){
            
            buff[_RS]=fwd_Input[3];
            
            fwd_EX_DM_rs_toEX=true;
        }
        else if(((EX_RS==WB_RD)&&((WB_OPCODE==0x00)&&(WB_FUNCT!=0x08)))||((EX_RS==WB_RT)&&((WB_OPCODE!=0x00)&&(WB_OPCODE!=0x2B)&&(WB_OPCODE!=0x29)&&(WB_OPCODE!=0x28)&&(WB_OPCODE!=0x04)&&(WB_OPCODE!=0x05)&&(WB_OPCODE!=0x07)&&(WB_OPCODE!=0x02)&&(WB_OPCODE!=0x3F)))){
            
            fwd_DM_WB_rs_toEX=true;
            
            buff[_RS]=reg[EX_RS];
        }
        
        else buff[_RS]=reg[EX_RS];
    }
    else buff[_RS]=reg[EX_RS];
    }
    if(what != 0 ){
    if(EX_RT!=0){
        if(((EX_RT==MEM_RD)&&((MEM_OPCODE==0x00)&&(MEM_FUNCT!=0x08)))||((EX_RT==MEM_RT)&&((MEM_OPCODE==0x08)||(MEM_OPCODE==0x09)||(MEM_OPCODE==0x0F)||(MEM_OPCODE==0x0C)||(MEM_OPCODE==0x0D)||(MEM_OPCODE==0x0E)||(MEM_OPCODE==0x0A)||(MEM_OPCODE==0x03)))){
            
            buff[_RT]=fwd_Input[3];
            
            fwd_EX_DM_rt_toEX=true;
        }
        else if(((EX_RT==WB_RD)&&((WB_OPCODE==0x00)&&(WB_FUNCT!=0x08)))||((EX_RT==WB_RT)&&((WB_OPCODE!=0x00)&&(WB_OPCODE!=0x2B)&&(WB_OPCODE!=0x29)&&(WB_OPCODE!=0x28)&&(WB_OPCODE!=0x04)&&(WB_OPCODE!=0x05)&&(WB_OPCODE!=0x07)&&(WB_OPCODE!=0x02)&&(WB_OPCODE!=0x3F)))){
            
            fwd_DM_WB_rt_toEX=true;
            
            buff[_RT]=reg[EX_RT];
        }
        else buff[_RT]=reg[EX_RT];
    }else{
        buff[_RT] = reg[EX_RT];
    }
    }
    
    return buff;
    
}

int * EX(int input[5]){
    if(stall==true){
        EX_Caddress = 0;
        EX_CimmediateUnsigned = 0;
        EX_Cimmediate = 0;
        EX_Cshamt = 0;
        EX_FUNCT = 0;
        EX_OPCODE = 0;
        pipeline[2] = 0;
        EX_RS = 0;
        EX_RT = 0;
        EX_RD = 0;
    }else if(stall == false){
        int BUFF[2];// BUFF[0] = RS, BUFF[1] = RT
        EX_Caddress = ID_Caddress;
        EX_CimmediateUnsigned = ID_CimmediateUnsigned;
        EX_Cimmediate = ID_Cimmediate;
        EX_Cshamt = ID_Cshamt;
        EX_FUNCT = ID_FUNCT;
        EX_OPCODE = ID_OPCODE;
        pipeline[2] = pipeline[1];
        input[2] = input[1];
        EX_RS = ID_RS;
        EX_RT = ID_RT;
        EX_RD = ID_RD;
        
        if(EX_OPCODE==0x00){
            if((EX_FUNCT=0x20)||(EX_FUNCT==0x22)||(EX_FUNCT==0x24)||(EX_FUNCT==0x25)||(EX_FUNCT==0x26)||(EX_FUNCT==0x27)||(EX_FUNCT==0x28)||(EX_FUNCT==0x2A)){
                BUFF = isFwd(BUFF, input, 3);
            }
            
            if(EX_FUNCT==0x20){//add
                int resultTmp=BUFF[_RS]+BUFF[_RT];
                int resultSign=resultTmp>>31;
                int rsTmp=BUFF[_RS]>>31;
                int rtTmp=BUFF[_RT]>>31;
                if((rsTmp==rtTmp)&&(resultSign!=rsTmp)){
                    fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
                }
                input[2]=resultTmp;
            }else if(EX_FUNCT==0x22){//sub
                int resultTmp=BUFF[_RS]-BUFF[_RT];
                int resultSign=resultTmp>>31;
                int rsTmp=BUFF[_RS]>>31;
                int rtTmp=(0-BUFF[_RT])>>31;
                if((rsTmp==rtTmp)&&(resultSign!=rsTmp)){
                    fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
                }
                input[2]=resultTmp;
            }else if(EX_FUNCT==0x24){//and
                input[2]=BUF[_RS]&BUFF[_RT];
            }else if(EX_FUNCT==0x25){//or
                input[2]=BUFF[_RS]|BUFF[_RT];
            }else if(EX_FUNCT==0x26){//xor
                input[2]=BUFF[_RS]^BUFF[_RT];
            }else if(EX_FUNCT==0x27){//nor
                input[2]=~(BUFF[_RS]|BUFF[_RT]);
            }else if(EX_FUNCT==0x28){//nand
                input[2]=~(BUFF[_RS]&BUFF[_RT]);
            }else if(EX_FUNCT==0x2A){//slt
                input[2]=(BUFF[_RS]<BUFF[_RT]);
            }else if(EX_FUNCT==0x00){//sll
                BUFF = isFwd(BUFF, input, _RT);
                input[2] = BUFF[_RT]<<EX_Cshamt;
            }else if(EX_FUNCT==0x02){//srl
                BUFF = isFwd(BUFF, input, _RT);
                input[2] = BUFF[_RT];
                for(l = 0 ; l < EX_Cshamt; l++){
                    input[2] = (input[2]>>1)&0x7FFFFFFF;
                }
            }else if(EX_FUNCT==0x03){//sra
                BUFF = isFwd(BUFF, input, _RT);
                input[2] = BUFF[_RT]>>EX_Cshamt;
            }
        }else if(EX_OPCODE==0x08){ //addi
            BUFF = isFwd(BUFF, input, _RS);
            int resultTmp = BUFF[_RS]+EX_Cimmediate;
            int resultSign = resultTmp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(resultSign!=rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            input[2] = resultTmp;
        }else if(EX_OPCODE == 0x09){ //addiu
            BUFF = isFwd(BUFF, input, _RS);
            int resultTmp = BUFF[_RS]+EX_Cimmediate;
            input[2] = resultTmp;
        }else if(EX_OPCODE == 0x23){ //lw
            BUFF = isFwd(BUFF, input, _RS);
            int temp = BUFF[_RS]+EX_Cimmediate;
            int tempTemp = temp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(tempTemp != rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_TMP = BUFF[_RS]+EX_Cimmediate;
        }else if(EX_OPCODE == 0x21){ //lh
            BUFF = isFwd(BUFF, input, _RS);
            int temp = BUFF[_RS]+EX_Cimmediate
            int tempTemp = temp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(tempTemp!=rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_TMP = BUFF[_RS]+EX_Cimmediate;
        }else if(EX_OPCODE==0x25){ //lhu
            BUFF = isFwd(BUFF, input, _RS);
            int temp = BUFF[_RS]+EX_Cimmediate
            int tempTemp = temp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(tempTemp!=rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_TMP = BUFF[_RS]+EX_Cimmediate;
        }else if(EX_OPCODE==0x20){ //lb
            BUFF = isFwd(BUFF, input, _RS);
            int temp = BUFF[_RS]+EX_Cimmediate
            int tempTemp = temp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(tempTemp!=rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_TMP = BUFF[_RS]+EX_Cimmediate;
        }else if(EX_OPCODE==0x24){ //lbu
            BUFF = isFwd(BUFF, input, _RS);
            int temp = BUFF[_RS]+EX_Cimmediate
            int tempTemp = temp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(tempTemp!=rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_TMP = BUFF[_RS]+EX_Cimmediate;
        }else if(EX_OPCODE==0x2B){ //SW
            BUFF = isFwd(BUFF, input, 3);
            int temp = BUFF[_RS]+EX_Cimmediate;
            int tempTemp = temp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(tempTmep!=rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_TMP = BUFF[_RS]+EX_Cimmediate;
            input[2] = BUFF[_RT];
        }else if(EX_OPCODE==0x29){ //sh
            BUFF = isFwd(BUFF, input, 3);
            int temp = BUFF[_RS]+EX_Cimmediate;
            int tempTemp = temp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(tempTmep!=rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_TMP = BUFF[_RS]+EX_Cimmediate;
            input[2] = BUFF[_RT]&0x0000FFFF;
        }else if(EX_OPCODE==0x28){ //sb
            BUFF = isFwd(BUFF, input, 3);
            int temp = BUFF[_RS]+EX_Cimmediate;
            int tempTemp = temp>>31;
            int rsTmp = BUFF[_RS]>>31;
            int cTmp = EX_Cimmediate>>15;
            if((rsTmp==cTmp)&&(tempTmep!=rsTmp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_TMP = BUFF[_RS]+EX_Cimmediate;
            input[2] = BUFF[_RT]&0x000000FF;
        }else if(EX_OPCODE==0x0F){ //lui
            input[2] = EX_Cimmediate<<16;
        }else if(EX_OPCODE==0x0C){ //andi
            BUFF = isFwd(BUFF, input, _RS);
            input[2] = BUFF[_RS]&EX_CimmediateUnsigned;
        }else if(EX_OPCODE==0x0D){ //ori
            BUFF = isFwd(BUFF, input, _RS);
            input[2] = BUFF[_RS] | EX_CimmediateUnsigned;
        }else if(EX_OPCODE==0x0E){ //nori
            BUFF = isFwd(BUFF, input, _RS);
            input[2] = ~(BUFF[_RS]|EX_CimmediateUnsigned);
        }else if(EX_OPCODE==0x0A){ //slti
            BUFF = isFwd(BUFF, input, _RS);
            input[2] = (BUFF[_RS]<EX_Cimmediate);
        }
            
    }
    return input;
}

int * ID(int input[5]){
    if(stall==false){
        pipeline[1] = pipeline[0];
    }else{
        stall==true;
    }
    if(flush==true){
        pipeline[1] = 0;
        flush = false;
    }
    opcode = pipeline[1]>>26;
    funct = (pipeline[1]<<26)>>26;
    rs = (pipeline[1]<<6)>>27;
    rt = (pipeline[1]<<11)>>27;
    rd = (pipeline[1]<<16)>>27;
    Cshamt = (pipeline[1]<<21)>>27;
    Cimmediate = ((pipeline[1]<<16)>>16);
    CimmediateUnsigned = pipeline[1]&0x0000FFFF;
    carryAddress = (pipeline[1]<<6)>>6;
    input[1] = input[0];
    switch(opcode){
        case 0x00:
            switch(funct){
                case 0x20://add
                case 0x21://addu
                case 0x22://sub
                case 0x24://and
                case 0x25://or
                case 0x26://xor
                case 0x27://nor
                case 0x28://nand
                case 0x2A://slt
                    ID_OPCODE = opcode;
                    ID_FUNCT = funct;
                    ID_RS = rs;
                    ID_RT = rt;
                    ID_RD = rd;
                    if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){
                        stall = true;
                    }else if(MEM_OPCODE==0x23){
                        stall = true;
                    }
                    break;
                case 0x00://sll
                case 0x02://srl
                case 0x03://sra
                    ID_OPCODE = opcode;
                    ID_FUNCT = funct;
                    ID_RT = rt;
                    ID_RD = rd;
                    ID_Cshamt = Cshamt;
                    if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&(ID_RT==EX_RT)&&(ID_RT!=0)){
                        stall = true;
                    }
                    break;
                case 0x08://jr
                    ID_OPCODE = opcode;
                    ID_FUNCT = funct;
                    ID_RS = rs;
                    if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24)||(EX_OPCODE==0x08)||(EX_OPCODE==0x09)||(EX_OPCODE==0x0F)||(EX_OPCODE==0x0C)||(EX_OPCODE==0x0D)||(EX_OPCODE==0x0E)||(EX_OPCODE==0x0A))&&((ID_RS==EX_RT)&&(ID_RS!=0))){//-------addiu OPCODE = 0x09
                        stall=true;
                    }
                    else if(((MEM_OPCODE==0x23)||(MEM_OPCODE==0x21)||(MEM_OPCODE==0x25)||(MEM_OPCODE==0x20)||(MEM_OPCODE==0x24))&&((ID_RS==MEM_RT)&&(ID_RS!=0))){
                        stall=true;
                    }
                    else if(((EX_OPCODE==0x00)&&((EX_FUNCT==0x20)||(EX_FUNCT==0x21)||(EX_FUNCT==0x22)||(EX_FUNCT==0x24)||(EX_FUNCT==0x25)||(EX_FUNCT==0x26)||(EX_FUNCT==0x27)||(EX_FUNCT==0x28)||(EX_FUNCT==0x2A)||(EX_FUNCT==0x00)||(EX_FUNCT==0x02)||(EX_FUNCT==0x03)))&&((ID_RS==EX_RD)&&(ID_RS!=0))){ //------addu--0x21
                        stall=true;
                    }
                    else if((MEM_OPCODE==0x00)&&((MEM_FUNCT==0x20)||(MEM_FUNCT==0x21)||(MEM_FUNCT==0x22)||(MEM_FUNCT==0x24)||(MEM_FUNCT==0x25)||(MEM_FUNCT==0x26)||(MEM_FUNCT==0x27)||(MEM_FUNCT==0x28)||(MEM_FUNCT==0x2A)||(MEM_FUNCT==0x00)||(MEM_FUNCT==0x02)||(MEM_FUNCT==0x03))){//-------- addu-----FUNCT = 0x21-----
                        if(ID_RS!=0){
                            if(ID_RS==MEM_RD){
                                fwd_EX_DM_rs_toID=true;
                                BUF_ID_RS=input[3];
                            }
                            else BUF_ID_RS=reg[ID_RS];
                        }
                        else BUF_ID_RS=reg[ID_RS];
                    }
                    else if((MEM_OPCODE==0x08)||(MEM_OPCODE==0x09)||(MEM_OPCODE==0x0F)||(MEM_OPCODE==0x0C)||(MEM_OPCODE==0x0D)||(MEM_OPCODE==0x0E)||(MEM_OPCODE==0x0A)){//----------addiu
                        if(ID_RS!=0){
                            if(ID_RS==MEM_RT){
                                fwd_EX_DM_rs_toID=true;
                                BUF_ID_RS=input[3];
                            }
                            else BUF_ID_RS=reg[ID_RS];
                        }
                        else BUF_ID_RS=reg[ID_RS];
                    }
                    else if(MEM_OPCODE==0x03){
                        if(ID_RS==31){
                            fwd_EX_DM_rs_toID=true;
                            BUF_ID_RS=input[3];
                        }
                        else BUF_ID_RS=reg[ID_RS];
                    }
                    else {
                        BUF_ID_RS=reg[ID_RS];
                    }
                    if(stall==false)flush=true;
                    break;
            }
        case 0x23://lw
        case 0x21://lh
        case 0x25://lhu
        case 0x20://lb
        case 0x24://lbu
            ID_OPCODE = opcode;
            ID_RS = rs;
            ID_RT = rt;
            ID_Cimmediate = Cimmediate;
            if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&(ID_RS==EX_RT)&&(ID_RS!=0)){
                stall = true;
            }
            break;
        case 0x2B://sw
        case 0x29://sh
        case 0x28://sb
            ID_OPCODE = opcode;
            ID_RS = rs;
            ID_RT = rt;
            ID_Cimmediate = Cimmediate;
            if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&((ID_RS==EX_RT)&&(ID_RS!=0)||(ID_RT==EX_RT)&&(ID_RT!=0))){
                stall = true;
            }
            break;
        case 0x08://addi
        case 0x09://addiu
        case 0x0A://slti
            ID_OPCODE = opcode;
            ID_RS = rs;
            ID_RT = rt;
            ID_Cimmediate = Cimmediate;
            if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&(ID_RS==EX_RT)&&(ID_RS!=0)){
                stall =true;
            }
            break;
        case 0x04://beq
            ID_OPCODE=opcode;
            ID_RS=rs;
            ID_RT=rt;
            ID_Cimmediate=Cimmediate;
            if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){
                stall=true;
            }
            else if(((EX_OPCODE==0x08)||(EX_OPCODE==0x09)||(EX_OPCODE==0x0F)||(EX_OPCODE==0x0C)||(EX_OPCODE==0x0D)||(EX_OPCODE==0x0E)||(EX_OPCODE==0x0A))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){//-------addiu---0x09
                stall=true;
            }
            else if(((MEM_OPCODE==0x23)||(MEM_OPCODE==0x21)||(MEM_OPCODE==0x25)||(MEM_OPCODE==0x20)||(MEM_OPCODE==0x24))&&(((ID_RS==MEM_RT)&&(ID_RS!=0))||((ID_RT==MEM_RT)&&(ID_RT!=0)))){
                stall=true;
            }
            else if(((EX_OPCODE==0x00)&&((EX_FUNCT==0x20)||(EX_FUNCT==0x21)||(EX_FUNCT==0x22)||(EX_FUNCT==0x24)||(EX_FUNCT==0x25)||(EX_FUNCT==0x26)||(EX_FUNCT==0x27)||(EX_FUNCT==0x28)||(EX_FUNCT==0x2A)||(EX_FUNCT==0x00)||(EX_FUNCT==0x02)||(EX_FUNCT==0x03)))&&(((ID_RS==EX_RD)&&(ID_RS!=0))||((ID_RT==EX_RD)&&(ID_RT!=0)))){ //addu 0x21
                stall=true;
            }
            else if((MEM_OPCODE==0x00)&&((MEM_FUNCT==0x20)||(MEM_FUNCT==0x21)||(MEM_FUNCT==0x22)||(MEM_FUNCT==0x24)||(MEM_FUNCT==0x25)||(MEM_FUNCT==0x26)||(MEM_FUNCT==0x27)||(MEM_FUNCT==0x28)||(MEM_FUNCT==0x2A)||(MEM_FUNCT==0x00)||(MEM_FUNCT==0x02)||(MEM_FUNCT==0x03))){//addu-------
                if(ID_RS!=0){
                    if(ID_RS==MEM_RD){
                        fwd_EX_DM_rs_toID=true;
                        BUF_ID_RS=input[3];
                    }
                    else BUF_ID_RS=reg[ID_RS];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT!=0){
                    if(ID_RT==MEM_RD){
                        fwd_EX_DM_rt_toID=true;
                        BUF_ID_RT=input[3];
                    }
                    else BUF_ID_RT=reg[ID_RT];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else if((MEM_OPCODE==0x08)||(MEM_OPCODE==0x09)||(MEM_OPCODE==0x0F)||(MEM_OPCODE==0x0C)||(MEM_OPCODE==0x0D)||(MEM_OPCODE==0x0E)||(MEM_OPCODE==0x0A)){ // addiu 0x09
                if(ID_RS!=0){
                    if(ID_RS==MEM_RT){
                        fwd_EX_DM_rs_toID=true;
                        BUF_ID_RS=input[3];
                    }
                    else BUF_ID_RS=reg[ID_RS];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT!=0){
                    if(ID_RT==MEM_RT){
                        fwd_EX_DM_rt_toID=true;
                        BUF_ID_RT=input[3];
                    }
                    else BUF_ID_RT=reg[ID_RT];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else if(MEM_OPCODE==0x03){
                if(ID_RS==31){
                    fwd_EX_DM_rs_toID=true;
                    BUF_ID_RS=input[3];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT==31){
                    fwd_EX_DM_rt_toID=true;
                    BUF_ID_RT=input[3];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else {
                BUF_ID_RT=reg[ID_RT];
                BUF_ID_RS=reg[ID_RS];
            }
            if(stall==false){
                if(BUF_ID_RT==BUF_ID_RS){
                    branch=1;
                    flush=true;
                }
            }
            break;
        case 0x05://bne
            ID_OPCODE=opcode;
            ID_RS=rs;
            ID_RT=rt;
            ID_Cimmediate=Cimmediate;
            if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){
                stall=true;
            }
            else if(((EX_OPCODE==0x08)||(EX_OPCODE==0x09)||(EX_OPCODE==0x0F)||(EX_OPCODE==0x0C)||(EX_OPCODE==0x0D)||(EX_OPCODE==0x0E)||(EX_OPCODE==0x0A))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){//-------addiu---0x09
                stall=true;
            }
            else if(((MEM_OPCODE==0x23)||(MEM_OPCODE==0x21)||(MEM_OPCODE==0x25)||(MEM_OPCODE==0x20)||(MEM_OPCODE==0x24))&&(((ID_RS==MEM_RT)&&(ID_RS!=0))||((ID_RT==MEM_RT)&&(ID_RT!=0)))){
                stall=true;
            }
            else if(((EX_OPCODE==0x00)&&((EX_FUNCT==0x20)||(EX_FUNCT==0x21)||(EX_FUNCT==0x22)||(EX_FUNCT==0x24)||(EX_FUNCT==0x25)||(EX_FUNCT==0x26)||(EX_FUNCT==0x27)||(EX_FUNCT==0x28)||(EX_FUNCT==0x2A)||(EX_FUNCT==0x00)||(EX_FUNCT==0x02)||(EX_FUNCT==0x03)))&&(((ID_RS==EX_RD)&&(ID_RS!=0))||((ID_RT==EX_RD)&&(ID_RT!=0)))){ //addu 0x21
                stall=true;
            }
            else if((MEM_OPCODE==0x00)&&((MEM_FUNCT==0x20)||(MEM_FUNCT==0x21)||(MEM_FUNCT==0x22)||(MEM_FUNCT==0x24)||(MEM_FUNCT==0x25)||(MEM_FUNCT==0x26)||(MEM_FUNCT==0x27)||(MEM_FUNCT==0x28)||(MEM_FUNCT==0x2A)||(MEM_FUNCT==0x00)||(MEM_FUNCT==0x02)||(MEM_FUNCT==0x03))){//addu-------
                if(ID_RS!=0){
                    if(ID_RS==MEM_RD){
                        fwd_EX_DM_rs_toID=true;
                        BUF_ID_RS=input[3];
                    }
                    else BUF_ID_RS=reg[ID_RS];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT!=0){
                    if(ID_RT==MEM_RD){
                        fwd_EX_DM_rt_toID=true;
                        BUF_ID_RT=input[3];
                    }
                    else BUF_ID_RT=reg[ID_RT];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else if((MEM_OPCODE==0x08)||(MEM_OPCODE==0x09)||(MEM_OPCODE==0x0F)||(MEM_OPCODE==0x0C)||(MEM_OPCODE==0x0D)||(MEM_OPCODE==0x0E)||(MEM_OPCODE==0x0A)){ // addiu 0x09
                if(ID_RS!=0){
                    if(ID_RS==MEM_RT){
                        fwd_EX_DM_rs_toID=true;
                        BUF_ID_RS=input[3];
                    }
                    else BUF_ID_RS=reg[ID_RS];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT!=0){
                    if(ID_RT==MEM_RT){
                        fwd_EX_DM_rt_toID=true;
                        BUF_ID_RT=input[3];
                    }
                    else BUF_ID_RT=reg[ID_RT];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else if(MEM_OPCODE==0x03){
                if(ID_RS==31){
                    fwd_EX_DM_rs_toID=true;
                    BUF_ID_RS=input[3];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT==31){
                    fwd_EX_DM_rt_toID=true;
                    BUF_ID_RT=input[3];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else {
                BUF_ID_RT=reg[ID_RT];
                BUF_ID_RS=reg[ID_RS];
            }
            if(stall==false){
                if(BUF_ID_RT!=BUF_ID_RS){
                    branch=1;
                    flush=true;
                }
            }
            break;
        case 0x07://bgtz
            ID_OPCODE=opcode;
            ID_RS=rs;
            ID_RT=rt;
            ID_Cimmediate=Cimmediate;
            if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){
                stall=true;
            }
            else if(((EX_OPCODE==0x08)||(EX_OPCODE==0x09)||(EX_OPCODE==0x0F)||(EX_OPCODE==0x0C)||(EX_OPCODE==0x0D)||(EX_OPCODE==0x0E)||(EX_OPCODE==0x0A))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){//-------addiu---0x09
                stall=true;
            }
            else if(((MEM_OPCODE==0x23)||(MEM_OPCODE==0x21)||(MEM_OPCODE==0x25)||(MEM_OPCODE==0x20)||(MEM_OPCODE==0x24))&&(((ID_RS==MEM_RT)&&(ID_RS!=0))||((ID_RT==MEM_RT)&&(ID_RT!=0)))){
                stall=true;
            }
            else if(((EX_OPCODE==0x00)&&((EX_FUNCT==0x20)||(EX_FUNCT==0x21)||(EX_FUNCT==0x22)||(EX_FUNCT==0x24)||(EX_FUNCT==0x25)||(EX_FUNCT==0x26)||(EX_FUNCT==0x27)||(EX_FUNCT==0x28)||(EX_FUNCT==0x2A)||(EX_FUNCT==0x00)||(EX_FUNCT==0x02)||(EX_FUNCT==0x03)))&&(((ID_RS==EX_RD)&&(ID_RS!=0))||((ID_RT==EX_RD)&&(ID_RT!=0)))){ //addu 0x21
                stall=true;
            }
            else if((MEM_OPCODE==0x00)&&((MEM_FUNCT==0x20)||(MEM_FUNCT==0x21)||(MEM_FUNCT==0x22)||(MEM_FUNCT==0x24)||(MEM_FUNCT==0x25)||(MEM_FUNCT==0x26)||(MEM_FUNCT==0x27)||(MEM_FUNCT==0x28)||(MEM_FUNCT==0x2A)||(MEM_FUNCT==0x00)||(MEM_FUNCT==0x02)||(MEM_FUNCT==0x03))){//addu-------
                if(ID_RS!=0){
                    if(ID_RS==MEM_RD){
                        fwd_EX_DM_rs_toID=true;
                        BUF_ID_RS=input[3];
                    }
                    else BUF_ID_RS=reg[ID_RS];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT!=0){
                    if(ID_RT==MEM_RD){
                        fwd_EX_DM_rt_toID=true;
                        BUF_ID_RT=input[3];
                    }
                    else BUF_ID_RT=reg[ID_RT];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else if((MEM_OPCODE==0x08)||(MEM_OPCODE==0x09)||(MEM_OPCODE==0x0F)||(MEM_OPCODE==0x0C)||(MEM_OPCODE==0x0D)||(MEM_OPCODE==0x0E)||(MEM_OPCODE==0x0A)){ // addiu 0x09
                if(ID_RS!=0){
                    if(ID_RS==MEM_RT){
                        fwd_EX_DM_rs_toID=true;
                        BUF_ID_RS=input[3];
                    }
                    else BUF_ID_RS=reg[ID_RS];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT!=0){
                    if(ID_RT==MEM_RT){
                        fwd_EX_DM_rt_toID=true;
                        BUF_ID_RT=input[3];
                    }
                    else BUF_ID_RT=reg[ID_RT];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else if(MEM_OPCODE==0x03){
                if(ID_RS==31){
                    fwd_EX_DM_rs_toID=true;
                    BUF_ID_RS=input[3];
                }
                else BUF_ID_RS=reg[ID_RS];
                if(ID_RT==31){
                    fwd_EX_DM_rt_toID=true;
                    BUF_ID_RT=input[3];
                }
                else BUF_ID_RT=reg[ID_RT];
            }
            else {
                BUF_ID_RT=reg[ID_RT];
                BUF_ID_RS=reg[ID_RS];
            }
            if(stall==false){
                if(BUF_ID_RS>0){
                    branch=1;
                    flush=true;
                }
            }
            break;
        case 0x0C://andi
        case 0x0D://ori
        case 0x0E://nori
            ID_OPCODE = opcode;
            ID_RS = rs;
            ID_RT = rt;
            ID_CimmediateUnsigned = CimmediateUnsigned;
            if(((EX_OPCODE==0x23)||(EX_OPCODE==0x21)||(EX_OPCODE==0x25)||(EX_OPCODE==0x20)||(EX_OPCODE==0x24))&&(ID_RS==EX_RT)&&(ID_RS!=0)){
                stall = true;
            }
            break;
        case 0x0F://lui
            ID_OPCODE=opcode;
            ID_RT=rt;
            ID_Cimmediate=Cimmediate;
            break;
        case 0x02://j
            ID_OPCODE=opcode;
            ID_Caddress=Caddress;
            flush=1;
            break;
        case 0x03://jal
            ID_OPCODE=opcode;
            ID_Caddress=Caddress;
            ID_RT=31;
            ID_RD=31;
            ID_RS=31;
            flush=1;
            input[1]=i;
            break;
        case 0x3F://halt
            ID_OPCODE=opcode;
            break;
    }
    return input;
}

void printRegister(){
    fprintf(writeSnapshot, "cycle %d\n", cycle);
    for(k = 0 ; k < 32; k++){
        fprintf(writeSnapshot, "$%.2d: 0x%.8X\n",k,reg[k]);
    }
    fprintf(writeSnapshot, "PC: 0x%.8X\n", curpc);
}

void printStage(){
    char mips[20];
    fprintf(writeSnapshot, "IF: 0x%08X", pipeline[0]);
    if(flush==true)fprintf(writeSnapshot, " to_be_flushed");
    if(stall==true){
        fprintf(writeSnapshot, " to_be_stalled");
    }
    fprintf(writeSnapshot, "\n");
    printOpcode(ID_OPCODE,ID_FUNCT,pipeline[1],mips);
    fprintf(writeSnapshot, "ID: %s", mips);
    if(stall==true){
        fprintf(writeSnapshot, " to_be_stalled");
    }
    if(fwd_EX_DM_rs_toID==true){
        fprintf(writeSnapshot, " fwd_EX-DM_rs_$%d",ID_RS);
        fwd_EX_DM_rs_toID=false;
    }
    if(fwd_EX_DM_rt_toID==true){
        fprintf(writeSnapshot, " fwd_EX-DM_rt_$%d",ID_RT);
        fwd_EX_DM_rt_toID=false;
    }
    fprintf(writeSnapshot, "\n");
    printOpcode(EX_OPCODE,EX_FUNCT,pipeline[2],mips);
    fprintf(writeSnapshot, "EX: %s", mips);
    if(fwd_DM_WB_rs_toEX==true){
        fprintf(writeSnapshot, " fwd_DM-WB_rs_$%d",EX_RS);
        fwd_DM_WB_rs_toEX=false;
    }else if(fwd_EX_DM_rs_toEX==true){
        fprintf(writeSnapshot, " fwd_EX-DM_rs_$%d",EX_RS);
        fwd_EX_DM_rs_toEX=false;
    }
    if(fwd_DM_WB_rt_toEX==true){
        fprintf(writeSnapshot, " fwd_DM-WB_rt_$%d",EX_RT);
        fwd_DM_WB_rt_toEX=false;
    }else if(fwd_EX_DM_rt_toEX==true){
        fprintf(writeSnapshot, " fwd_EX-DM_rt_$%d",EX_RT);
        fwd_EX_DM_rt_toEX=false;
    }
    fprintf(writeSnapshot, "\n");
    printOpcode(MEM_OPCODE,MEM_FUNCT,pipeline[3],mips);
    fprintf(writeSnapshot, "DM: %s\n", mips);
    printOpcode(WB_OPCODE,WB_FUNCT,pipeline[4],mips);
    fprintf(writeSnapshot, "WB: %s\n\n\n", mips);
}
void printOpcode(int opcodep,int functp,int pipep,char* result){
    if((pipep&0xFC1FFFFF)==0)sprintf(result, "NOP");
    else if(opcodep==0x00){
        if(functp==0x20)sprintf(result, "ADD");
        else if(functp==0x21)sprintf(result, "ADDU");
        else if(functp==0x22)sprintf(result, "SUB");
        else if(functp==0x24)sprintf(result, "AND");
        else if(functp==0x25)sprintf(result, "OR");
        else if(functp==0x26)sprintf(result, "XOR");
        else if(functp==0x27)sprintf(result, "NOR");
        else if(functp==0x28)sprintf(result, "NAND");
        else if(functp==0x2A)sprintf(result, "SLT");
        else if(functp==0x00)sprintf(result, "SLL");
        else if(functp==0x02)sprintf(result, "SRL");
        else if(functp==0x03)sprintf(result, "SRA");
        else if(functp==0x08)sprintf(result, "JR");
    }
    else if(opcodep==0x08)sprintf(result, "ADDI");
    else if(opcodep==0x09)sprintf(result, "ADDIU");
    else if(opcodep==0x23)sprintf(result, "LW");
    else if(opcodep==0x21)sprintf(result, "LH");
    else if(opcodep==0x25)sprintf(result, "LHU");
    else if(opcodep==0x20)sprintf(result, "LB");
    else if(opcodep==0x24)sprintf(result, "LBU");
    else if(opcodep==0x2B)sprintf(result, "SW");
    else if(opcodep==0x29)sprintf(result, "SH");
    else if(opcodep==0x28)sprintf(result, "SB");
    else if(opcodep==0x0F)sprintf(result, "LUI");
    else if(opcodep==0x0C)sprintf(result, "ANDI");
    else if(opcodep==0x0D)sprintf(result, "ORI");
    else if(opcodep==0x0E)sprintf(result, "NORI");
    else if(opcodep==0x0A)sprintf(result, "SLTI");
    else if(opcodep==0x04)sprintf(result, "BEQ");
    else if(opcodep==0x05)sprintf(result, "BNE");
    else if(opcodep==0x07)sprintf(result, "BGTZ");
    else if(opcodep==0x02)sprintf(result, "J");
    else if(opcodep==0x03)sprintf(result, "JAL");
    else if(opcodep==0x3F)sprintf(result, "HALT");
}
int toBigEndian(unsigned int k){
    int a,b,c,d;
    a=k>>24;
    b=((k<<8)>>24)<<8;
    c=((k>>8)<<24)>>8;
    d=k<<24;
    k=a+b+c+d;
    return k;
}
int main(){
    int pc,iNum,dNum,iCycle=0,dCycle=0;
    unsigned int iMemory[5000];
    int result[5];
    memset(iMemory,0,5000);
    memset(dMemory,0,5000);
    memset(pipe,0,5);
    memset(reg,0,100);
    FILE *iimage;
    FILE *dimage;
    iimage=fopen( "iimage.bin","rb");
    dimage=fopen( "dimage.bin","rb");
    writeSnapshot=fopen("snapshot.rpt","w");
    writeError=fopen("error_dump.rpt","w");
    fread(&pc,sizeof(int),1,iimage);
    pc=toBigEndian(pc);
    fread(&iNum,sizeof(int),1,iimage);
    iNum=toBigEndian(iNum);
    iCycle=pc;
    i=pc;
    num=0;
    while(num!=iNum){
        fread(&iMemory[iCycle],sizeof(int),1,iimage);
        iCycle+=4;
        num++;
    }
    fread(&reg[29],sizeof(int),1,dimage);
    reg[29]=toBigEndian(reg[29]);
    sp=reg[29];
    fread(&dNum,sizeof(int),1,dimage);
    dNum=toBigEndian(dNum);
    while(dCycle!=dNum){
        fread(&dMemory[dCycle],sizeof(int),1,dimage);
        dCycle++;
    }
    fclose(iimage);
    fclose(dimage);
    for(i=pc;i<iCycle;i+=4){
        iMemory[i]=toBigEndian(iMemory[i]);
    }
    for(i=0;i<dCycle;i++){
        dMemory[i]=toBigEndian(dMemory[i]);
    }
    while(1){
        if(cycle==0)curpc = i;
        printRegister();
        cycle++;
        result = WB(result);
        result = MEM(result);
        result = EX(result);
        result = ID(result);
        if(stall == false){
            pipeline[0] = iMemory[i];
            IF_OPCODE = pipeline[0]>>26;
            printStage();
            i = i+4;
            if((ID_OPCODE==0x02)||(ID_OPCODE==0x03)){
                i = ((i>>28)<<28)+ID_Caddress*4;
            }
            if(branch==1){
                branch = 0;
                i = i-4 + ID_Cimmediate*4;
            }
            if((ID_OPCODE==0x00)&&(ID_FUNCT==0x08)){
                i = BUF_ID_RS;
            }
            curpc = i;
        }else if(stall==true){
            pipeline[0] = iMemory[i];
            curpc = i;
            printStage();
        }
        if(halt == true) break;
        if((IF_OPCODE==0x3F)&&(ID_OPCODE==0x3F)&&(EX_OPCODE==0x3F)&&(MEM_OPCODE==0x3F)&&(WB_OPCODE==0x3F)) break;
    }
    fclose(writeSnapshot);
    fclose(writeError);
}