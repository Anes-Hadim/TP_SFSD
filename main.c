#include<stdbool.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>


#define MAX 100
#define LoadFact 0.75 //loading factor for bulk loading

typedef struct rec {
    char id[6];
    bool del;
    char first_name[20];
    char last_name[20];
    char birth_date[11];
    char birth_city[20];
}rec;

typedef struct Tblock { 
    rec array[MAX];
    int Nb;
    int deleted; //number of deleted records inside the block
}Tblock;

typedef struct Header {
    int BlkNb;
    int RecNb; //total undeleted records in the file
    int DelRecNb; //total deleted records in the file
}Header;

typedef struct TOF {
    Header header;
    FILE* f;
}TOF;

void open(TOF* file,char* filename,char* mode);

void close(TOF* file);

void setHeader(TOF* file,int field,int val);

int getHeader(TOF* file,int field);

void readBlock(TOF* file,int Bnb,Tblock* buffer);

void writeBlock(TOF* file,int Bnb,Tblock buffer);

void createTOF(char* filename);


void open(TOF* file,char* filename,char* mode) {
    file->f=fopen(filename,mode);
    if(file->f==NULL) {
        perror("error openning the file");
        exit(1);
    }
    int n=fread(&(file->header),sizeof(Header),1,file->f);
    if(n!=1) {
        setHeader(file,1,0);
        setHeader(file,2,0);
        setHeader(file,3,0);
    }
}

void close(TOF* file) {
    fseek(file->f,0,SEEK_SET);
    fwrite(&(file->header),sizeof(Header),1,file->f);
    fclose(file->f);
}

void setHeader(TOF* file,int field,int val) {
    if(file->f!=NULL) {
        if(field==1) {
            file->header.BlkNb=val;
        } else if(field==2) {
            file->header.RecNb=val;
        } else if(field==3) {
            file->header.DelRecNb=val;
        }
    }
}

int getHeader(TOF* file,int field) {
    if(file->f!=NULL) {
        if(field==1) {
            return file->header.BlkNb;
        } else if(field==2) {
            return file->header.RecNb;
        } else if(field==3) {
            return file->header.DelRecNb;
        }
    }
    return -1;
}

void readBlock(TOF* file,int Bnb,Tblock* buffer) {
    if(Bnb<=getHeader(file,1)) {
        fseek(file->f,(Bnb-1)*sizeof(Tblock)+sizeof(Header),SEEK_SET);
        fread(buffer,sizeof(Tblock),1,file->f);
    }
}

void writeBlock(TOF* file,int Bnb,Tblock buffer) {
    if(Bnb<=getHeader(file,1)) {
        fseek(file->f,(Bnb-1)*sizeof(Tblock)+sizeof(Header),SEEK_SET);
        fwrite(&buffer,sizeof(Tblock),1,file->f);
    }
}

int main(){
    TOF File;
    open(&File,"TOF.bin","rb+");
    Tblock buf;
    printf("block number %d\n",getHeader(&File,1));
    printf("record number %d\n",getHeader(&File,2));
    printf("deleted recs %d\n",getHeader(&File,3));
    for (int i = 1; i <= getHeader(&File,1); i++)
    {
        readBlock(&File,i,&buf);
        printf("block %d\n",i);
        printf("block nb %d\n",buf.Nb);
        for (int j = 0; j < buf.Nb; j++)
        {
            printf("the id is : %s\n",buf.array[j].id);
        }
        printf("\n\n");
    }
    
    close(&File);
    return 0;
}