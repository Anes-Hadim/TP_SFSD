#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define B 1000
#define RecSep '#'
#define FieldSep '@'
#define TuppleSep '?'

typedef struct TOVSblock
{
    char array[B];
} TOVSblock;

typedef struct TOVSHeader
{
    int BlkNb;
    int RecNb;    // total undeleted records in the file
    int DelRecNb; // total deleted records in the file
    int pos;      // NB is max for all the block exept the last one which will be same as pos
} TOVSHeader;

typedef struct TOVS
{
    TOVSHeader header;
    FILE *f;
} TOVS;

TOVS globalTOVS;

void openTOVS(TOVS *file, char *filename, char *mode);

void closeTOVS(TOVS *file);

void setHeaderTOVS(TOVS *file, int field, int val);

int getHeaderTOVS(TOVS *file, int field);

void readBlockTOVS(TOVS *file, int Bnb, TOVSblock *buffer);

void writeBlockTOVS(TOVS *file, int Bnb, TOVSblock buffer);

void openTOVS(TOVS *file, char *filename, char *mode)
{
    file->f = fopen(filename, mode);
    if (file->f == NULL)
    {
        perror("error openning the file");
        exit(1);
    }
    int n = fread(&(file->header), sizeof(TOVSHeader), 1, file->f);
    if (n != 1)
    {
        setHeaderTOVS(file, 1, 0);
        setHeaderTOVS(file, 2, 0);
        setHeaderTOVS(file, 3, 0);
        setHeaderTOVS(file, 4, 0);
    }
}

void closeTOVS(TOVS *file)
{
    fseek(file->f, 0, SEEK_SET);
    fwrite(&(file->header), sizeof(TOVSHeader), 1, file->f);
    fclose(file->f);
}

void setHeaderTOVS(TOVS *file, int field, int val)
{
    if (file->f != NULL)
    {
        switch (field)
        {
        case 1:
            file->header.BlkNb = val;
            break;
        case 2:
            file->header.RecNb = val;
            break;
        case 3:
            file->header.DelRecNb = val;
            break;
        case 4:
            file->header.pos = val;
            break;

        default:
            printf("Error field does not exist\n");
            break;
        }
    }
}

int getHeaderTOVS(TOVS *file, int field)
{
    if (file->f != NULL)
    {
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

void readBlockTOVS(TOVS *file, int Bnb, TOVSblock *buffer)
{
    if (Bnb <= getHeaderTOVS(file, 1))
    {
        fseek(file->f, (Bnb - 1) * sizeof(TOVSblock) + sizeof(TOVSHeader), SEEK_SET);
        fread(buffer, sizeof(TOVSblock), 1, file->f);
    }
}

void writeBlockTOVS(TOVS *file, int Bnb, TOVSblock buffer)
{
    fseek(file->f, (Bnb - 1) * sizeof(TOVSblock) + sizeof(TOVSHeader), SEEK_SET);
    fwrite(&buffer, sizeof(TOVSblock), 1, file->f);
}

void createTOVS(char *filename)
{
    TOVS file;
    openTOVS(&file, filename, "wb+");
    setHeaderTOVS(&file, 1, 0);
    setHeaderTOVS(&file, 2, 0);
    setHeaderTOVS(&file, 3, 0);
    setHeaderTOVS(&file, 4, 0);
    closeTOVS(&file);
}

void extract_string(TOVS *file, TOVSblock *buf, int *blk, int *pos, char *val, char *del)
{
    int k = 0;
    do
    {
        val[k] = buf->array[*pos];
        (*pos)++;
        k++;
        if (*pos >= B)
        {
            (*blk)++;
            readBlockTOVS(file, *blk, buf);
            *pos = 0;
        }
    } while (buf->array[*pos] != FieldSep);
    val[k] = '\0';
    (*pos)++;
    if (*pos >= B)
    {
        (*blk)++;
        readBlockTOVS(file, *blk, buf);
        *pos = 0;
    }
    *del = buf->array[*pos];
}

void searchTOVS(char *filename, char *key, bool *found, int *blk, int *pos)
{
    *found = false;
    *pos = 0;
    *blk = 1;
    int prevPos = 0;
    int prevBlk = 1;
    TOVS file;
    openTOVS(&file, filename, "rb+");
    TOVSblock buf;
    if (getHeaderTOVS(&file,1)==0) {
        *blk=1;
        *pos=0;
        *found=false;
    } else {
        char val[6];
        char del;
        bool stop = false;
        readBlockTOVS(&file, *blk, &buf);
        int count = 0;                                                 // number of records processed
        int nbrec = getHeaderTOVS(&file, 2) + getHeaderTOVS(&file, 3); // total number of records (deleted and !deleted)
        while (count <= nbrec && !(*found) && !stop)
        {
            if (count==nbrec) {
                if (count != 0 && strcmp(val, key) > 0)
                {
                    *pos = prevPos;
                    *blk = prevBlk;
                }
                stop=true;
            } else if (count != 0 && strcmp(val, key) > 0)
            {
                stop = true;    // here pos is the new rec of the second biggest rec to key
                *pos = prevPos; // so pos will point at the begining or the first largest res
                *blk = prevBlk; // it is exactly where we should begin the insertion
            }
            else
            {
                prevPos = *pos;
                prevBlk = *blk;
                extract_string(&file, &buf, blk, pos, val, &del);
                if (strcmp(val, key) == 0 && del == 'f')
                {
                    *found = true; // will stop pos at the del field
                }
                else
                {
                    do
                    {
                        (*pos)++;
                        if (*pos >= B)
                        {
                            (*blk)++;
                            readBlockTOVS(&file, *blk, &buf);
                            *pos = 0;
                        }
                    } while (buf.array[*pos] != RecSep);
                    (*pos)++;
                    if (*pos >= B)
                    {
                        (*blk)++;
                        readBlockTOVS(&file, *blk, &buf);
                        *pos = 0;
                    }
                } // will stop pos at the first char of the new rec
            }
            count++; // because of count we will never need to test if we are in the last block and pos > NB
        }
    }
    closeTOVS(&file);
}

void insertTOVS(char *filename, char rec[200])
{
    //rec will not be empty
    bool found;
    int blk, pos;
    char key[6];
    int i = 0;
    do
    {
        key[i] = rec[i];
        i++;
    } while (rec[i] != FieldSep);
    key[i] = '\0';
    searchTOVS(filename, key, &found, &blk, &pos);
    if (!found)
    {
        TOVS file;
        TOVSblock buf;
        openTOVS(&file, filename, "rb+");
        char temp;
        i = 0;
        int endBlk = getHeaderTOVS(&file, 1);
        int endPos = getHeaderTOVS(&file, 4);
        if (endBlk==0) {
            blk=1;
            pos=0;
            i=0;
            while(rec[i]!='\0') {
                buf.array[pos]=rec[i];
                i++;
                pos++;
                if(pos==B) {
                    pos=0;
                    writeBlockTOVS(&file,blk,buf);
                    blk++;
                }
            }
            writeBlockTOVS(&file,blk,buf);
            setHeaderTOVS(&file,1,blk);
            setHeaderTOVS(&file, 2,1);
            setHeaderTOVS(&file, 4, pos);
        } else {
            readBlockTOVS(&file, blk, &buf);
            while (endBlk != blk || endPos != pos)
            {
                temp = buf.array[pos];
                buf.array[pos] = rec[i];
                rec[i] = temp;
                i++;
                pos++;
                if (pos >= B)
                {
                    pos = 0;
                    writeBlockTOVS(&file, blk, buf);
                    blk++;
                    readBlockTOVS(&file, blk, &buf);
                }
                if(rec[i]=='\0') {
                    i=0;
                }
            }
            int size=strlen(rec);
            int count=0;
            while (count<size)
            {
                buf.array[pos] = rec[i];
                pos++;
                i++;
                if (i==size) {
                    i=0;
                }
                if (pos >= B)
                {
                    pos = 0;
                    writeBlockTOVS(&file, blk, buf);
                    blk++;
                }
                count++;
            }
            writeBlockTOVS(&file,blk,buf);
            setHeaderTOVS(&file,1,blk);
            setHeaderTOVS(&file, 2, getHeaderTOVS(&file, 2) + 1);
            setHeaderTOVS(&file, 4, pos);
        }
        closeTOVS(&file);
    }else{
        // printf("insertion failed for the id : %s already exit\n",key);
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void treat_str(char* str ,char* year) {
    int cpt=0;
    int i=0;
    char del;
    year[0]='\0';
    while(i<strlen(str)) {
        if(str[i]==FieldSep) {
            cpt++;
        }
        if(cpt==1) {
            i++;
            del=str[i];
        }
        if(cpt==6 && del=='f') {
            if (str[i+1]!=FieldSep) {
                year[0]=str[i+1];
                year[1]=str[i+2];
                year[2]=str[i+3];
                year[3]='\0';
            }
            break;
        }
        i++;
    }
}

void create_str(char* str,int blk,int pos) {
    int j=strlen(str);
    char nblk[10];
    char nbpos[10];
    sprintf(nblk,"%d",blk);
    sprintf(nbpos,"%d",pos);

    str[j]=FieldSep;
    j++;
    for(int i=0;i<strlen(nblk);i++) {
        str[j]=nblk[i];
        j++;
    }
    
    str[j]=TuppleSep;
    j++;
    for(int i=0;i<strlen(nbpos);i++) {
        str[j]=nblk[i];
        j++;
    }
    str[j]=FieldSep;
    j++;
    str[j]='\0';
}

void insert_index(char* str) {
    TOVS index;
    TOVSblock buf;
    openTOVS(&index,"indexYear.bin","wb+");
    int j=getHeaderTOVS(&index,4);
    int i = getHeaderTOVS(&index,1);
    for(int k=0;k<strlen(str);k++) {
        if(j<B) {
            buf.array[j]=str[k];
            j++;
        } else {
            writeBlockTOVS(&index,i,buf);
            i++;
            j=0;
        }
    }
    setHeaderTOVS(&index,1,i);
    setHeaderTOVS(&index,4,j);
}

void loading_index_file() {
    TOVS file;
    openTOVS(&file,"TOVS.bin","rb");
    TOVSblock buf1;
    int i=1;
    int k=0;
    int pos=0;
    int blk=1;
    int cpt=0;
    char str[150];
    char year[4];
    char year1[3000]="1.0";
    char year2[3000]="2.0";
    char year3[3000]="3.0";
    char year4[3000]="4.0";
    char year5[3000]="5.0";
    while(i<=getHeaderTOVS(&file,1)) {
        readBlockTOVS(&file,i,&buf1);
        int size = i==getHeaderTOVS(&file,1) ? getHeaderTOVS(&file,4) : B;
        int j=0;
        while(j<size) {

            if(cpt==0) {
                pos=j;
                blk=i;
            }

            if(buf1.array[j]==FieldSep) {
                cpt=1;
            }

            if(buf1.array[j]!=RecSep) {
                cpt=0;
                str[k]=buf1.array[j];
                k++;
            } else {
                str[k]='\0';
                k=0;
                treat_str(str,year);
                if(strcmp(year,"1.0")==0) {
                    create_str(year1,blk,pos);
                } else if(strcmp(year,"2.0")==0) {
                    create_str(year2,blk,pos);
                } else if(strcmp(year,"3.0")==0) {
                    create_str(year3,blk,pos);
                } else if(strcmp(year,"4.0")==0) {
                    create_str(year4,blk,pos);
                } else if(strcmp(year,"5.0")==0) {
                    create_str(year5,blk,pos);
                }
            }
            j++;
        }
        i++;
    }
    year1[strlen(year1)]=RecSep;
    printf("%s\n",year1);
    year2[strlen(year2)]=RecSep;
    year3[strlen(year3)]=RecSep;
    year4[strlen(year4)]=RecSep;
    year5[strlen(year5)]=RecSep;
    insert_index(year1);
    insert_index(year2);
    insert_index(year3);
    insert_index(year4);
    insert_index(year5);

    closeTOVS(&file);
}

void search(char *year){
    TOVS file;
    TOVS index;
    TOVSblock buf1;
    openTOVS(&file,"TOVS.bin","rb");
    openTOVS(&index,"indexYear.bin","rb");
    int j=0;
    int cpt =0;
    int i=0;
    int bnb=1;
    char yeartotest[4];
    if (strcmp(year,"1.0")==0)
    {
        cpt =1;
    }else if (strcmp(year,"2.0")==0)  
    {
        cpt =2;
    }else if(strcmp(year,"3.0")==0){
        cpt =3;
    }
    else if(strcmp(year,"4.0")==0){
        cpt =4;
    }
    else if(strcmp(year,"5.0")==0){
        cpt =5;
    }
    readBlockTOVS(&index,bnb,&buf1);
    while (i!=cpt)
    {
        if (buf1.array[j]==RecSep)
        {
            i++;
        }else{
            j++;
            if (j>B)
            {
                j=0;
                bnb++;
                readBlockTOVS(&index,bnb,&buf1);
            }
            
        }
        
        
    }
    
    
    closeTOVS(&file);
    closeTOVS(&index);
}

int main() {
    createTOVS("indexYear.bin");
    loading_index_file();
    return 0;
}