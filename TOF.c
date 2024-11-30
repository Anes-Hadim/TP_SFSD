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

void createTOF(char* filename) {
    TOF file;
    open(&file,filename,"wb+");
    setHeader(&file,1,0);
    setHeader(&file,2,0);
    setHeader(&file,3,0);
    close(&file);
}

void binary_search(char* filename,char* key,bool* found,int* blk,int* pos) {
    TOF file;
    Tblock buf;
    open(&file,filename,"rb+");
    int lo=1;
    int up=getHeader(&file,1);
    *found=false;
    *pos=0;
    *blk=0;
    bool stop=false;
    while(lo<=up && !(*found) && !stop) {
        *blk=(lo+up)/2;
        readBlock(&file,*blk,&buf);
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
        } else if(buf.Nb<MAX*LoadFact){
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
    if(!(*found)) {
        *blk=lo;
    }
    close(&file);
}

void insert(rec r,char* filename) {
    bool found;
    int blk,pos;
    binary_search(filename,r.id,&found,&blk,&pos);
    printf("the block %d \n",blk);
    // printf("the position %d\n",pos);
    if(!found) {
        TOF file;
        Tblock buf;
        open(&file,filename,"rb+");
        if(getHeader(&file,1)==0) {
            buf.array[0]=r;
            buf.Nb=1;
            buf.deleted=0;
            setHeader(&file,1,1);
            writeBlock(&file,1,buf);
        } else {
            bool continu=true;
            while(continu && blk<=getHeader(&file,1)) {
                readBlock(&file,blk,&buf);
                rec x=buf.array[buf.Nb-1];
                int k=buf.Nb-1;
                while(k>pos) {
                    buf.array[k]=buf.array[k-1];
                    k--;
                }
                buf.array[pos]=r;
                if(buf.Nb<MAX*LoadFact) {
                    buf.Nb++;
                    buf.array[buf.Nb-1]=x;
                    writeBlock(&file,blk,buf);
                    continu=false;
                } else {
                    writeBlock(&file,blk,buf);
                    blk++;
                    pos=0;
                    r=x;
                }
            }
            if(blk>getHeader(&file,1)) { //same for if(continu)
                buf.array[0]=r;
                buf.Nb=1;
                buf.deleted=0;
                writeBlock(&file,blk,buf);
                setHeader(&file,1,blk);
            }
        }
        setHeader(&file,2,getHeader(&file,2)+1);
        close(&file);
    }
}

void charging_TOF(){
    FILE* F;
    TOF File;
    Tblock buf;
    int count=0;
    F = fopen("TOF.txt","r");
    if (F==NULL)
    {
        perror("opening file");
        exit(1);
    }
    char string[100];
    rec r;
    fgets(string,100,F);
    while ((fgets(string,100,F)))
    {
        int cpt=1;
        int index=0;
        int i=0;
        while(string[index]!='\0') {
            if (string[index]=='\t') {
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
                case 5:
                    r.birth_city[i]='\0';
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
        r.birth_city[i]='\0';
        r.del=false;
        count++;
        printf("count is %d\n",count);
        insert(r,"TOF.bin");
        // open(&File,"TOF.bin","rb+");
        // for (int i = 1; i <= getHeader(&File,1); i++)
        // {
        //     readBlock(&File,i,&buf);
        //     printf("block %d\n",i);
        //     for (int j = 0; j < buf.Nb; j++)
        //     {
        //         printf("the id is : %s\n",buf.array[j].id);
        //     }
        //     printf("\n\n");
        // }
        // close(&File);
        if (count >100)
        {
            break;
        }
        // TOF file;
        // open(&file,"TOF.bin","rb+");
        // printf("%d\n",getHeader(&file,1));
        // close(&file);
    }
    fclose(F);
}

bool search(int key ,rec* r){
    FILE* F;
    F = fopen("TOF.txt","r");
    if (F==NULL)
    {
        perror("opening file");
        exit(1);
    }
    char string[100];
    bool stop;
    fgets(&string,100,F);
    while (fgets(&string,100,F) && !stop)
    {
        int cpt=1;
        int index=0;
        int i=0;
        while(string[index]!='\0') {
            if (string[index]=='\t') {
                switch (cpt)
                {
                case 1:
                    r->id[i]='\0';
                    break;
                case 2:
                    r->first_name[i]='\0';
                    break;
                case 3:
                    r->last_name[i]='\0';
                    break;
                case 4:
                    r->birth_date[i]='\0';
                    break;
                case 5:
                    r->birth_city[i]='\0';
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
                    r->first_name[i]=string[index];
                    break;
                case 3:
                    r->last_name[i]=string[index];
                    break;
                case 4:
                    r->birth_date[i]=string[index];
                    break;
                case 5:
                    r->birth_city[i]=string[index];
                    break;
                }
                i++;
            }
            index++;
        }
        r->birth_city[i]='\0';
        if (atoi(r->id)==key)
        {
            fclose(F);
            return true;
        }
    }
    fclose(F);
    return false;
}

void bulk_loading(char* filename) {
    TOF file;
    Tblock buf;
    FILE* F;
    int key=10000;
    open(&file,filename,"rb+");
    int blk=1,pos=0;
    int nrec=0;
    rec r;
    while(key<20000) {
            if (search(key,&r))
            {
                r.del=false;
                if(pos<LoadFact*MAX) {
                    buf.array[pos] = r;
                    nrec++;
                    pos++;
                } else {
                    buf.deleted=0;
                    buf.Nb=pos;
                    writeBlock(&file,blk,buf);
                    blk++;
                    buf.array[0]=r;
                    nrec++;
                    pos=1;
                }
            }
            key++;
    }
    buf.Nb=pos;
    buf.deleted=0;
    writeBlock(&file,blk,buf);
    setHeader(&file,1,blk);
    setHeader(&file,2,nrec);
    setHeader(&file,3,0);
    close(&file);
}

void delete(rec r,char* filename) {
    bool found;
    int blk,pos;
    binary_search(filename,r.id,&found,&blk,&pos);
    if(found) {
        TOF file;
        Tblock buf;
        open(&file,filename,"rb+");
        readBlock(&file,blk,&buf);
        buf.array[pos].del=true;
        buf.deleted++;
        writeBlock(&file,blk,buf);
        if(blk==1 && buf.Nb-buf.deleted==0) {
            setHeader(&file,1,0);
        }
        setHeader(&file,3,getHeader(&file,3)+1);
        setHeader(&file,2,getHeader(&file,2)-1);
        close(&file);
    }
}

int main(){
    createTOF("TOF.bin");
    charging_TOF();
    return 0;
}