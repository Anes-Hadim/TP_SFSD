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

typedef struct TOFblock { 
    rec array[MAX];
    int Nb;
    int deleted; //number of deleted records inside the block
}TOFblock;

typedef struct TOFHeader {
    int BlkNb;
    int RecNb; //total undeleted records in the file
    int DelRecNb; //total deleted records in the file
}TOFHeader;

typedef struct index_array{
    char id[6];
    int pos;
    int blk;
}index_array;

typedef struct indexTOF{
    int size;
    index_array array[200];
}indexTOF;

typedef struct TOF {
    TOFHeader header;
    FILE* f;
    indexTOF index;
}TOF;

void openTOF(TOF* file,char* filename,char* mode);

void closeTOF(TOF* file);

void setHeaderTOF(TOF* file,int field,int val);

int getHeaderTOF(TOF* file,int field);

void readBlockTOF(TOF* file,int Bnb,TOFblock* buffer);

void writeBlockTOF(TOF* file,int Bnb,TOFblock buffer);

void createTOF(char* filename);


void openTOF(TOF* file,char* filename,char* mode) {
    file->f=fopen(filename,mode);
    if(file->f==NULL) {
        perror("error openning the file");
        exit(1);
    }
    int n=fread(&(file->header),sizeof(TOFHeader),1,file->f);
    if(n!=1) {
        setHeaderTOF(file,1,0);
        setHeaderTOF(file,2,0);
        setHeaderTOF(file,3,0);
    }
}

void closeTOF(TOF* file) {
    fseek(file->f,0,SEEK_SET);
    fwrite(&(file->header),sizeof(TOFHeader),1,file->f);
    fclose(file->f);
}

void setHeaderTOF(TOF* file,int field,int val) {
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

int getHeaderTOF(TOF* file,int field) {
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

void readBlockTOF(TOF* file,int Bnb,TOFblock* buffer) {
    if(Bnb<=getHeaderTOF(file,1)) {
        fseek(file->f,(Bnb-1)*sizeof(TOFblock)+sizeof(TOFHeader),SEEK_SET);
        fread(buffer,sizeof(TOFblock),1,file->f);
    }
}

void writeBlockTOF(TOF* file,int Bnb,TOFblock buffer) {
    fseek(file->f,(Bnb-1)*sizeof(TOFblock)+sizeof(TOFHeader),SEEK_SET);
    fwrite(&buffer,sizeof(TOFblock),1,file->f);
}

void createTOF(char* filename) {
    TOF file;
    openTOF(&file,filename,"wb+");
    setHeaderTOF(&file,1,0);
    setHeaderTOF(&file,2,0);
    setHeaderTOF(&file,3,0);
    closeTOF(&file);
}

void binary_search(char* filename,char* key,bool* found,int* blk,int* pos) {
    TOF file;
    TOFblock buf;
    openTOF(&file,filename,"rb+");
    int lo=1;
    int up=getHeaderTOF(&file,1);
    *found=false;
    *pos=0;
    *blk=1;
    bool stop=false;
    while(lo<=up && !(*found) && !stop) {
        *blk=(lo+up)/2;
        readBlockTOF(&file,*blk,&buf);
        if(strcmp(key,buf.array[0].id)>=0 && strcmp(key,buf.array[buf.Nb-1].id)<=0) {
            int inf=0, sup=buf.Nb-1;
            while(inf<=sup && !(*found)) {
                *pos=(inf+sup)/2;
                if(!buf.array[*pos].del && strcmp(key,buf.array[*pos].id)==0) {
                    *found=true;
                } else if (strcmp(key,buf.array[*pos].id)<0) {
                    sup=*pos-1;
                } else {
                    inf=*pos+1;
                }
            }
            if(!(*found)) {
                *pos=inf;
                stop=true; //this is the case where the block is found but the rec is not 
            }
        } else if(buf.Nb<MAX*LoadFact  && strcmp(key,buf.array[*pos].id)>0){
            *pos=buf.Nb;
            lo=*blk;
            stop=true;
        } else {
            if(strcmp(key,buf.array[0].id)<0) {
                up=*blk-1;
            } else {
                lo=*blk+1;
            }
        }
    }
    if(!(*found) && !stop) {
        *blk=lo;
    }
    closeTOF(&file);
}

void insertTOF(rec r,char* filename) {
    bool found;
    int blk,pos;
    binary_search(filename,r.id,&found,&blk,&pos);
    if(!found) {
        TOF file;
        TOFblock buf;
        openTOF(&file,filename,"rb+");
        if(getHeaderTOF(&file,1)==0) {
            buf.array[0]=r;
            buf.Nb=1;
            buf.deleted=0;
            setHeaderTOF(&file,1,1);
            writeBlockTOF(&file,1,buf);
        } else {
            bool continu=true;
            rec x;
            readBlockTOF(&file,blk,&buf);
            if(pos==buf.Nb) {
                buf.array[buf.Nb]=r;
                buf.Nb++;
                writeBlockTOF(&file,blk,buf);
                continu=false;
            }
            while(continu && blk<=getHeaderTOF(&file,1)) {
                x=buf.array[buf.Nb-1];
                int k=buf.Nb-1;
                while(k>pos) {
                    buf.array[k]=buf.array[k-1];
                    k--;
                }
                buf.array[pos]=r;
                if(buf.Nb<MAX*LoadFact) {
                    buf.Nb++;
                    buf.array[buf.Nb-1]=x;
                    writeBlockTOF(&file,blk,buf);
                    continu=false;
                } else {
                    writeBlockTOF(&file,blk,buf);
                    blk++;
                    pos=0;
                    r=x;
                }
                if(continu && blk<=getHeaderTOF(&file,1)) {
                    readBlockTOF(&file,blk,&buf);
                }
            }
            if(blk>getHeaderTOF(&file,1)) { //same for if(continu)
                buf.array[0]=r;
                buf.Nb=1;
                buf.deleted=0;
                writeBlockTOF(&file,blk,buf);
                setHeaderTOF(&file,1,blk);
            }
        }
        setHeaderTOF(&file,2,getHeaderTOF(&file,2)+1);
        closeTOF(&file);
    }
}

void loading_TOF(){
    FILE* F;
    F = fopen("students_data_1a.csv","r");
    if (F==NULL)
    {
        perror("opening file");
        exit(1);
    }
    char string[100];
    rec r;
    fgets(string,sizeof(string),F);
    while ((fgets(string,sizeof(string),F)))
    {
        int cpt=1;
        int index=0;
        int i=0;
        while(string[index]!='\n') {
            if (string[index]==',') {
                switch (cpt)
                {
                case 1:
                    r.id[i]='\0';
                    break;
                case 2:
                    r.first_name[i]='\0';
                    break;
                case 3:
                    r.last_name[i]='\0';
                    break;
                case 4:
                    r.birth_date[i]='\0';
                    break;
                }
                i=0;
                cpt++;
            } else {
                switch (cpt)
                {
                case 1:
                    r.id[i]=string[index];
                    break;
                case 2:
                    r.first_name[i]=string[index];
                    break;
                case 3:
                    r.last_name[i]=string[index];
                    break;
                case 4:
                    r.birth_date[i]=string[index];
                    break;
                case 5:
                    r.birth_city[i]=string[index];
                    break;
                }
                i++;
            }
            index++;
        }
        r.birth_city[i-1]='\0';
        r.del=false;
        insertTOF(r,"TOF.bin");
    }
    fclose(F);
}

void deleteTOF(rec r,char* filename) {
    bool found;
    int blk,pos;
    binary_search(filename,r.id,&found,&blk,&pos);
    if(found) {
        TOF file;
        TOFblock buf;
        openTOF(&file,filename,"rb+");
        readBlockTOF(&file,blk,&buf);
        buf.array[pos].del=true;
        buf.deleted++;
        writeBlockTOF(&file,blk,buf);
        if(blk==1 && buf.Nb-buf.deleted==0) {
            setHeaderTOF(&file,1,0);
        }
        setHeaderTOF(&file,3,getHeaderTOF(&file,3)+1);
        setHeaderTOF(&file,2,getHeaderTOF(&file,2)-1);
        closeTOF(&file);
    }
}

void loading_index(){          // sparse index
    TOF file;
    openTOF(&file,"TOF.bin","r");
    int Nblk=getHeaderTOF(&file,1);
    int i=1;
    TOFblock buffer;
    while (i<=Nblk)
    {
        readBlockTOF(&file,i,&buffer);
        strcpy(file.index.array[i].id,buffer.array[buffer.Nb].id);
        file.index.array[i].blk=i;
        file.index.array[i].pos=buffer.Nb-1;
        i++;
    }
    file.index.size=Nblk;
    closeTOF(&file);
}


//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

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

void createTOVS(char *filename) {
    TOVS file;
    openTOVS(&file,filename,"wb+");
    setHeaderTOVS(&file,1,0);
    setHeaderTOVS(&file,2,0);
    setHeaderTOVS(&file,3,0);
    setHeaderTOVS(&file,4,0);
    closeTOVS(&file);
}

void extract_string(TOVS* file,TOVSblock* buf,int* blk,int* pos,char* val,char* del) {
    int k=0;
    do {
        val[k]=buf->array[*pos];
        (*pos)++;
        k++;
        if(*pos>=MAX) {
            (*blk)++;
            readBlockTOVS(file,*blk,buf);
            *pos=0;
        }
    } while(buf->array[*pos]!=FieldSep);
    val[k]='\0';
    (*pos)++;
    if(*pos>=MAX) {
        (*blk)++;
        readBlockTOVS(file,*blk,buf);
        *pos=0;
    }
    *del = buf->array[*pos];
}

void searchTOVS(char* filename,char* key,bool* found,int* blk,int* pos) {
    *found=false;
    *pos=0;
    *blk=1;
    int prevPos=0;
    int prevBlk=1;
    TOVS file;
    openTOVS(&file,filename,"rb+");
    TOVSblock buf;
    char val[15];
    char del;
    bool stop=false;
    readBlockTOVS(&file,*blk,&buf);
    int count=0;
    int nbrec=getHeaderTOVS(&file,2)+getHeaderTOVS(&file,3);
    while(count<nbrec && !(*found) && !stop) {
        if(count!=0 && strcmp(val,key)>0) {
            stop=true; //here pos is the new rec of the second biggest rec to key
            *pos=prevPos; //so pos will point at the begining or the first largest res
            *blk=prevBlk; //it is exactly where we should begin the insertion
        } else {
            prevPos=*pos;
            prevBlk=*blk;
            extract_string(&file,&buf,blk,pos,val,&del);
            if(strcmp(val,key)==0 && del=='f') {
                *found=true; //will stop pos at the del field
            } else {
                do {
                    (*pos)++;
                    if(*pos>=MAX) {
                        (*blk)++;
                        readBlockTOVS(&file,*blk,&buf);
                        *pos=0;
                    }
                }while(buf.array[*pos]!=RecSep);
                (*pos)++;
                if(*pos>=MAX) {
                    (*blk)++;
                    readBlockTOVS(&file,*blk,&buf);
                    *pos=0;
                }
            } //will stop pos at the first char of the new rec
        }
        count++; //because of count we will never need to test if we are in the last block and pos > NB
    }
    closeTOVS(&file);
}

void insertTOVS(char* filename,char* rec) {
    bool found;
    int blk,pos;
    char key[15];
    int i=0;
    do {
        key[i]=rec[i];
        i++;
    } while(rec[i]!=FieldSep);
    key[i]='\0';
    searchTOVS(filename,key,&found,&blk,&pos);
    if(!found) {
        TOVS file;
        TOVSblock buf;
        openTOVS(&file,filename,"rb+");
        char temp;
        i=0;
        readBlockTOVS(&file,blk,&buf);
        int endBlk=getHeaderTOVS(&file,1);
        int endPos=getHeaderTOVS(&file,4);
        while(endBlk!=blk || endPos!=pos) {
            temp=buf.array[pos];
            buf.array[pos]=rec[i];
            rec[i]=temp;
            i++;
            pos++;
            if(pos>=MAX) {
                pos=0;
                writeBlockTOVS(&file,blk,buf);
                blk++;
                readBlockTOVS(&file,blk,&buf);
            }
            if(i>=strlen(rec)) {
                i=0;
            }
        }
        while(i<strlen(rec)) {
            if(pos>=MAX) {
                pos=0;
                writeBlockTOVS(&file,blk,buf);
                blk++;
                setHeaderTOVS(&file,1,blk);
            }
            buf.array[pos]=rec[i];
            pos++;
            i++;
        }
        writeBlockTOVS(&file,blk,buf);
        setHeaderTOVS(&file,2,getHeaderTOVS(&file,2)+1);
        setHeaderTOVS(&file,4,pos);
        closeTOVS(&file);
    }
}

void deleteTOVS(char* filename,char* key) {
    bool found;
    int blk,pos;
    searchTOVS(filename,key,&found,&blk,&pos);
    if(found) {
        TOVS file;
        TOVSblock buf;
        openTOVS(&file,filename,"rb+");
        readBlockTOVS(&file,blk,&buf);
        buf.array[pos]='v';
        setHeaderTOVS(&file,2,getHeaderTOVS(&file,2)-1);
        setHeaderTOVS(&file,3,getHeaderTOVS(&file,3)+1);
        writeBlockTOVS(&file,blk,buf);
        closeTOVS(&file);
    }
}

typedef struct tovs_info{
        char id[6];
        char year[4];
        char info[150];
}tovs_info;

bool search(char* key ,tovs_info* r){
    FILE* F;
    F = fopen("students_data_2a.csv","r");
    if (F==NULL)
    {
        perror("opening file");
        exit(1);
    }
    char string[200];
    int count=1;
    fgets(string,sizeof(string),F);
    while (fgets(string,sizeof(string),F))
    {
        int cpt=1;
        int index=0;
        int i=0;
        while(string[index]!='\n') {
            if (string[index]==',') {
                count++;
            }
            if (string[index]==',' && count<3) {
                switch (cpt)
                {
                case 1:
                    r->id[i]='\0';
                    break;
                case 2:
                    r->year[i]='\0';
                    break;
                case 3:
                    r->info[i]='\0';
                    break;
                }
                i=0;
                cpt++;
            } else {
                switch (cpt)
                {
                case 1:
                    r->id[i]=string[index];
                    break;
                case 2:
                    r->year[i]=string[index];
                    break;
                case 3:
                    r->info[i]=string[index];
                    break;
                }
                i++;
            }
            index++;
        }
        r->info[i]='\0';
        if (strcmp(key,r->id)==0)
        {
            fclose(F);
            return true;
        }
        count=0;
    }
    fclose(F);
    return false;
}


void next_block(TOVS tovs_f,int *bnb,int *index,TOVSblock buffer2){
    if (*index==B)
    {
        (*bnb)++;
        writeBlockTOVS(&tovs_f,*bnb,buffer2);
        *index=0;
    }
    else
    {
        return;
    }
}


void loading_TOVS(){
    TOF tof_f;
    TOVS tovs_f;
    openTOF(&tof_f,"TOF.bin","rb+");
    openTOVS(&tovs_f,"TOVS.bin","rb+");
    int Nblk=getHeaderTOF(&tof_f,1),i=1;
    TOFblock buffer1;
    TOVSblock buffer2;
    int cpt=1;
    int k=0;
    int index=0;
    int bnb=0;
    int recnb=0;
    tovs_info info;
    while (i<=Nblk)
    {
        readBlockTOF(&tof_f,i,&buffer1);
        for (int j = 0; j < buffer1.Nb; j++)
        {
            cpt=1;
            while (cpt<=5)
            {
                switch (cpt)
                {
                case 1:
                    while (buffer1.array[j].id[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].id[k];
                        k++;
                        // printf("%c",buffer2.array[index]);
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                    buffer2.array[index]='f';
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                    buffer2.array[index]=FieldSep;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                    // printf("\n");
                    cpt++;
                    break;
                case 2:
                    while (buffer1.array[j].first_name[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].first_name[k];
                        // printf("%c",buffer2.array[index]);
                        k++;
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
                    // printf("\n");
                    next_block(tovs_f,&bnb,&index,buffer2);
                    cpt++;
                    break;
                case 3:
                    while (buffer1.array[j].last_name[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].last_name[k];
                        k++;
                        // printf("%c",buffer2.array[index]);
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
                    // printf("\n");
                    next_block(tovs_f,&bnb,&index,buffer2);
                    cpt++;
                    break;
                case 4:
                    while (buffer1.array[j].birth_date[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].birth_date[k];
                        k++;
                        // printf("%c",buffer2.array[index]);
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
                    // printf("\n");
                    next_block(tovs_f,&bnb,&index,buffer2);
                    cpt++;
                    break;
                case 5:
                    while (buffer1.array[j].birth_city[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].birth_city[k];
                        k++;
                        // printf("%c",buffer2.array[index]);
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
                    // printf("\n");
                    next_block(tovs_f,&bnb,&index,buffer2);
                    cpt++;
                    break;
                default:
                    break;
                }
            }
            if (search(buffer1.array[j].id,&info))
                {
                    cpt=1;
                    k=0;
                    while (cpt<=2)
                    {
                        switch (cpt)
                        {
                        case 1:
                            while (info.year[k]!='\0')
                            {
                                buffer2.array[index]=info.year[k];
                                k++;
                                // printf("%c",buffer2.array[index]);
                                index++;
                                next_block(tovs_f,&bnb,&index,buffer2);
                            }
                            k=0;
                            buffer2.array[index]=FieldSep;
                            index++;
                            // printf("\n");
                            next_block(tovs_f,&bnb,&index,buffer2);
                            cpt++;
                            break;
                        case 2:
                        while (info.info[k]!='\0')
                            {
                                buffer2.array[index]=info.info[k];
                                k++;
                                // printf("%c",buffer2.array[index]);
                                index++;
                                next_block(tovs_f,&bnb,&index,buffer2);
                            }
                            k=0;
                            cpt++;
                            // printf("\n");
                            break;
                        default:
                            break;
                        }
                    }
                    buffer2.array[index]=RecSep;
                    recnb++;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                } else {
                    printf("it happened\n");
                    buffer2.array[index] = FieldSep;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                    buffer2.array[index] = FieldSep;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                    buffer2.array[index] = RecSep;
                    recnb++;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                }
        }
        i++;
    }
    if (index!=0) {
        writeBlockTOVS(&tovs_f,bnb,buffer2);
    }
    setHeaderTOVS(&tovs_f,1,bnb);
    setHeaderTOVS(&tovs_f,2,recnb);
    setHeaderTOVS(&tovs_f,4,index);
    closeTOF(&tof_f);
    closeTOVS(&tovs_f);
}


int main(){
    createTOF("TOF.bin");
    loading_TOF();
    createTOVS("TOVS.bin");
    loading_TOVS();
    TOVS tovs_f;
    openTOVS(&tovs_f,"TOVS.bin","rb+");
    printf("number of blocks is :%d\n",getHeaderTOVS(&tovs_f,1));
    printf("number of records is :%d\n",getHeaderTOVS(&tovs_f,2));
    closeTOVS(&tovs_f);
    return 0;
}