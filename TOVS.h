#include"TOF.h"


#define B 1000
#define RecSep '#'
#define FieldSep '@'

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

void openTOVS(TOVS *file, char *filename, char *mode);

void closeTOVS(TOVS *file);

void setHeaderTOVS(TOVS *file, int field, int val);

int getHeaderTOVS(TOVS *file, int field);

void readBlockTOVS(TOVS *file, int Bnb, TOVSblock *buffer);

void writeBlockTOVS(TOVS *file, int Bnb, TOVSblock buffer);

void createTOVS(char *filename);

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

void createTOVS(char *filename) //creates a new file with the header (empty file)
{
    TOVS file;
    openTOVS(&file, filename, "wb+");
    setHeaderTOVS(&file, 1, 0);
    setHeaderTOVS(&file, 2, 0);
    setHeaderTOVS(&file, 3, 0);
    setHeaderTOVS(&file, 4, 0);
    closeTOVS(&file);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct tovs_info
{
    char id[6];
    char year[4];
    char info[150];
} tovs_info;

void extract_string(TOVS *file, TOVSblock *buf, int *blk, int *pos, char *val, char *del); //gets the id number and the deletion flag from the record (string)

void searchTOVS(char *filename, char *key, bool *found, int *blk, int *pos); //search for a record in the TOVS file with a given id

void insertTOVS(char *filename, char rec[200]); //insert a given record (string) in the TOVS file

void deleteTOVS(char *filename, char *key); //delete a record with a given id from the TOVS file

void getfield(char *field, char *full_str, int *index);//appends a given field int the final string to be inserted in the TOVS (the one formed by create_string)

void create_string(rec r, tovs_info c, char full_str[200]);//creates a string from the TOF record and the csv information to be inserted in the TOVS file

void missing_fields(char *string,char* id,char * field); //counts the missing fields in the TOVS file

bool loading_TOVS(); //creates the TOVS file from the csv file and extend the TOF file information

void delete_given_recsTOVS(); //delete the records in the delete_students.csv file


void extract_string(TOVS *file, TOVSblock *buf, int *blk, int *pos, char *val, char *del) //gets the id number and the deletion flag from the record (string)
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

void delete_given_recsTOVS()
{
    failed_delete=0;
    successed_delete=0;
    
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




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//creating the TOVS file directly from the TOF file
//the old approache where there is no shifts inside the TOVS file

bool search(char key[6] ,tovs_info* r){ //search for the current id in the csv file (the ids are brought from the TOF file "there are in order")
    FILE* F;
    F = fopen("students_data_2a.csv","r");
    if (F==NULL)
    {
        perror("opening file");
        exit(1);
    }
    char string[100];
    bool stop;
    int count=1;
    fgets(string,100,F);
    while (fgets(string,100,F) && !stop)
    {
        int cpt=1;
        int index=0;
        int i=0;
        while(string[index]!='\0') {
            if (string[index]==',') {
                count++;
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
                if (count<3)
                {
                    i=0;
                    cpt++;
                }
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

void next_block(TOVS tovs_f,int *bnb,int *index,TOVSblock buffer2){ //goes to the next block if the current block is full
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

void loading_TOVS_TOF(){ //creates the TOVS file from the TOF file and the csv file (without shifts as the TOF is already ordered)
    //as the work was focused on the first loading_TOVS function there is a lot of repeated code in this function
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
                    cpt++;
                    break;
                case 2:
                    while (buffer1.array[j].first_name[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].first_name[k];
                        k++;
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                    cpt++;
                    break;
                case 3:
                    while (buffer1.array[j].last_name[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].last_name[k];
                        k++;
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                    cpt++;
                    break;
                case 4:
                    while (buffer1.array[j].birth_date[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].birth_date[k];
                        k++;
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
                    next_block(tovs_f,&bnb,&index,buffer2);
                    cpt++;
                    break;
                case 5:
                    while (buffer1.array[j].birth_city[k]!='\0')
                    {
                        buffer2.array[index]=buffer1.array[j].birth_city[k];
                        k++;
                        index++;
                        next_block(tovs_f,&bnb,&index,buffer2);
                    }
                    k=0;
                    buffer2.array[index]=FieldSep;
                    index++;
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
                                index++;
                                next_block(tovs_f,&bnb,&index,buffer2);
                            }
                            k=0;
                            buffer2.array[index]=FieldSep;
                            index++;
                            next_block(tovs_f,&bnb,&index,buffer2);
                            cpt++;
                            break;
                        case 2:
                        while (info.info[k]!='\0')
                            {
                                buffer2.array[index]=info.info[k];
                                k++;
                                index++;
                                next_block(tovs_f,&bnb,&index,buffer2);
                            }
                            k=0;
                            cpt++;
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
        bnb++;
        writeBlockTOVS(&tovs_f,bnb,buffer2);
    }
    setHeaderTOVS(&tovs_f,1,bnb);
    setHeaderTOVS(&tovs_f,2,recnb);
    setHeaderTOVS(&tovs_f,4,index);
    closeTOF(&tof_f);
    closeTOVS(&tovs_f);
}