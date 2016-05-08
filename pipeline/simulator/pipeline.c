#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<stdbool.h>
FILE *writeSnapshot;
FILE *writeError;
void stallDetectRt();
void stallDetectRtRs();
void stallDetectRs();
void printRegister();
void printStage();
void MEM();
void WB();
void EX();
void isFwd();
void ID();
void forBranch();
int toBigEndian(unsigned int);
void printOP(int,int,int,char*);
unsigned int carryAddress,Tempp;
unsigned int pipeline[5];
int dMemory[5000];
int i=0,l,k,halt=0,TempSigned=0,sp,OP=0,FT=0,rs=0,rt=0,rd=0,cycle=0,Cshamt=0;
int Register[100],num;
unsigned int CimmediateUnsigned=0;
short Cimmediate=0;
int WB_Temp=0,MEM_Temp=0,EX_Temp=0,ID_Temp=0;
int IF_OP=0,ID_OP=0,EX_OP=0,MEM_OP=0,WB_OP=0;
int ID_FT=0,EX_FT=0,MEM_FT=0,WB_FT=0;
int ID_RS=0,EX_RS=0,MEM_RS=0,WB_RS=0;
int ID_RT=0,EX_RT=0,MEM_RT=0,WB_RT=0;
int ID_RD=0,EX_RD=0,MEM_RD=0,WB_RD=0;
int ID_Cshamt=0,EX_Cshamt=0,MEM_Cshamt=0,WB_Cshamt=0;
int ID_Cimmediate=0,EX_Cimmediate=0,MEM_Cimmediate=0,WB_Cimmediate=0;
int ID_CimmediateUnsigned=0,EX_CimmediateUnsigned=0,MEM_CimmediateUnsigned=0,WB_CimmediateUnsigned=0;
int ID_Caddress=0,EX_Caddress=0,MEM_Caddress=0,WB_Caddress=0;
bool flush=false,stall=false;
bool fwd_EX_DM_rs_toID=false,fwd_EX_DM_rt_toID=false;
bool fwd_EX_DM_rs_toEX=false,fwd_EX_DM_rt_toEX=false;
bool fwd_DM_WB_rs_toEX=0,fwd_DM_WB_rt_toEX=0;
bool  branch=false;
int BUF_ID_RS=0,BUF_ID_RT=0;
int curpc;

#define _RS 0
#define _RT 1
int result[5];
int BUF_RS,BUF_RT;
int main()
{
    int pc,iNum,dNum,iCycle=0,dCycle=0;
    unsigned int iMemory[5000];
    memset(iMemory,0,5000);
    memset(dMemory,0,5000);
    memset(pipeline,0,5);
    memset(Register,0,100);
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
    fread(&Register[29],sizeof(int),1,dimage);
    Register[29]=toBigEndian(Register[29]);
    sp=Register[29];
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

    for(i=pc;;){
        if(cycle==0)curpc=i;
        printRegister();
        cycle++;
        //WB
        WB();
        //MEM
        MEM();
        //EX
        EX();
        //ID
        ID();
        //IF
        if(stall!=1){
            pipeline[0]=iMemory[i];
            IF_OP=pipeline[0]>>26;
            //PC
            printStage();
            i+=4;
            if((ID_OP==0x02)||(ID_OP==0x03)){
                i=((i>>28)<<28)+ID_Caddress*4;
            }
            if(branch==1){
                branch=0;
                i=i-4+4*ID_Cimmediate;
            }
            if((ID_OP==0x00)&&(ID_FT==0x08)){
                i=BUF_ID_RS;
            }
            curpc=i;

        }
        else if(stall==1){
            pipeline[0]=iMemory[i];
            curpc=i;
            printStage();
        }
        if(halt==1) break;
        if((IF_OP==0x3F)&&(ID_OP==0x3F)&&(EX_OP==0x3F)&&(MEM_OP==0x3F)&&(WB_OP==0x3F))break;
    }

    fclose(writeSnapshot);
    fclose(writeError);
}


void printRegister(){
    fprintf(writeSnapshot, "cycle %d\n", cycle);
    for(k=0;k<32;k++){
        fprintf(writeSnapshot, "$%.2d: 0x%.8X\n",k,Register[k]);
    }
    fprintf(writeSnapshot, "PC: 0x%.8X\n", curpc);
}
void printStage(){
    char mips[20];
    fprintf(writeSnapshot, "IF: 0x%08X", pipeline[0]);
    if(flush==1)fprintf(writeSnapshot, " to_be_flushed");
    if(stall==1){
        fprintf(writeSnapshot, " to_be_stalled");
    }
    fprintf(writeSnapshot, "\n");
    printOP(ID_OP,ID_FT,pipeline[1],mips);
    fprintf(writeSnapshot, "ID: %s", mips);
    if(stall==1){
        fprintf(writeSnapshot, " to_be_stalled");
    }
    if(fwd_EX_DM_rs_toID==1){
        fprintf(writeSnapshot, " fwd_EX-DM_rs_$%d",ID_RS);
        fwd_EX_DM_rs_toID=0;}
    if(fwd_EX_DM_rt_toID==1){
        fprintf(writeSnapshot, " fwd_EX-DM_rt_$%d",ID_RT);
        fwd_EX_DM_rt_toID=0;}
    fprintf(writeSnapshot, "\n");
    printOP(EX_OP,EX_FT,pipeline[2],mips);
    fprintf(writeSnapshot, "EX: %s", mips);
    if(fwd_DM_WB_rs_toEX==1){
        fprintf(writeSnapshot, " fwd_DM-WB_rs_$%d",EX_RS);
        fwd_DM_WB_rs_toEX=0;}
    else if(fwd_EX_DM_rs_toEX==1){
        fprintf(writeSnapshot, " fwd_EX-DM_rs_$%d",EX_RS);
        fwd_EX_DM_rs_toEX=0;}
    if(fwd_DM_WB_rt_toEX==1){
        fprintf(writeSnapshot, " fwd_DM-WB_rt_$%d",EX_RT);
        fwd_DM_WB_rt_toEX=0;}
    else if(fwd_EX_DM_rt_toEX==1){
        fprintf(writeSnapshot, " fwd_EX-DM_rt_$%d",EX_RT);
        fwd_EX_DM_rt_toEX=0;}
    fprintf(writeSnapshot, "\n");
    printOP(MEM_OP,MEM_FT,pipeline[3],mips);
    fprintf(writeSnapshot, "DM: %s\n", mips);
    printOP(WB_OP,WB_FT,pipeline[4],mips);
    fprintf(writeSnapshot, "WB: %s\n\n\n", mips);
}
void printOP(int OPp,int FTp,int pipelinep,char* result){
    if((pipelinep&0xFC1FFFFF)==0)sprintf(result, "NOP");
    else if(OPp==0x00){
        if(FTp==0x20)sprintf(result, "ADD");
        else if(FTp==0x21)sprintf(result, "ADDU");
        else if(FTp==0x22)sprintf(result, "SUB");
        else if(FTp==0x24)sprintf(result, "AND");
        else if(FTp==0x25)sprintf(result, "OR");
        else if(FTp==0x26)sprintf(result, "XOR");
        else if(FTp==0x27)sprintf(result, "NOR");
        else if(FTp==0x28)sprintf(result, "NAND");
        else if(FTp==0x2A)sprintf(result, "SLT");
        else if(FTp==0x00)sprintf(result, "SLL");
        else if(FTp==0x02)sprintf(result, "SRL");
        else if(FTp==0x03)sprintf(result, "SRA");
        else if(FTp==0x08)sprintf(result, "JR");
    }
    else if(OPp==0x08)sprintf(result, "ADDI");
    else if(OPp==0x09)sprintf(result, "ADDIU");
    else if(OPp==0x23)sprintf(result, "LW");
    else if(OPp==0x21)sprintf(result, "LH");
    else if(OPp==0x25)sprintf(result, "LHU");
    else if(OPp==0x20)sprintf(result, "LB");
    else if(OPp==0x24)sprintf(result, "LBU");
    else if(OPp==0x2B)sprintf(result, "SW");
    else if(OPp==0x29)sprintf(result, "SH");
    else if(OPp==0x28)sprintf(result, "SB");
    else if(OPp==0x0F)sprintf(result, "LUI");
    else if(OPp==0x0C)sprintf(result, "ANDI");
    else if(OPp==0x0D)sprintf(result, "ORI");
    else if(OPp==0x0E)sprintf(result, "NORI");
    else if(OPp==0x0A)sprintf(result, "SLTI");
    else if(OPp==0x04)sprintf(result, "BEQ");
    else if(OPp==0x05)sprintf(result, "BNE");
    else if(OPp==0x07)sprintf(result, "BGTZ");
    else if(OPp==0x02)sprintf(result, "J");
    else if(OPp==0x03)sprintf(result, "JAL");
    else if(OPp==0x3F)sprintf(result, "HALT");
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
void MEM(){
    MEM_Caddress=EX_Caddress;
    MEM_CimmediateUnsigned=EX_CimmediateUnsigned;
    MEM_Cimmediate=EX_Cimmediate;
    MEM_Cshamt=EX_Cshamt;
    MEM_Temp=EX_Temp;
    MEM_FT=EX_FT;
    MEM_OP=EX_OP;
    pipeline[3]=pipeline[2];
    result[3]=result[2];
    MEM_RS=EX_RS;
    MEM_RD=EX_RD;
    MEM_RT=EX_RT;
    if(MEM_OP==0x00)MEM_RT=-1;
    else if(MEM_OP==0x23){//lw
        int Temp=Register[MEM_RS]+MEM_Cimmediate;
        //int TempTemp=Temp>>31;
        //int rsTemp=Register[MEM_RS]>>31;
        //int cTemp=MEM_Cimmediate>>15;
        if(Temp<0||(Temp+3<0)||(Temp>1023)||(Temp+3)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt=1;
        }
        if(Temp%4!=0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n", cycle);
            halt=1;
        }
        if(halt!=1){
            result[3]=dMemory[MEM_Temp/4];
            MEM_RD=-1;
        }
    }
    else if(MEM_OP==0x21){//lh
        int Temp=Register[MEM_RS]+MEM_Cimmediate;
        //int TempTemp=Temp>>31;
        //int rsTemp=Register[MEM_RS]>>31;
        //int cTemp=MEM_Cimmediate>>15;
        if(Temp<0||(Temp+1<0)||(Temp>1023)||(Temp+1)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt=1;
        }
        if(Temp%2!=0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n", cycle);
            halt=1;
        }
        if(halt!=1){
            if(MEM_Temp%4==0)result[3]=dMemory[MEM_Temp/4]>>16;
            else if(MEM_Temp%4==2)result[3]=(dMemory[MEM_Temp/4]<<16)>>16;
            MEM_RD=-1;
        }
    }
    else if(MEM_OP==0x25){//lhu
        int Temp=Register[MEM_RS]+MEM_Cimmediate;
        //int TempTemp=Temp>>31;
        //int rsTemp=Register[MEM_RS]>>31;
        //int cTemp=MEM_Cimmediate>>15;
        if(Temp<0||(Temp+1<0)||(Temp>1023)||(Temp+1)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt=1;
        }
        if(Temp%2!=0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n", cycle);
            halt=1;
        }
        if(halt!=1){
            if(MEM_Temp%4==0)result[3]=(dMemory[MEM_Temp/4]>>16)&0x0000FFFF;
            else if(MEM_Temp%4==2)result[3]=dMemory[MEM_Temp/4]&0x0000FFFF;
            MEM_RD=-1;
        }
    }
    else if(MEM_OP==0x20){//lb
        int Temp=Register[MEM_RS]+MEM_Cimmediate;
        //int TempTemp=Temp>>31;
        //int rsTemp=Register[MEM_RS]>>31;
        //int cTemp=MEM_Cimmediate>>15;
        if(Temp<0||Temp>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt=1;
        }
        if(halt!=1){
            if(MEM_Temp%4==0)result[3]=dMemory[MEM_Temp/4]>>24;
            else if(MEM_Temp%4==1)result[3]=(dMemory[MEM_Temp/4]<<8)>>24;
            else if(MEM_Temp%4==2)result[3]=(dMemory[MEM_Temp/4]<<16)>>24;
            else if(MEM_Temp%4==3)result[3]=(dMemory[MEM_Temp/4]<<24)>>24;
            MEM_RD=-1;
        }
    }
    else if(MEM_OP==0x24){//lbu
        int Temp=Register[MEM_RS]+MEM_Cimmediate;
        // int TempTemp=Temp>>31;
        //int rsTemp=Register[MEM_RS]>>31;
        //int cTemp=MEM_Cimmediate>>15;
        if(Temp<0||Temp>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt=1;
        }
        if(halt!=1){
            if(MEM_Temp%4==0)result[3]=(dMemory[MEM_Temp/4]>>24)&0x000000FF;
            else if(MEM_Temp%4==1)result[3]=((dMemory[MEM_Temp/4]<<8)>>24)&0x000000FF;
            else if(MEM_Temp%4==2)result[3]=((dMemory[MEM_Temp/4]<<16)>>24)&0x000000FF;
            else if(MEM_Temp%4==3)result[3]=dMemory[MEM_Temp/4]&0x000000FF;
            MEM_RD=-1;
        }
    }
    else if(MEM_OP==0x2B){//sw
        int Temp=Register[MEM_RS]+MEM_Cimmediate;
        //int TempTemp=Temp>>31;
        //int rsTemp=Register[rs]>>31;
        //int cTemp=Cimmediate>>15;
        if(Temp<0||(Temp+3<0)||(Temp>1023)||(Temp+3)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt=1;
        }
        if(Temp%4!=0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n", cycle);
            halt=1;
        }
        if(halt!=1){
            dMemory[MEM_Temp/4]=result[3];
            MEM_RT=-1;
            MEM_RD=-1;
        }
    }
    else if(MEM_OP==0x29){//sh
        int Temp=Register[MEM_RS]+MEM_Cimmediate;
        //int TempTemp=Temp>>31;
        //int rsTemp=Register[rs]>>31;
        //int cTemp=Cimmediate>>15;
        if(Temp<0||(Temp+1<0)||(Temp>1023)||(Temp+1)>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt=1;
        }
        if(Temp%2!=0){
            fprintf(writeError, "In cycle %d: Misalignment Error\n", cycle);
            halt=1;
        }
        if(halt!=1){
            if(MEM_Temp%4==0)dMemory[MEM_Temp/4]=(dMemory[MEM_Temp/4]&0x0000FFFF)+(result[3]<<16);
            else if(MEM_Temp%4==2)dMemory[MEM_Temp/4]=(dMemory[MEM_Temp/4]&0xFFFF0000)+result[3];
            MEM_RT=-1;
            MEM_RD=-1;
        }
    }
    else if(MEM_OP==0x28){//sb
        int Temp=Register[MEM_RS]+MEM_Cimmediate;
        //int TempTemp=Temp>>31;
        //int rsTemp=Register[rs]>>31;
        //int cTemp=Cimmediate>>15;
        if(Temp<0||Temp>1023){
            fprintf(writeError, "In cycle %d: Address Overflow\n", cycle);
            halt=1;
        }
        if(halt!=1){
            if(MEM_Temp%4==0)dMemory[MEM_Temp/4]=(dMemory[MEM_Temp/4]&0x00FFFFFF)+(result[3]<<24);
            else if(MEM_Temp%4==1)dMemory[MEM_Temp/4]=(dMemory[MEM_Temp/4]&0xFF00FFFF)+(result[3]<<16);
            else if(MEM_Temp%4==2)dMemory[MEM_Temp/4]=(dMemory[MEM_Temp/4]&0xFFFF00FF)+(result[3]<<8);
            else if(MEM_Temp%4==3)dMemory[MEM_Temp/4]=(dMemory[MEM_Temp/4]&0xFFFFFF00)+result[3];
            MEM_RT=-1;
            MEM_RD=-1;
        }
    }
    else if((MEM_OP==0x08)||(MEM_OP==0x09)||(MEM_OP==0x0F)||(MEM_OP==0x0C)||(MEM_OP==0x0D)||(MEM_OP==0x0E)||(MEM_OP==0x0A)){
        MEM_RD=-1;
    }
    else if((MEM_OP==0x02)||(MEM_OP==0x3F)){
        MEM_RT=-1;
        MEM_RD=-1;
    }
    else if(MEM_OP==0x03){
        MEM_RT=31;
        MEM_RD=31;
    }

}
void WB(){
    WB_Caddress=MEM_Caddress;
    WB_CimmediateUnsigned=MEM_CimmediateUnsigned;
    WB_Cimmediate=MEM_Cimmediate;
    WB_Cshamt=MEM_Cshamt;
    WB_Temp=MEM_Temp;
    WB_FT=MEM_FT;
    WB_OP=MEM_OP;
    pipeline[4]=pipeline[3];
    result[4]=result[3];
    WB_RD=MEM_RD;
    WB_RT=MEM_RT;
    if(WB_OP==0x00){
        if(WB_FT!=0x08&&((pipeline[4]&0xFC1FFFFF)!=0)){ //not jr
            if(WB_RD==0)fprintf(writeError, "In cycle %d: Write $0 Error\n", cycle);
            else Register[WB_RD]=result[4];
        }
    }
    else if(WB_OP==0x03){
        Register[31]=result[4];
    }
    else if((WB_OP!=0x04)&&(WB_OP!=0x05)&&(WB_OP!=0x07)&&(WB_OP!=0x2B)&&(WB_OP!=0x29)&&(WB_OP!=0x28)&&(WB_OP!=0x02)&&(WB_OP!=0x3F)){
        if(WB_RT==0)fprintf(writeError, "In cycle %d: Write $0 Error\n", cycle);
        else Register[WB_RT]=result[4];
    }
}
void EX(){
    if(stall==true){
        EX_Caddress=0;
        EX_CimmediateUnsigned=0;
        EX_Cimmediate=0;
        EX_Cshamt=0;
        EX_FT=0;
        EX_OP=0;
        pipeline[2]=0;
        EX_RS=0;
        EX_RT=0;
        EX_RD=0;
    }
    else if(stall==false){
        EX_Caddress=ID_Caddress;
        EX_CimmediateUnsigned=ID_CimmediateUnsigned;
        EX_Cimmediate=ID_Cimmediate;
        EX_Cshamt=ID_Cshamt;
        EX_FT=ID_FT;
        EX_OP=ID_OP;
        pipeline[2]=pipeline[1];
        result[2]=result[1];
        EX_RS=ID_RS;
        EX_RT=ID_RT;
        EX_RD=ID_RD;


        if(EX_OP==0x00){
            if((EX_FT==0x20)||(EX_FT==0x21)||(EX_FT==0x22)||(EX_FT==0x24)||(EX_FT==0x25)||(EX_FT==0x26)||(EX_FT==0x27)||(EX_FT==0x28)||(EX_FT==0x2A)){
                isFwd(3);
            }
            if(EX_FT==0x20){//add
                int resultTemp=BUF_RS+BUF_RT;
                int resultSign=resultTemp>>31;
                int rsTemp=BUF_RS>>31;
                int rtTemp=BUF_RT>>31;
                if((rsTemp==rtTemp)&&(resultSign!=rsTemp)){
                    fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
                }
                result[2]=resultTemp;
            }
            else if(EX_FT==0x21){//addu
                int resultTemp=BUF_RS+BUF_RT;
                //int resultSign=resultTemp>>31;
                result[2]=resultTemp;
            }
            else if(EX_FT==0x22){//sub
                int resultTemp=BUF_RS-BUF_RT;
                int resultSign=resultTemp>>31;
                int rsTemp=BUF_RS>>31;
                int rtTemp=(0-BUF_RT)>>31;
                if((rsTemp==rtTemp)&&(resultSign!=rsTemp)){
                    fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
                }
                result[2]=resultTemp;
            }
            else if(EX_FT==0x24){//and
                result[2]=BUF_RS&BUF_RT;
            }
            else if(EX_FT==0x25){//or
                result[2]=BUF_RS|BUF_RT;
            }
            else if(EX_FT==0x26){//xor
                result[2]=BUF_RS^BUF_RT;
            }
            else if(EX_FT==0x27){//nor
                result[2]=~(BUF_RS|BUF_RT);
            }
            else if(EX_FT==0x28){//nand
                result[2]=~(BUF_RS&BUF_RT);
            }
            else if(EX_FT==0x2A){//slt
                result[2]=(BUF_RS<BUF_RT);
            }
            else if(EX_FT==0x00){//sll
                isFwd(_RT);
                result[2]=BUF_RT<<EX_Cshamt;

            }
            else if(EX_FT==0x02){//srl
                isFwd(_RT);
                result[2]=BUF_RT;
                for(l=0;l<EX_Cshamt;l++){
                    result[2]=(result[2]>>1)&0x7FFFFFFF;
                }

            }
            else if(EX_FT==0x03){//sra
                isFwd(_RT);
                result[2]=BUF_RT>>EX_Cshamt;
            }
        }
        else if(EX_OP==0x08){//addi
            isFwd(_RS);
            int resultTemp=BUF_RS+EX_Cimmediate;
            int resultSign=resultTemp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(resultSign!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            result[2]=resultTemp;
        }
        else if(EX_OP==0x09){//addiu
            isFwd(_RS);
            int resultTemp=BUF_RS+EX_Cimmediate;
            result[2]=resultTemp;
        }
        else if(EX_OP==0x23){//lw
            isFwd(_RS);
            int Temp=BUF_RS+EX_Cimmediate;
            int TempTemp=Temp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(TempTemp!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_Temp=BUF_RS+EX_Cimmediate;
        }
        else if(EX_OP==0x21){//lh
            isFwd(_RS);
            int Temp=BUF_RS+EX_Cimmediate;
            int TempTemp=Temp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(TempTemp!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_Temp=BUF_RS+EX_Cimmediate;
        }
        else if(EX_OP==0x25){//lhu
            isFwd(_RS);
            int Temp=BUF_RS+EX_Cimmediate;
            int TempTemp=Temp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(TempTemp!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_Temp=BUF_RS+EX_Cimmediate;
        }
        else if(EX_OP==0x20){//lb
            isFwd(_RS);
            int Temp=BUF_RS+EX_Cimmediate;
            int TempTemp=Temp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(TempTemp!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_Temp=BUF_RS+EX_Cimmediate;
        }
        else if(EX_OP==0x24){//lbu
            isFwd(_RS);
            int Temp=BUF_RS+EX_Cimmediate;
            int TempTemp=Temp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(TempTemp!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_Temp=BUF_RS+EX_Cimmediate;
        }
        else if(EX_OP==0x2B){//sw
            isFwd(3);
            int Temp=BUF_RS+EX_Cimmediate;
            int TempTemp=Temp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(TempTemp!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_Temp=BUF_RS+EX_Cimmediate;
            result[2]=BUF_RT;
        }
        else if(EX_OP==0x29){//sh
            isFwd(3);
            int Temp=BUF_RS+EX_Cimmediate;
            int TempTemp=Temp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(TempTemp!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_Temp=BUF_RS+EX_Cimmediate;
            result[2]=BUF_RT&0x0000FFFF;
        }
        else if(EX_OP==0x28){//sb
            isFwd(3);
            int Temp=BUF_RS+EX_Cimmediate;
            int TempTemp=Temp>>31;
            int rsTemp=BUF_RS>>31;
            int cTemp=EX_Cimmediate>>15;
            if((rsTemp==cTemp)&&(TempTemp!=rsTemp)){
                fprintf(writeError, "In cycle %d: Number Overflow\n", cycle);
            }
            EX_Temp=BUF_RS+EX_Cimmediate;
            result[2]=BUF_RT&0x000000FF;
        }
        else if(EX_OP==0x0F){//lui
            result[2]=EX_Cimmediate<<16;
        }
        else if(EX_OP==0x0C){//andi
            isFwd(_RS);
            result[2]=BUF_RS&EX_CimmediateUnsigned;
        }
        else if(EX_OP==0x0D){//ori
            isFwd(_RS);
            result[2]=BUF_RS|EX_CimmediateUnsigned;
        }
        else if(EX_OP==0x0E){//nori
            isFwd(_RS);
            result[2]=~(BUF_RS|EX_CimmediateUnsigned);
        }
        else if(EX_OP==0x0A){//slti
            isFwd(_RS);
            result[2]=(BUF_RS<EX_Cimmediate);
        }
    }
}
void isFwd(int what){
    if(what!=_RT){
        if(EX_RS != 0){
            if(((EX_RS==MEM_RD)&&((MEM_OP==0x00)&&(MEM_FT!=0x08)))||((EX_RS==MEM_RT)&&((MEM_OP==0x08)||(MEM_OP==0x09)||(MEM_OP==0x0F)||(MEM_OP==0x0C)||(MEM_OP==0x0D)||(MEM_OP==0x0E)||(MEM_OP==0x0A)||(MEM_OP==0x03)))){
                BUF_RS=result[3];
                fwd_EX_DM_rs_toEX=1;
            }else if(((EX_RS==WB_RD)&&((WB_OP==0x00)&&(WB_FT!=0x08)))||((EX_RS==WB_RT)&&((WB_OP!=0x00)&&(WB_OP!=0x2B)&&(WB_OP!=0x29)&&(WB_OP!=0x28)&&(WB_OP!=0x04)&&(WB_OP!=0x05)&&(WB_OP!=0x07)&&(WB_OP!=0x02)&&(WB_OP!=0x3F)))){
                fwd_DM_WB_rs_toEX=1;
                BUF_RS=Register[EX_RS];
            }else BUF_RS=Register[EX_RS];
        }else BUF_RS=Register[EX_RS];
    }
    if(what!=_RS){
        if(EX_RT != 0){
            if(((EX_RT==MEM_RD)&&((MEM_OP==0x00)&&(MEM_FT!=0x08)))||((EX_RT==MEM_RT)&&((MEM_OP==0x08)||(MEM_OP==0x09)||(MEM_OP==0x0F)||(MEM_OP==0x0C)||(MEM_OP==0x0D)||(MEM_OP==0x0E)||(MEM_OP==0x0A)||(MEM_OP==0x03)))){
                BUF_RT=result[3];
                fwd_EX_DM_rt_toEX=1;
            }else if(((EX_RT==WB_RD)&&((WB_OP==0x00)&&(WB_FT!=0x08)))||((EX_RT==WB_RT)&&((WB_OP!=0x00)&&(WB_OP!=0x2B)&&(WB_OP!=0x29)&&(WB_OP!=0x28)&&(WB_OP!=0x04)&&(WB_OP!=0x05)&&(WB_OP!=0x07)&&(WB_OP!=0x02)&&(WB_OP!=0x3F)))){
                fwd_DM_WB_rt_toEX=1;
                BUF_RT=Register[EX_RT];
            }else BUF_RT=Register[EX_RT];
        }else BUF_RT=Register[EX_RT];
    }

}

void ID(){
    if(stall==false){
        pipeline[1]=pipeline[0];
    }
    else stall=false;
    if(flush==true){
        pipeline[1]=0;
        flush=0;
        stall = false;
    }
    OP=pipeline[1]>>26;
    FT=(pipeline[1]<<26)>>26;
    rs=(pipeline[1]<<6)>>27;
    rt=(pipeline[1]<<11)>>27;
    rd=(pipeline[1]<<16)>>27;
    Cshamt=(pipeline[1]<<21)>>27;
    Cimmediate=((pipeline[1]<<16)>>16);
    CimmediateUnsigned=pipeline[1]&0x0000FFFF;
    carryAddress=(pipeline[1]<<6)>>6;
    result[1]=result[0];
    switch(OP){
        case 0x00:
            switch(FT){
                case 0x20://add
                case 0x21://addu
                case 0x22://sub
                case 0x24://and
                case 0x25://or
                case 0x26://xor
                case 0x27://nor
                case 0x28://nand
                case 0x2A://slt
                    ID_OP=OP;
                    ID_FT=FT;
                    ID_RS=rs;
                    ID_RT=rt;
                    ID_RD=rd;
                    stallDetectRtRs();
                    break;
                case 0x00://sll
                case 0x02://srl
                case 0x03://sra
                    ID_OP=OP;
                    ID_FT=FT;
                    ID_RT=rt;
                    ID_RD=rd;
                    ID_Cshamt=Cshamt;
                    stallDetectRt();
                    break;
                case 0x08://jr
                    ID_OP=OP;
                    ID_FT=FT;
                    ID_RS=rs;
                    if(((EX_OP==0x23)||(EX_OP==0x21)||(EX_OP==0x25)||(EX_OP==0x20)||(EX_OP==0x24)||(EX_OP==0x08)||(EX_OP==0x09)||(EX_OP==0x0F)||(EX_OP==0x0C)||(EX_OP==0x0D)||(EX_OP==0x0E)||(EX_OP==0x0A))&&((ID_RS==EX_RT)&&(ID_RS!=0))){
                        stall=1;
                    }
                    else if(((MEM_OP==0x23)||(MEM_OP==0x21)||(MEM_OP==0x25)||(MEM_OP==0x20)||(MEM_OP==0x24))&&((ID_RS==MEM_RT)&&(ID_RS!=0))){
                        stall=1;
                    }
                    else if(((EX_OP==0x00)&&((EX_FT==0x20)||(EX_FT==0x21)||(EX_FT==0x22)||(EX_FT==0x24)||(EX_FT==0x25)||(EX_FT==0x26)||(EX_FT==0x27)||(EX_FT==0x28)||(EX_FT==0x2A)||(EX_FT==0x00)||(EX_FT==0x02)||(EX_FT==0x03)))&&((ID_RS==EX_RD)&&(ID_RS!=0))){
                        stall=1;
                    }
                    else if((MEM_OP==0x00)&&((MEM_FT==0x20)||(MEM_FT==0x21)||(MEM_FT==0x22)||(MEM_FT==0x24)||(MEM_FT==0x25)||(MEM_FT==0x26)||(MEM_FT==0x27)||(MEM_FT==0x28)||(MEM_FT==0x2A)||(MEM_FT==0x00)||(MEM_FT==0x02)||(MEM_FT==0x03))){
                        if(ID_RS!=0){
                            if(ID_RS==MEM_RD){

                                fwd_EX_DM_rs_toID=1;
                                BUF_ID_RS=result[3];
                            }
                            else BUF_ID_RS=Register[ID_RS];
                        }
                        else BUF_ID_RS=Register[ID_RS];
                    }
                    else if((MEM_OP==0x08)||(MEM_OP==0x09)||(MEM_OP==0x0F)||(MEM_OP==0x0C)||(MEM_OP==0x0D)||(MEM_OP==0x0E)||(MEM_OP==0x0A)){
                        if(ID_RS!=0){
                            if(ID_RS==MEM_RT){
                                fwd_EX_DM_rs_toID=1;
                                BUF_ID_RS=result[3];
                            }
                            else BUF_ID_RS=Register[ID_RS];
                        }
                        else BUF_ID_RS=Register[ID_RS];
                    }
                    else if(MEM_OP==0x03){
                        if(ID_RS==31){
                            fwd_EX_DM_rs_toID=1;
                            BUF_ID_RS=result[3];
                        }
                        else BUF_ID_RS=Register[ID_RS];
                    }
                    else {
                        BUF_ID_RS=Register[ID_RS];
                    }
                    if(stall==0){
                        flush=1;
                        stall = 0;
                    }
                    break;
        }
        break;
        case 0x23://lw
        case 0x21://lh
        case 0x25://lhu
        case 0x20://lb
        case 0x24://lbu
            ID_OP=OP;
            ID_RS=rs;
            ID_RT=rt;
            ID_Cimmediate=Cimmediate;
            stallDetectRs();
            break;
        case 0x2B://sw
        case 0x29://sh
        case 0x28://sb
            ID_OP=OP;
            ID_RS=rs;
            ID_RT=rt;
            ID_Cimmediate=Cimmediate;
            stallDetectRtRs();
            break;
        case 0x08://addi
        case 0x09://addiu
        case 0x0A://slti
            ID_OP=OP;
            ID_Cimmediate=Cimmediate;
            ID_RS=rs;
            ID_RT=rt;
            stallDetectRs();
            break;
        case 0x04://beq
            forBranch();
            if(stall!=1){
                if(BUF_ID_RT==BUF_ID_RS){
                    branch=1;
                    flush=1;
                }
            }
            break;
        case 0x05://bne
            forBranch();
            if(stall!=1){
                if(BUF_ID_RT!=BUF_ID_RS){
                    branch=1;
                    flush=1;
                }
            }
            break;
        case 0x07://bgtz
            forBranch();
            if(stall!=1){
                if(BUF_ID_RS>0){
                    branch=1;
                    flush=1;
                }
            }
            break;
        case 0x0C://andi
        case 0x0D://ori
        case 0x0E://nori
            ID_OP=OP;
            ID_RS=rs;
            ID_RT=rt;
            ID_CimmediateUnsigned=CimmediateUnsigned;
            stallDetectRs();
            break;
        case 0x0F://lui
            ID_OP=OP;
            ID_RT=rt;
            ID_Cimmediate=Cimmediate;
            break;
        case 0x02://j
            ID_OP=OP;
            ID_Caddress=carryAddress;
            flush=1;
            break;
        case 0x03://jal
            ID_OP=OP;
            ID_Caddress=carryAddress;
            ID_RT=31;
            ID_RD=31;
            ID_RS=31;
            flush=1;
            result[1]=i;
            break;
        case 0x3F://halt
            ID_OP=OP;
            break;
        case 0x35:
            stall = false;
            break;
    }
}
void forBranch(){
    ID_OP=OP;
    ID_RS=rs;
    ID_RT=rt;
    ID_Cimmediate=Cimmediate;
    if(((EX_OP==0x23)||(EX_OP==0x21)||(EX_OP==0x25)||(EX_OP==0x20)||(EX_OP==0x24))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){
        stall=1;
    }
    else if(((EX_OP==0x08)||(EX_OP==0x09)||(EX_OP==0x0F)||(EX_OP==0x0C)||(EX_OP==0x0D)||(EX_OP==0x0E)||(EX_OP==0x0A))&&(((ID_RS==EX_RT)&&(ID_RS!=0))||((ID_RT==EX_RT)&&(ID_RT!=0)))){
        if(ID_OP==0x07&&EX_OP==0x0F&&ID_RT==EX_RT){
            stall = false;
        }else stall=true;
    }
    else if(((MEM_OP==0x23)||(MEM_OP==0x21)||(MEM_OP==0x25)||(MEM_OP==0x20)||(MEM_OP==0x24))&&(((ID_RS==MEM_RT)&&(ID_RS!=0))||((ID_RT==MEM_RT)&&(ID_RT!=0)))){
        stall=1;
    }
    else if(((EX_OP==0x00)&&((EX_FT==0x20)||(MEM_FT==0x21)||(EX_FT==0x22)||(EX_FT==0x24)||(EX_FT==0x25)||(EX_FT==0x26)||(EX_FT==0x27)||(EX_FT==0x28)||(EX_FT==0x2A)||(EX_FT==0x00)||(EX_FT==0x02)||(EX_FT==0x03)))&&(((ID_RS==EX_RD)&&(ID_RS!=0))||((ID_RT==EX_RD)&&(ID_RT!=0)))){
        stall=1;
    }
    else if((MEM_OP==0x00)&&((MEM_FT==0x20)||(MEM_FT==0x21)||(MEM_FT==0x22)||(MEM_FT==0x24)||(MEM_FT==0x25)||(MEM_FT==0x26)||(MEM_FT==0x27)||(MEM_FT==0x28)||(MEM_FT==0x2A)||(MEM_FT==0x00)||(MEM_FT==0x02)||(MEM_FT==0x03))){
        if(ID_RS!=0){
            if(ID_RS==MEM_RD){
                fwd_EX_DM_rs_toID=1;
                BUF_ID_RS=result[3];
            }
            else BUF_ID_RS=Register[ID_RS];
        }
        else BUF_ID_RS=Register[ID_RS];
        if(ID_RT!=0){
            if(ID_RT==MEM_RD){
                fwd_EX_DM_rt_toID=1;
                BUF_ID_RT=result[3];
            }
            else BUF_ID_RT=Register[ID_RT];
        }
        else BUF_ID_RT=Register[ID_RT];
    }
    else if((MEM_OP==0x08)||(MEM_OP==0x09)||(MEM_OP==0x0F)||(MEM_OP==0x0C)||(MEM_OP==0x0D)||(MEM_OP==0x0E)||(MEM_OP==0x0A)){
        if(ID_RS!=0){
            if(ID_RS==MEM_RT){
                fwd_EX_DM_rs_toID=1;
                BUF_ID_RS=result[3];
            }
            else BUF_ID_RS=Register[ID_RS];
        }
        else BUF_ID_RS=Register[ID_RS];
        if(ID_RT!=0){
            if(ID_RT==MEM_RT){
                fwd_EX_DM_rt_toID=1;
                BUF_ID_RT=result[3];
            }
            else BUF_ID_RT=Register[ID_RT];
        }
        else BUF_ID_RT=Register[ID_RT];
    }
    else if(MEM_OP==0x03){
        if(ID_RS==31){
            fwd_EX_DM_rs_toID=1;
            BUF_ID_RS=result[3];
        }
        else BUF_ID_RS=Register[ID_RS];
        if(ID_RT==31){
            fwd_EX_DM_rt_toID=1;
            BUF_ID_RT=result[3];
        }
        else BUF_ID_RT=Register[ID_RT];
    }
    else {
        BUF_ID_RT=Register[ID_RT];
        BUF_ID_RS=Register[ID_RS];
    }
}
void stallDetectRtRs(){
    int MEM_nop = pipeline[3];
    int EX_nop = pipeline[2];
    int WB_nop = pipeline[0];
    /*
    Check if ID_RT and ID_RS equal to EX_RD
    if RT=RS=EX_RD
        1,if EX is load type must stall
        2,if EX is not load type can fwd
    else if RT==EX_RD but RS!=EX_RD
        1,if EX is load type must stall
        2,if RS!=MEM_RD fwd
        3,if RS==MEM_RD and MEM is not branch type stall!
    */
    //check not branch not jr
    if(((EX_OP==0x00)&&(EX_FT==0x08))||EX_OP==0x04||EX_OP==0x05||EX_OP==0x07||EX_OP==0x02||EX_OP==0x03){ // jr, beq, bne, bgtz

    if(MEM_nop==0){
             stall = false;
            }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT != MEM_RD){

                    }else if(ID_RT==MEM_RD){

                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RT==MEM_RT){

                    stall = true;
                }else if(ID_RT!=MEM_RT){
                    //do nothing
                }
            }

        if(MEM_nop==0){
             stall = false;
            }else if(MEM_OP==0x00){ //MEM R-type

                if(MEM_FT==0x08){

                }else{
                    if(ID_RS != MEM_RD){

                    }else if(ID_RS==MEM_RD && ID_RS != 0){

                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RS==MEM_RT && ID_RS != 0){

                    stall = true;
                }else if(ID_RS!=MEM_RT){
                    //do nothing
                }
            }


    }else if(EX_OP==0x00){ //EX_RD type check if equal EX_RD
        if(ID_RT==EX_RD && ID_RS==EX_RD &&ID_RT!=0&&ID_RS!=0 ){
        //FWD
        }else if(ID_RT==EX_RD && ID_RS != EX_RD ){

        /*
        RS!=MEM_RD fwd
        RS==MEM_RD and MEM_RD is not branch -> stall
        */
        if(MEM_nop==0){

        }else if(MEM_OP==0x00){ //MEM R-type

                if(MEM_FT==0x08){

                }else{
                    if(ID_RS != MEM_RD && ID_RS!=0){

                    }else if(ID_RS==MEM_RD&&ID_RS!=0){
                            stall = true;

                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RS==MEM_RT && ID_RS != 0){
                if(MEM_OP==0x2B||MEM_OP==0x29||MEM_OP==0x28){

                }else stall = true;
                }else if(ID_RS!=MEM_RT && ID_RS != 0){
                    //do nothing
                }
            }
        }else if(ID_RS==EX_RD && ID_RT != EX_RD){

        if(MEM_nop==0){

        }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT != MEM_RD && ID_RT != 0){

                    }else if(ID_RT==MEM_RD && ID_RT!= 0){
                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RT==MEM_RT && ID_RT !=0){
                    stall = true;
                }else if(ID_RT!=MEM_RT && ID_RT != 0){
                    //do nothing
                }
            }
        }else if(ID_RS!=EX_RD && ID_RT!=EX_RD){



                if(MEM_nop==0){

        }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT == MEM_RD && ID_RS != MEM_RD && ID_RT != 0){
                         stall = true;
                    }else if(ID_RS == MEM_RD && ID_RT != MEM_RD && ID_RS != 0){
                        stall = true;
                    }else if(ID_RS ==MEM_RD && ID_RT== MEM_RD && MEM_RD != 0){
                        stall = true;
                    }else{

                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else{
                    if(ID_RT == MEM_RT && ID_RS != MEM_RT && ID_RT != 0){
                         stall = true;
                    }else if(ID_RS == MEM_RT && ID_RT != MEM_RT && ID_RS != 0){
                        stall = true;
                    }else if(ID_RS ==MEM_RT && ID_RT== MEM_RT && MEM_RT != 0){
                        stall = true;
                    }else{

                    }
                }
            }
        }
    }else{ // check EX_RT
        //if(cycle==7) printf("ID_RS = %d, ID_RT = %d, EX_RD = %d,EX_RT = %d,EX_RS = %d , MEM_RS = %d ,MEM_RT = %d", ID_RS, ID_RT, EX_RD,EX_RT,EX_RS, MEM_RS,MEM_RT);
        //if(cycle==7) printf("ID_RT = %d, ID_RS = %d, EX_RT = %d, MEM_RT = %d", ID_RT, ID_RS, EX_RT, MEM_RT);
        if(ID_RT==EX_RT && ID_RS==EX_RT && ID_RT!=0&&ID_RS!=0){
            if(EX_OP==0x23||EX_OP==0x21||EX_OP==0x25||EX_OP==0x20||EX_OP==0x24){ // load type
                stall = true;
            }else{
                // FWD
            }
        }else if(ID_RT==EX_RT && ID_RS != EX_RT &&ID_RT !=0){

            if(EX_OP==0x23||EX_OP==0x21||EX_OP==0x25||EX_OP==0x20||EX_OP==0x24){ // load type
                stall = true;
            }
            if(EX_OP==0x2B||EX_OP==0x29||EX_OP==0x28){
                if(EX_RT==MEM_RT){
                    stall =  true;
                }
                if(EX_RT==MEM_RD){
                    stall = true;
                }
            }
             if(MEM_OP==0x00){ //MEM R-type
            if(MEM_nop==0){

        }else if(MEM_FT==0x08){

                }else{
                    if(ID_RS != MEM_RD && ID_RS!=0){

                    }else if(ID_RS==MEM_RD && ID_RS!=0){
                        stall = true;
                    }
                }
            }else{ // check RT
                        if(MEM_nop==0){

        }else if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RS==MEM_RT && ID_RS != 0){
                    stall = true;
                }else if(ID_RS!=MEM_RT&& ID_RS != 0){
                    //do nothing
                }
            }
        }else if(ID_RS==EX_RT && ID_RT!=EX_RT && ID_RS != 0){

            if(EX_OP==0x23||EX_OP==0x21||EX_OP==0x25||EX_OP==0x20||EX_OP==0x24){ // load type
                stall = true;
            }
            if(EX_OP==0x2B||EX_OP==0x29||EX_OP==0x28){
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){

                }else if(EX_RT==MEM_RT){
                    stall =  true;
                }
            }

            if(MEM_nop==0){

        }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT != MEM_RD &&ID_RT != 0){

                    }else if(ID_RT==MEM_RD && ID_RT != 0){
                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RT==MEM_RT && ID_RT != 0){
                    stall = true;
                }else if(ID_RT!=MEM_RT && ID_RT != 0){
                    //do nothing
                }
            }
        }else if(ID_RS != EX_RT && ID_RT != EX_RT ){
                    //if(cycle==6) printf("ID_RT = %d, ID_RS = %d, EX_RT = %d, MEM_RT = %d", ID_RT, ID_RS, EX_RT, MEM_RT);
            if(MEM_nop==0){

        }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT == MEM_RD && ID_RS != MEM_RD && ID_RT != 0){
                         stall = true;
                    }else if(ID_RS == MEM_RD && ID_RT != MEM_RD && ID_RS != 0){
                        stall = true;
                    }else if(ID_RS ==MEM_RD && ID_RT== MEM_RD && MEM_RD != 0){
                        stall = true;
                    }else{

                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else{
                    if(ID_RT == MEM_RT && ID_RS != MEM_RT && ID_RT != 0){
                         stall = true;
                    }else if(ID_RS == MEM_RT && ID_RT != MEM_RT && ID_RS != 0){
                        stall = true;
                    }else if(ID_RS ==MEM_RT && ID_RT== MEM_RT && MEM_RT != 0){
                        stall = true;
                    }else{

                    }
                }
            }
        }
    }
    if(EX_nop==0){
        if(MEM_nop==0){

        }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT == MEM_RD && ID_RS != MEM_RD && ID_RT != 0){
                         stall = true;
                    }else if(ID_RS == MEM_RD && ID_RT != MEM_RD && ID_RS != 0){
                        stall = true;
                    }else if(ID_RS ==MEM_RD && ID_RT== MEM_RD && MEM_RD != 0){
                        stall = true;
                    }else{

                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else{
                    if(ID_RT == MEM_RT && ID_RS != MEM_RT && ID_RT != 0){
                         stall = true;
                    }else if(ID_RS == MEM_RT && ID_RT != MEM_RT && ID_RS != 0){
                        stall = true;
                    }else if(ID_RS ==MEM_RT && ID_RT== MEM_RT && MEM_RT != 0){
                        stall = true;
                    }else{

                    }
                }
            }
    }
}

void stallDetectRs(){
    int MEM_nop = pipeline[3];
    int EX_nop = pipeline[2];
    int WB_nop = pipeline[0];
    if(((EX_OP==0x00)&&(EX_FT==0x08))||EX_OP==0x04||EX_OP==0x05||EX_OP==0x07||EX_OP==0x02||EX_OP==0x03){
        if(MEM_nop==0){
             stall = false;
            }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RS != MEM_RD){

                    }else if(ID_RS==MEM_RD && ID_RS != 0){

                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RS==MEM_RT){

                    stall = true;
                }else if(ID_RS!=MEM_RT && ID_RS != 0){
                    //do nothing
                }
            }
    }else if(EX_OP==0x00){
        if(ID_RS==EX_RD && ID_RS != 0){

        }else if(ID_RS!=EX_RD && ID_RS != 0){
            if(MEM_nop==0){
             stall = false;
            }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RS != MEM_RD){

                    }else if(ID_RS==MEM_RD){

                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RS==MEM_RT){

                    stall = true;
                }else if(ID_RS!=MEM_RT){
                    //do nothing
                }
            }
        }

    }else{

        if(ID_RS==EX_RT && ID_RS!=0){

            if(EX_OP==0x23||EX_OP==0x21||EX_OP==0x25||EX_OP==0x20||EX_OP==0x24){ // load type
                 stall = true;
            }
            else if(EX_OP==0x2B||EX_OP==0x29||EX_OP==0x28){
                if(EX_RT==MEM_RT){
                    stall = true;
                }else stall = false;
            }else{
                // FWD
            }
        }else if(ID_RS!=EX_RT && ID_RS != 0){

            if(MEM_nop==0){
             stall = false;
            }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RS != MEM_RD){

                    }else if(ID_RS==MEM_RD){
                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RS==MEM_RT){
                if(MEM_OP==0x2B||MEM_OP==0x29||MEM_OP==0x28){

                }else stall = true;
                }else if(ID_RS!=MEM_RT){
                    //do nothing
                }
            }
        }

    }
    if(EX_nop==0){


        if(MEM_nop==0){

        }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RS != MEM_RD){

                    }else if(ID_RS==MEM_RD && ID_RS!=0){

                        stall = true;
                    }
                }
            }else{ // check RT

                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RS==MEM_RT && ID_RS != 0){

                if(MEM_OP==0x2B||MEM_OP==0x29||MEM_OP==0x28||MEM_OP==0x23||MEM_OP==0x21||MEM_OP==0x25||MEM_OP==0x20||MEM_OP==0x24||MEM_OP==0x0A){
                }else stall = true;
                if(MEM_OP==0x29) stall = false;
                }else if(ID_RS!=MEM_RT){
                    //do nothing
                }
            }
    }
}


void stallDetectRt(){
    int EXn_nop = pipeline[2]&0xFC1FFFFF;
    int MEM_nop = pipeline[3];
    if(pipeline[1]==0){

     }else if(((EX_OP==0x00)&&(EX_FT==0x08))||EX_OP==0x04||EX_OP==0x05||EX_OP==0x07||EX_OP==0x02||EX_OP==0x03){
             if(MEM_nop==0){
             stall = false;
            }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT != MEM_RD){

                    }else if(ID_RT==MEM_RD && ID_RT != 0){

                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RT==MEM_RT && ID_RT != 0){

                    stall = true;
                }else if(ID_RT!=MEM_RT){
                    //do nothing
                }
            }

    }else if(EX_OP==0x00){
        if(ID_RT==EX_RD && ID_RT != 0){

        }else if(ID_RT!=EX_RD && ID_RT!= 0){
            if(MEM_nop==0){
             stall = false;
            }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT != MEM_RD){

                    }else if(ID_RT==MEM_RD){

                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RT==MEM_RT){
                if(MEM_OP==0x2B||MEM_OP==0x29||MEM_OP==0x28){

                }else stall = true;
                }else if(ID_RT!=MEM_RT){
                    //do nothing
                }
            }
        }

    }else{
        if(ID_RT==EX_RT && ID_RT!=0){
            if(EX_OP==0x23||EX_OP==0x21||EX_OP==0x25||EX_OP==0x20||EX_OP==0x24){ // load type
                 stall = true;
            }else{
                // FWD
            }
        }else if(ID_RT!=EX_RT && ID_RT!=0){
            if(MEM_nop==0){
             stall = false;
            }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT != MEM_RD){

                    }else if(ID_RT==MEM_RD){
                        stall = true;
                    }
                }
            }else{ // check RT
                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RT==MEM_RT){
                if(MEM_OP==0x2B||MEM_OP==0x29||MEM_OP==0x28){

                }else stall = true;
                }else if(ID_RT!=MEM_RT){
                    //do nothing
                }
            }
        }

    }
    if(EXn_nop==0){

        if(MEM_nop==0){

        }else if(MEM_OP==0x00){ //MEM R-type
                if(MEM_FT==0x08){

                }else{
                    if(ID_RT != MEM_RD){

                    }else if(ID_RT==MEM_RD && ID_RT!=0){

                        stall = true;
                    }
                }
            }else{ // check RT

                if(MEM_OP==0x04||MEM_OP==0x05||MEM_OP==0x07){ //branch
                    //do nothing
                }else if(ID_RT==MEM_RT && ID_RT != 0){
                if(MEM_OP==0x2B||MEM_OP==0x29||MEM_OP==0x28||MEM_OP==0x23||MEM_OP==0x21||MEM_OP==0x25||MEM_OP==0x20||MEM_OP==0x24||MEM_OP==0x0A){

                }else stall = true;
                }else if(ID_RT!=MEM_RT){
                    //do nothing
                }
            }
    }

}
