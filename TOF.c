#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX 100
#define LoadFact 0.75 // loading factor for bulk loading


int writenum=0;
int readnum=0;
int fragmentation=0;

// missing_values //

int miss_firstname=0;
int miss_lastname=0;
int miss_birthdate=0;
int miss_birthcity=0;
int miss_year=0;
int miss_skill=0;

// counters for failed and successed insertions and deletions //
int failed_insert=0;
int successed_insert=0;
int failed_delete=0;
int successed_delete=0;
//-----------------------------------------//


typedef struct rec
{
    char id[6];
    bool del;
    char first_name[20];
    char last_name[20];
    char birth_date[11];
    char birth_city[20];
} rec;

typedef struct TOFblock
{
    rec array[MAX];
    int Nb;
    int deleted; // number of deleted records inside the block
} TOFblock;

typedef struct TOFHeader
{
    int BlkNb;
    int RecNb;    // total undeleted records in the file
    int DelRecNb; // total deleted records in the file
} TOFHeader;

typedef struct index_array
{
    char id[6];
    int blk;
} index_array;

typedef struct indexTOF
{
    int size;
    index_array array[200];
} indexTOF;

typedef struct TOF
{
    TOFHeader header;
    FILE *f;
    indexTOF index;
} TOF;

TOF globalTOF;

void openTOF(TOF *file, char *filename, char *mode);

void closeTOF(TOF *file);

void setHeaderTOF(TOF *file, int field, int val);

int getHeaderTOF(TOF *file, int field);

void readBlockTOF(TOF *file, int Bnb, TOFblock *buffer);

void writeBlockTOF(TOF *file, int Bnb, TOFblock buffer);

void createTOF(char *filename);

void openTOF(TOF *file, char *filename, char *mode)
{
    file->f = fopen(filename, mode);
    if (file->f == NULL)
    {
        perror("error openning the file");
        exit(1);
    }
    int n = fread(&(file->header), sizeof(TOFHeader), 1, file->f);
    if (n != 1)
    {
        setHeaderTOF(file, 1, 0);
        setHeaderTOF(file, 2, 0);
        setHeaderTOF(file, 3, 0);
    }
}

void closeTOF(TOF *file)
{
    fseek(file->f, 0, SEEK_SET);
    fwrite(&(file->header), sizeof(TOFHeader), 1, file->f);
    fclose(file->f);
}

void setHeaderTOF(TOF *file, int field, int val)
{
    if (file->f != NULL)
    {
        if (field == 1)
        {
            file->header.BlkNb = val;
        }
        else if (field == 2)
        {
            file->header.RecNb = val;
        }
        else if (field == 3)
        {
            file->header.DelRecNb = val;
        }
    }
}

int getHeaderTOF(TOF *file, int field)
{
    if (file->f != NULL)
    {
        if (field == 1)
        {
            return file->header.BlkNb;
        }
        else if (field == 2)
        {
            return file->header.RecNb;
        }
        else if (field == 3)
        {
            return file->header.DelRecNb;
        }
    }
    return -1;
}

void readBlockTOF(TOF *file, int Bnb, TOFblock *buffer)
{
    if (Bnb <= getHeaderTOF(file, 1))
    {
        fseek(file->f, (Bnb - 1) * sizeof(TOFblock) + sizeof(TOFHeader), SEEK_SET);
        fread(buffer, sizeof(TOFblock), 1, file->f);
    }
}

void writeBlockTOF(TOF *file, int Bnb, TOFblock buffer)
{
    fseek(file->f, (Bnb - 1) * sizeof(TOFblock) + sizeof(TOFHeader), SEEK_SET);
    fwrite(&buffer, sizeof(TOFblock), 1, file->f);
}

void createTOF(char *filename)
{
    TOF file;
    openTOF(&file, filename, "wb+");
    setHeaderTOF(&file, 1, 0);
    setHeaderTOF(&file, 2, 0);
    setHeaderTOF(&file, 3, 0);
    closeTOF(&file);
}

void binary_search(char *filename, char *key, bool *found, int *blk, int *pos)
{
    TOF file;
    TOFblock buf;
    openTOF(&file, filename, "rb+");
    int lo = 1;
    int up = getHeaderTOF(&file, 1);
    *found = false;
    *pos = 0;
    *blk = 1;
    bool stop = false;
    while (lo <= up && !(*found) && !stop)
    {
        *blk = (lo + up) / 2;
        readBlockTOF(&file, *blk, &buf);
        readnum++;
        if (strcmp(key, buf.array[0].id) >= 0 && strcmp(key, buf.array[buf.Nb - 1].id) <= 0)
        {
            int inf = 0, sup = buf.Nb - 1;
            while (inf <= sup && !(*found))
            {
                *pos = (inf + sup) / 2;
                if (!buf.array[*pos].del && strcmp(key, buf.array[*pos].id) == 0)
                {
                    *found = true;
                }
                else if (strcmp(key, buf.array[*pos].id) < 0)
                {
                    sup = *pos - 1;
                }
                else
                {
                    inf = *pos + 1;
                }
            }
            if (!(*found))
            {
                *pos = inf;
                stop = true; // this is the case where the block is found but the rec is not
            }
        }
        else if (buf.Nb < MAX * LoadFact && strcmp(key, buf.array[*pos].id) > 0)
        {
            *pos = buf.Nb;
            lo = *blk;
            stop = true;
        }
        else
        {
            if (strcmp(key, buf.array[0].id) < 0)
            {
                up = *blk - 1;
            }
            else
            {
                lo = *blk + 1;
            }
        }
    }
    if (!(*found) && !stop)
    {
        *blk = lo;
    }
    closeTOF(&file);
}

void insertTOF(rec r, char *filename)
{
    bool found;
    int blk, pos;
    binary_search(filename, r.id, &found, &blk, &pos);
    if (!found)
    {
        //TOF fragmentation
        fragmentation+=sizeof(rec)-(strlen(r.id)+strlen(r.first_name)+strlen(r.last_name)+strlen(r.birth_city)+strlen(r.birth_date)+1);
        TOF file;
        TOFblock buf;
        openTOF(&file, filename, "rb+");
        if (getHeaderTOF(&file, 1) == 0)
        {
            buf.array[0] = r;
            buf.Nb = 1;
            buf.deleted = 0;
            setHeaderTOF(&file, 1, 1);
            writeBlockTOF(&file, 1, buf);
            writenum++;
        }
        else
        {
            bool continu = true;
            rec x;
            readBlockTOF(&file, blk, &buf);
            readnum++;
            if (pos == buf.Nb)
            {
                buf.array[buf.Nb] = r;
                buf.Nb++;
                writeBlockTOF(&file, blk, buf);
                writenum++;
                continu = false;
            }
            while (continu && blk <= getHeaderTOF(&file, 1))
            {
                x = buf.array[buf.Nb - 1];
                int k = buf.Nb - 1;
                while (k > pos)
                {
                    buf.array[k] = buf.array[k - 1];
                    k--;
                }
                buf.array[pos] = r;
                if (buf.Nb < MAX * LoadFact)
                {
                    buf.Nb++;
                    buf.array[buf.Nb - 1] = x;
                    writeBlockTOF(&file, blk, buf);
                    writenum++;
                    continu = false;
                }
                else
                {
                    writeBlockTOF(&file, blk, buf);
                    writenum++;
                    blk++;
                    pos = 0;
                    r = x;
                }
                if (continu && blk <= getHeaderTOF(&file, 1))
                {
                    readBlockTOF(&file, blk, &buf);
                    readnum++;
                }
            }
            if (blk > getHeaderTOF(&file, 1))
            { // same for if(continu)
                buf.array[0] = r;
                buf.Nb = 1;
                buf.deleted = 0;
                writeBlockTOF(&file, blk, buf);
                writenum++;
                setHeaderTOF(&file, 1, blk);
            }
        }
        setHeaderTOF(&file, 2, getHeaderTOF(&file, 2) + 1);
        successed_insert++;
        closeTOF(&file);
    } else {
        // printf("insertion failed : the record with id %s already exists\n",r.id);
        failed_insert++;
    }
}

void loading_TOF()
{
    fragmentation=0;
    FILE *F;
    F = fopen("students_data_1a.csv", "r");
    if (F == NULL)
    {
        perror("opening file");
        exit(1);
    }

    readnum=0;
    writenum=0;
    FILE *statsTOF;
    statsTOF = fopen("statsTOF.txt", "a+");
    if (statsTOF == NULL)
    {
        perror("opening file");
        exit(1);
    }
    fprintf(statsTOF, "insertion of the following ids\n\n\n");


    char string[100];
    rec r;
    fgets(string, sizeof(string), F);
    while ((fgets(string, sizeof(string), F)))
    {
        int cpt = 1;
        int index = 0;
        int i = 0;
        while (string[index] != '\0')
        {
            if (string[index] == ',')
            {
                switch (cpt)
                {
                case 1:
                    r.id[i] = '\0';
                    break;
                case 2:
                    r.first_name[i] = '\0';
                    break;
                case 3:
                    r.last_name[i] = '\0';
                    break;
                case 4:
                    r.birth_date[i] = '\0';
                    break;
                }
                i = 0;
                cpt++;
            }
            else
            {
                switch (cpt)
                {
                case 1:
                    r.id[i] = string[index];
                    break;
                case 2:
                    r.first_name[i] = string[index];
                    break;
                case 3:
                    r.last_name[i] = string[index];
                    break;
                case 4:
                    r.birth_date[i] = string[index];
                    break;
                case 5:
                    r.birth_city[i] = string[index];
                    break;
                }
                i++;
            }
            index++;
        }
        r.birth_city[i] = '\0';
        r.del = false;
        
        insertTOF(r, "TOF.bin");

        fprintf(statsTOF, "insertion of the id %s costed %d reads (including the search) and %d writes\n",r.id,readnum,writenum);
        readnum = 0;
        writenum = 0; 
        
    }
    fclose(statsTOF);
    fclose(F);
}

void deleteTOF(char *id, char *filename)
{
    bool found;
    int blk, pos;
    binary_search(filename, id, &found, &blk, &pos);
    if (found)
    {
        TOF file;
        TOFblock buf;
        openTOF(&file, filename, "rb+");
        readBlockTOF(&file, blk, &buf);
        readnum++;
        buf.array[pos].del = true;
        buf.deleted++;
        writeBlockTOF(&file, blk, buf);
        writenum++;
        if (blk == 1 && buf.Nb - buf.deleted == 0)
        {
            setHeaderTOF(&file, 1, 0);
        }
        setHeaderTOF(&file, 3, getHeaderTOF(&file, 3) + 1);
        setHeaderTOF(&file, 2, getHeaderTOF(&file, 2) - 1);
        successed_delete++;
        //-----------------------------------------------------------------//
        if (pos==buf.Nb-1 && pos!=0)
        {
            strcpy(globalTOF.index.array[blk-1].id , buf.array[pos-1].id);
        } else if (pos==buf.Nb-1 && pos==0) {
            // right_shift_index(&globalTOF,blk-1,1);   not yet implemented
        }
        //-----------------------------------------------------------------//
        closeTOF(&file);
    } else {
        // printf("deletion failed : the record with id %s does not exist\n",id);
        failed_delete++;
    }
}

void delete_given_recsTOF()
{
    FILE *F;
    F = fopen("delete_students.csv", "r");
    if (F == NULL)
    {
        perror("opening file");
        exit(1);
    }

    FILE *statsTOF;
    statsTOF = fopen("statsTOF.txt", "a+");
    if (statsTOF == NULL)
    {
        perror("opening file");
        exit(1);
    }
    fprintf(statsTOF, "deletion of the following ids\n\n\n");

    char string[10];
    char id[6];
    fgets(string, 10, F);
    while (fgets(string, 10, F))
    {
        for (int i = 0; i < 5; i++)
        {
            id[i] = string[i];
        }
        id[5] = '\0';
        deleteTOF(id, "TOF.bin");

        fprintf(statsTOF,"deletion of the id %s costed %d reads (including the search) and %d writes\n",id,readnum,writenum);
        readnum = 0;
        writenum = 0; 
    }
    fclose(F);
    fclose(statsTOF);
}

//-----------------------------------------//
//-----------------------------------------//
//INDEX TOF FFUNCTIONS
//----------------------------------------//
//----------------------------------------//

void loading_index_TOF()
{ // sparse index
    openTOF(&globalTOF, "TOF.bin", "r");
    int Nblk = getHeaderTOF(&globalTOF, 1);
    int i = 1;
    TOFblock buffer;
    while (i <= Nblk)
    {
        readBlockTOF(&globalTOF, i, &buffer);
        strcpy(globalTOF.index.array[i-1].id, buffer.array[buffer.Nb-1].id);
        globalTOF.index.array[i-1].blk = i;
        i++;
    }
    globalTOF.index.size = Nblk;
    closeTOF(&globalTOF);
}

void binary_search_index(bool *found,int *i,char key[6]){
    *found=false;
    int sup=globalTOF.index.size-1;
    int inf=0;
    if (strcmp(key,globalTOF.index.array[globalTOF.index.size-1].id)>0)
    {
        *i=globalTOF.index.array[globalTOF.index.size-1].blk;
        *found=false;
    } else {
        while (sup>=inf && !(*found))
        {
            *i=(sup+inf)/2;
            if (strcmp(key,globalTOF.index.array[*i].id)==0)
            {
                *found=true;
            }else if(strcmp(key,globalTOF.index.array[*i].id)<0){
                sup=(*i)-1;
            }else{
                inf=(*i)+1;
            }
        }
        *i=globalTOF.index.array[*i].blk;
        *found=true;
    }
}

//not checked yet

// void right_shift_index(TOF *file,int pos,int n){
//     for (int i = file->index.size-1; i > pos; i--)
//     {
//         file->index.array[i+n]=file->index.array[i];
//     }
//     file->index.size+=n;
// }

// void left_shift_index(TOF *file,int pos,int n){
//     for (int i = pos; i < file->index.size-n; i++)
//     {
//         file->index.array[i]=file->index.array[i+n];
//     }
//     file->index.size-=n;
// }

void loading_fact() {
    TOF file;
    openTOF(&file, "TOF.bin", "r");
    int Nblk = getHeaderTOF(&file, 1);
    int nbrec = getHeaderTOF(&file, 2);
    double avr_fact = (double) nbrec / (Nblk*MAX) ;
    printf("average loading factor is %0.3lf\n",avr_fact);
    closeTOF(&file);
}

void frag_stat() {
    TOF file;
    openTOF(&file, "TOF.bin", "r");
    int nrec = getHeaderTOF(&file, 2);
    printf("the TOF file has %d records each one with a capcity of %zu bytes\n",nrec,sizeof(rec));
    printf("the alocated space for records is %zu bytes (without conting the remaining space inside each block)\n",nrec*sizeof(rec));
    printf("the unused space inside the records is %d bytes\n",fragmentation);
    printf("the fragmentation rate is %0.3lf\n",(double)fragmentation/(nrec*sizeof(rec)));

    closeTOF(&file);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define B 1000
#define RecSep '#'
#define FieldSep '@'

typedef struct index_array_TOVS
{
    char id[6];
    int blk;
    int pos;
} index_array_TOVS;

typedef struct indexTOVS
{
    int size;
    index_array_TOVS array[100001];
} indexTOVS;
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
    indexTOVS index;
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
        readnum++;
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
                            readnum++;
                            *pos = 0;
                        }
                    } while (buf.array[*pos] != RecSep);
                    (*pos)++;
                    if (*pos >= B)
                    {
                        (*blk)++;
                        readBlockTOVS(&file, *blk, &buf);
                        readnum++;
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
                    writenum++;
                    blk++;
                }
            }
            writeBlockTOVS(&file,blk,buf);
            writenum++;
            setHeaderTOVS(&file,1,blk);
            setHeaderTOVS(&file, 2,1);
            setHeaderTOVS(&file, 4, pos);
        } else {
            readBlockTOVS(&file, blk, &buf);
            readnum++;
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
                    writenum++;
                    blk++;
                    readBlockTOVS(&file, blk, &buf);
                    readnum++;
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
                    writenum++;
                    blk++;
                }
                count++;
            }
            writeBlockTOVS(&file,blk,buf);
            writenum++;
            setHeaderTOVS(&file,1,blk);
            setHeaderTOVS(&file, 2, getHeaderTOVS(&file, 2) + 1);
            setHeaderTOVS(&file, 4, pos);
        }
        successed_insert++;
        closeTOVS(&file);
    }else{
        // printf("insertion failed for the id : %s already exit\n",key);
        failed_delete++;
    }
}

void deleteTOVS(char *filename, char *key)
{
    bool found;
    int blk, pos;
    searchTOVS(filename, key, &found, &blk, &pos);
    if (found)
    {
        TOVS file;
        TOVSblock buf;
        openTOVS(&file, filename, "rb+");
        readBlockTOVS(&file, blk, &buf);
        readnum++;
        buf.array[pos] = 'v';
        setHeaderTOVS(&file, 2, getHeaderTOVS(&file, 2) - 1);
        setHeaderTOVS(&file, 3, getHeaderTOVS(&file, 3) + 1);
        writeBlockTOVS(&file, blk, buf);
        writenum++;
        successed_delete++;
        closeTOVS(&file);
    }else{
        // printf("deletion failed for the id : %s it does not exist\n",key);
        failed_delete++;
    }
}

void loading_index_TOVS(){
    openTOVS(&globalTOVS,"TOVS.bin","r");
    TOVSblock buf;
    bool search=true;  // search for field sep
    int i=1;
    int index=0;
    int k=0;
    while(i <= getHeaderTOVS(&globalTOVS,1)) 
    {
        readBlockTOVS(&globalTOVS,i,&buf);
        int j=0;
        while (j<B) {
            if (buf.array[j]==FieldSep)
            {
                search=false;
            } else if (search)
            {
                if (k==0) {
                    globalTOVS.index.array[index].blk=i;
                    globalTOVS.index.array[index].pos=j;
                }
                globalTOVS.index.array[index].id[k]=buf.array[j];
                k++;
            }
            if (buf.array[j]==RecSep)
            {
                search=true;
                globalTOVS.index.array[index].id[k]='\0';
                k=0;
                index++;
            }
            j++;
        }
        i++;
    }
    globalTOVS.index.size=index;
    closeTOVS(&globalTOVS);
}

//archive
//supposing that one recorde can't take a hole block
//     openTOVS(&globalTOVS,"TOVS.bin","r");
//     TOVSblock buf;
//     int cpt=0;
//     for (int i = 1; i <= getHeaderTOVS(&globalTOVS,1); i++)
//     {   
//         readBlockTOVS(&globalTOVS,i,&buf);
//         int j;
//         if (i!=getHeaderTOVS(&globalTOVS,1)) {
//             j=B-1;
//         } else if (getHeaderTOVS(&globalTOVS,4)!=0) {
//             j=getHeaderTOVS(&globalTOVS,4)-1;
//         } else {
//             j=B-1;
//         }
//         if (buf.array[j]==RecSep)
//         {
//             j--;
//         }
//         while (buf.array[j]!=RecSep)
//         {
//             j--;
//         }
//         int k=0;
//         int temp=i;
//         while (buf.array[j]!=FieldSep)
//         {
//             j++;
//             if(j==B) {
//                 j=0;
//                 temp++;
//                 readBlockTOVS(&globalTOVS,temp,&buf);
//             }
//             globalTOVS.index.array[cpt].id[k]=buf.array[j];
//             k++;
//         }
//         cpt++;
//     }
//     globalTOVS.index.size=cpt;
//     closeTOVS(&globalTOVS);
// }

typedef struct tovs_info
{
    char id[6];
    char year[4];
    char info[150];
} tovs_info;

void getfield(char *field, char *full_str, int *index)
{
    int k = 0;
    while (field[k] != '\0')
    {
        full_str[*index] = field[k];
        k++;
        (*index)++;
    }
    full_str[*index] = FieldSep;
    (*index)++;
}

void create_string(rec r, tovs_info c, char full_str[200])
{
    int index = 0;
    getfield(r.id, full_str, &index);
    full_str[index] = 'f';
    index++;
    full_str[index] = FieldSep;
    index++;
    getfield(r.first_name, full_str, &index);
    getfield(r.last_name, full_str, &index);
    getfield(r.birth_date, full_str, &index);
    getfield(r.birth_city, full_str, &index);
    getfield(c.year, full_str, &index);
    getfield(c.info, full_str, &index);
    full_str[index - 1] = RecSep;
    full_str[index] = '\0';
}

//  ----------------missing_fields------------------ //

void missing_fields(char *string,char* id,char * field){

    if (string[0]=='\0' || string[0]=='\n' || string[0]=='\r')
    {
        if (strcmp(field,"firstname")==0)
        {
            miss_firstname++;
        }
        else if (strcmp(field,"lastname")==0)
        {
            miss_lastname++;
        }
        else if (strcmp(field,"birth date")==0)
        {
            miss_birthdate++;
        }
        else if (strcmp(field,"birth city")==0)
        {
            miss_birthcity++;
        }
        else if (strcmp(field,"year")==0)
        {
            miss_year++;
        }
        else if (strcmp(field,"skills")==0)
        {
            miss_skill++;
        }
    }
}


bool loading_TOVS()
{   

    //-----------------//
    miss_firstname=0;
    miss_lastname=0;
    miss_birthdate=0;
    miss_birthcity=0;
    miss_year=0;
    miss_skill=0;
    //-----------------//
    failed_insert=0;
    successed_insert=0;
    failed_delete=0;
    successed_delete=0;
    //-----------------//

    

    FILE *missed;
    missed=fopen("missing_fields.txt","a+");
    if (missed == NULL)
    {
        perror("opening file");
        exit(1);
    }

    // stats insertion and deletion //
    FILE *stat_F;
    stat_F = fopen("statsTOVS.txt", "a+");
    if (stat_F == NULL)
    {
        perror("opening file");
        exit(1);
    }

    // csv 2 file for insertion //
    FILE *F;
    F = fopen("students_data_2a.csv", "r");
    if (F == NULL)
    {
        perror("opening file");
        exit(1);
    }
    char string[200];
    int count = 0;
    tovs_info r;
    fgets(string, sizeof(string), F);
    while (fgets(string, sizeof(string), F))
    {
        int cpt = 1;
        int index = 0;
        int i = 0;
        while (string[index] != '\n')
        {
            if (string[index] == ',')
            {
                count++;
            }
            if (string[index] == ',' && count < 3)
            {
                switch (cpt)
                {
                case 1:
                    r.id[i] = '\0';
                    break;
                case 2:
                    r.year[i] = '\0';
                    break;
                case 3:
                    r.info[i] = '\0';
                    break;
                }
                i = 0;
                cpt++;
            }
            else
            {
                switch (cpt)
                {
                case 1:
                    r.id[i] = string[index];
                    break;
                case 2:
                    r.year[i] = string[index];
                    break;
                case 3:
                    r.info[i] = string[index];
                    break;
                }
                i++;
            }
            index++;
        }
        r.info[i] = '\0';
        // traitement
        // 1-search tof (r.id)
        bool found;
        int blk, pos;
        binary_search("TOF.bin", r.id, &found, &blk, &pos);
        if (found)
        {
            rec r_TOF;
            TOF tof_f;
            TOFblock buf;
            openTOF(&tof_f, "TOF.bin", "r+");
            readBlockTOF(&tof_f, blk, &buf);
            readnum++;
            r_TOF = buf.array[pos];

            //missing fields
            missing_fields(r_TOF.first_name,r_TOF.id,"firstname");
            missing_fields(r_TOF.last_name,r_TOF.id,"lastname");
            missing_fields(r_TOF.birth_date,r_TOF.id,"birth date");
            missing_fields(r_TOF.birth_city,r_TOF.id,"birth city");
            missing_fields(r.year,r_TOF.id,"year");
            missing_fields(r.info,r_TOF.id,"skills");
            // 2-create string
            char final_str[200];
            create_string(r_TOF, r, final_str);
            
            // 3-insert TOVS
            insertTOVS("TOVS.bin", final_str);
            closeTOF(&tof_f);
        }
        count = 0;
        fprintf(stat_F,"the statistics for the insertion of the id %s are : writes : %d \t\tand reads : %d\n",r.id,writenum,readnum);
        writenum=0;
        readnum=0;
    }
    fprintf(stat_F,"\n THE statistics for deleted ids\n\n");
    fprintf(missed,"the missing fields are : \n");
    fprintf(missed,"the missing firstnames are : %d\n",miss_firstname);
    fprintf(missed,"the missing lastnames are : %d\n",miss_lastname);
    fprintf(missed,"the missing birthdates are : %d\n",miss_birthdate);
    fprintf(missed,"the missing birthcities are : %d\n",miss_birthcity);
    fprintf(missed,"the missing years are : %d\n",miss_year);
    fprintf(missed,"the missing skills are : %d\n",miss_skill);
    fclose(F);
    fclose(stat_F);
    fclose(missed);
    return false;
}

// void next_block(TOVS tovs_f,int *bnb,int *index,TOVSblock buffer2){
//     if (*index==B)
//     {
//         (*bnb)++;
//         writeBlockTOVS(&tovs_f,*bnb,buffer2);
//         *index=0;
//     }
//     else
//     {
//         return;
//     }
// }

// void loading_TOVS(){
//     TOF tof_f;
//     TOVS tovs_f;
//     openTOF(&tof_f,"TOF.bin","rb+");
//     openTOVS(&tovs_f,"TOVS.bin","rb+");
//     int Nblk=getHeaderTOF(&tof_f,1),i=1;
//     TOFblock buffer1;
//     TOVSblock buffer2;
//     int cpt=1;
//     int k=0;
//     int index=0;
//     int bnb=0;
//     int recnb=0;
//     tovs_info info;
//     while (i<=Nblk)
//     {
//         readBlockTOF(&tof_f,i,&buffer1);
//         for (int j = 0; j < buffer1.Nb; j++)
//         {
//             cpt=1;
//             while (cpt<=5)
//             {
//                 switch (cpt)
//                 {
//                 case 1:
//                     while (buffer1.array[j].id[k]!='\0')
//                     {
//                         buffer2.array[index]=buffer1.array[j].id[k];
//                         k++;
//                         index++;
//                         next_block(tovs_f,&bnb,&index,buffer2);
//                     }
//                     k=0;
//                     buffer2.array[index]=FieldSep;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     buffer2.array[index]='f';
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     buffer2.array[index]=FieldSep;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     cpt++;
//                     break;
//                 case 2:
//                     while (buffer1.array[j].first_name[k]!='\0')
//                     {
//                         buffer2.array[index]=buffer1.array[j].first_name[k];
//                         k++;
//                         index++;
//                         next_block(tovs_f,&bnb,&index,buffer2);
//                     }
//                     k=0;
//                     buffer2.array[index]=FieldSep;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     cpt++;
//                     break;
//                 case 3:
//                     while (buffer1.array[j].last_name[k]!='\0')
//                     {
//                         buffer2.array[index]=buffer1.array[j].last_name[k];
//                         k++;
//                         index++;
//                         next_block(tovs_f,&bnb,&index,buffer2);
//                     }
//                     k=0;
//                     buffer2.array[index]=FieldSep;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     cpt++;
//                     break;
//                 case 4:
//                     while (buffer1.array[j].birth_date[k]!='\0')
//                     {
//                         buffer2.array[index]=buffer1.array[j].birth_date[k];
//                         k++;
//                         index++;
//                         next_block(tovs_f,&bnb,&index,buffer2);
//                     }
//                     k=0;
//                     buffer2.array[index]=FieldSep;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     cpt++;
//                     break;
//                 case 5:
//                     while (buffer1.array[j].birth_city[k]!='\0')
//                     {
//                         buffer2.array[index]=buffer1.array[j].birth_city[k];
//                         k++;
//                         index++;
//                         next_block(tovs_f,&bnb,&index,buffer2);
//                     }
//                     k=0;
//                     buffer2.array[index]=FieldSep;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     cpt++;
//                     break;
//                 default:
//                     break;
//                 }
//             }
//             if (search(buffer1.array[j].id,&info))
//                 {
//                     cpt=1;
//                     k=0;
//                     while (cpt<=2)
//                     {
//                         switch (cpt)
//                         {
//                         case 1:
//                             while (info.year[k]!='\0')
//                             {
//                                 buffer2.array[index]=info.year[k];
//                                 k++;
//                                 index++;
//                                 next_block(tovs_f,&bnb,&index,buffer2);
//                             }
//                             k=0;
//                             buffer2.array[index]=FieldSep;
//                             index++;
//                             next_block(tovs_f,&bnb,&index,buffer2);
//                             cpt++;
//                             break;
//                         case 2:
//                         while (info.info[k]!='\0')
//                             {
//                                 buffer2.array[index]=info.info[k];
//                                 k++;
//                                 index++;
//                                 next_block(tovs_f,&bnb,&index,buffer2);
//                             }
//                             k=0;
//                             cpt++;
//                             break;
//                         default:
//                             break;
//                         }
//                     }
//                     buffer2.array[index]=RecSep;
//                     recnb++;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                 } else {
//                     printf("it happened\n");
//                     buffer2.array[index] = FieldSep;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     buffer2.array[index] = FieldSep;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                     buffer2.array[index] = RecSep;
//                     recnb++;
//                     index++;
//                     next_block(tovs_f,&bnb,&index,buffer2);
//                 }
//         }
//         i++;
//     }
//     if (index!=0) {
//         bnb++;
//         writeBlockTOVS(&tovs_f,bnb,buffer2);
//     }
//     setHeaderTOVS(&tovs_f,1,bnb);
//     setHeaderTOVS(&tovs_f,2,recnb);
//     setHeaderTOVS(&tovs_f,4,index);
//     closeTOF(&tof_f);
//     closeTOVS(&tovs_f);
// }

void delete_given_recsTOVS()
{
    
    FILE *stat_F;
    stat_F = fopen("statsTOVS.txt", "a+");
    if (stat_F == NULL)
    {
        perror("opening file");
        exit(1);
    }

    FILE *F;
    F = fopen("delete_students.csv", "r");
    if (F == NULL)
    {
        perror("opening file");
        exit(1);
    }

    char string[10];
    char id[6];
    fgets(string, 10, F);
    while (fgets(string, 10, F))
    {
        for (int i = 0; i < 5; i++)
        {
            id[i] = string[i];
        }
        id[5] = '\0';
        deleteTOVS("TOVS.bin", id);
        fprintf(stat_F,"the statistics for the deletion of the id %s are : writes : %d \t\tand reads : %d\n",id,writenum,readnum);
        readnum=0;
        writenum=0;
    }
    fclose(stat_F);
    fclose(F);
}



int main()
{
    // FILE *statsTOF;
    // statsTOF = fopen("statsTOF.txt", "w");
    // fclose(statsTOF);

    // FILE *statsTOVS;
    // statsTOVS = fopen("statsTOVS.txt", "w");
    // fclose(statsTOVS);
    
    // FILE *missing_val;
    // missing_val=fopen("missing_fields.txt","w");
    // fclose(missing_val);

    // createTOF("TOF.bin");
    // loading_TOF();
    // loading_fact();
    // frag_stat();

    
    // printf("this is for the TOF : \n");
    // printf("the number of failed insertions is :%d\n",failed_insert);
    // printf("the number of successed insertions is :%d\n",successed_insert);

    // createTOVS("TOVS.bin");
    // loading_TOVS();

    // delete_given_recsTOF();
    // loading_fact();
    // printf("the number of failed deletions is :%d\n",failed_delete);
    // printf("the number of successed deletions is :%d\n",successed_delete);

    // TOVS tovs_f;
    // openTOVS(&tovs_f, "TOVS.bin", "rb+");
    // printf("number of blocks is :%d\n", getHeaderTOVS(&tovs_f, 1));
    // printf("number of records is :%d\n", getHeaderTOVS(&tovs_f, 2));
    // closeTOVS(&tovs_f);

    // delete_given_recsTOVS();

    // printf("this is for the TOVS : \n");
    // printf("the number of failed insertions is :%d\n",failed_insert);
    // printf("the number of successed insertions is %d\n",successed_insert);    
    // printf("the number of failed deletions is :%d\n",failed_delete);
    // printf("the number of successed deletions is :%d\n",successed_delete);


    //INDEX TESTING
    // loading_index_TOF();
    // for (int j = 0; j < globalTOF.index.size; j++)
    // {
    //     printf("the %d id is : %s in block %d\n",j,globalTOF.index.array[j].id,globalTOF.index.array[j].blk);
    // }
    // int i;
    // bool found;
    // binary_search_index(&found,&i,"10153");
    // printf("found is : %d\n",found);
    // printf("the block is : %d\n",i);

    loading_index_TOVS();
    for (int j = 0; j < globalTOVS.index.size; j++)
    {
        printf("the %d id is : %s in block %d\n",j,globalTOF.index.array[j].id,globalTOF.index.array[j].blk);
    }

    return 0;
}


//  le cas //