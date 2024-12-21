#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX 100
#define LoadFact 0.75 // loading factor for bulk loading


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

void createTOF(char *filename) //creates a new file with the header (empty file)
{
    TOF file;
    openTOF(&file, filename, "wb+");
    setHeaderTOF(&file, 1, 0);
    setHeaderTOF(&file, 2, 0);
    setHeaderTOF(&file, 3, 0);
    closeTOF(&file);
}




//-----------------------------------------//
//-----------------------------------------//
//INDEX TOF FFUNCTIONS
//----------------------------------------//
//----------------------------------------//


void loading_index_TOF() //creates a sparse primary index based on the TOF file considering the last record in each block even if it is deleted logicaly
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

void binary_search_index(bool *found,int *i,char key[6]) { //binary search function inside the index found is a boolean that refers if the value is possible to be found in the file
                                                           //not if the actuale value exists
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



////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

//stats variables

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


void binary_search(char *filename, char *key, bool *found, int *blk, int *pos); //a binary search for a given key in the TOF file
void insertTOF(rec r, char *filename); //insert a record in the TOF file
void deleteTOF(char *id, char *filename); //delete a record from the TOF file (logical deletion)
void loading_TOF(); //loading the TOF file from a csv file
void delete_given_recsTOF(); //deleting records mentioned in the csv from the TOF file (logical deletion)
void loading_fact(); //calculating the loading factor of the TOF file
void frag_stat(); //calculating the fragmentation rate of the TOF file inside the records (does not consider the remaining space inside the blocks)

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