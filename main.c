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
    fseek(file->f,(Bnb-1)*sizeof(Tblock)+sizeof(Header),SEEK_SET);
    fwrite(&buffer,sizeof(Tblock),1,file->f);
}


////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////

#define B 1000
#define RecSep '#'
#define FieldSep '@'


typedef struct TOVSblock { 
    char array[B];
}TOVSblock;

typedef struct TOVSHeader {
    int BlkNb;
    int RecNb; //total undeleted records in the file
    int DelRecNb; //total deleted records in the file
    int pos; //NB is max for all the block exept the last one which will be same as pos
}TOVSHeader;

typedef struct TOVS {
    TOVSHeader header;
    FILE* f;
}TOVS;

void openTOVS(TOVS* file,char* filename,char* mode);

void closeTOVS(TOVS* file);

void setHeaderTOVS(TOVS* file,int field,int val);

int getHeaderTOVS(TOVS* file,int field);

void readBlockTOVS(TOVS* file,int Bnb,TOVSblock* buffer);

void writeBlockTOVS(TOVS* file,int Bnb,TOVSblock buffer);

void openTOVS(TOVS* file,char* filename,char* mode) {
    file->f=fopen(filename,mode);
    if(file->f==NULL) {
        perror("error openning the file");
        exit(1);
    }
    int n=fread(&(file->header),sizeof(TOVSHeader),1,file->f);
    if(n!=1) {
        setHeaderTOVS(file,1,0);
        setHeaderTOVS(file,2,0);
        setHeaderTOVS(file,3,0);
        setHeaderTOVS(file,4,0);
    }
}

void closeTOVS(TOVS* file) {
    fseek(file->f,0,SEEK_SET);
    fwrite(&(file->header),sizeof(TOVSHeader),1,file->f);
    fclose(file->f);
}

void setHeaderTOVS(TOVS* file,int field,int val) {
    if(file->f!=NULL) {
        switch (field)
        {
        case 1:
            file->header.BlkNb=val;
            break;
        case 2:
            file->header.RecNb=val;
            break;
        case 3:
            file->header.DelRecNb=val;
            break;
        case 4:
            file->header.pos=val;
            break;
        
        default:
            printf("Error field does not exist\n");
            break;
        }
    }
}

int getHeaderTOVS(TOVS* file,int field) {
    if(file->f!=NULL) {
        switch (field)
        {
        case 1:
            return file->header.BlkNb;
            break;
        case 2:
            return file->header.RecNb;
            break;
        case 3:
            return file->header.DelRecNb;
            break;
        case 4:
            return file->header.pos;
            break;
        
        default:
            printf("Error field does not exist\n");
            return -1;
            break;
        }
    }
    return -1;
}

void readBlockTOVS(TOVS* file,int Bnb,TOVSblock* buffer) {
    if(Bnb<=getHeaderTOVS(file,1)) {
        fseek(file->f,(Bnb-1)*sizeof(TOVSblock)+sizeof(TOVSHeader),SEEK_SET);
        fread(buffer,sizeof(TOVSblock),1,file->f);
    }
}

void writeBlockTOVS(TOVS* file,int Bnb,TOVSblock buffer) {
    fseek(file->f,(Bnb-1)*sizeof(TOVSblock)+sizeof(TOVSHeader),SEEK_SET);
    fwrite(&buffer,sizeof(TOVSblock),1,file->f);
}

int main(){
    TOF File;
    open(&File,"TOF.bin","rb+");
    Tblock buf1;
    printf("block number %d\n",getHeader(&File,1));
    printf("record number %d\n",getHeader(&File,2));
    printf("deleted recs %d\n",getHeader(&File,3));
    for (int i = 1; i <= getHeader(&File,1); i++)
    {
        readBlock(&File,i,&buf1);
        printf("block %d\n",i);
        for (int j = 0; j < buf1.Nb; j++)
        {
            printf("%s %d %s %s %s %s\n",buf1.array[j].id,buf1.array[j].del,buf1.array[j].first_name,buf1.array[j].last_name,buf1.array[j].birth_date,buf1.array[j].birth_city);
        }
        printf("\n\n");
    }
    close(&File);
    TOVS file;
    openTOVS(&file,"TOVS.bin","rb+");
    TOVSblock buf;
    printf("block number %d\n",getHeaderTOVS(&file,1));
    printf("record number %d\n",getHeaderTOVS(&file,2));
    printf("deleted number %d\n",getHeaderTOVS(&file,3));
    printf("pos %d\n",getHeaderTOVS(&file,4));
    for (int i = 1; i <getHeaderTOVS(&file,1); i++)
    {
        readBlockTOVS(&file,i,&buf);
        printf("\nblock %d\n",i);
        for (int j = 0; j < B; j++)
        {
            if (buf.array[j]==RecSep) {
                printf("\n");
            } else if (buf.array[j]==FieldSep) {
                printf(" ");
            } else {
                printf("%c",buf.array[j]);
            }
        }
    }
    readBlockTOVS(&file,getHeaderTOVS(&file,1),&buf);
    printf("\nblock %d\n",getHeaderTOVS(&file,1));
    for (int j = 0; j < getHeaderTOVS(&file,4); j++)
    {
        if (buf.array[j]==RecSep) {
            printf("\n");
        } else if (buf.array[j]==FieldSep) {
            printf(" ");
        } else {
            printf("%c",buf.array[j]);
        }
    }
    closeTOVS(&file);
    return 0;
}