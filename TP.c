#include"TOVS.h"

#define TuppleSep '?'

void treat_str(char* str ,char* year) { //extract the year from the given record (string)
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

void create_str(char* str,int blk,int pos) { //creating the years's string by appending the found block and position
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
        str[j]=nbpos[i];
        j++;
    }
    str[j]='\0';
}

void insert_index(char* str) { //insert a year's string inside the index file
    TOVS index;
    TOVSblock buf;
    openTOVS(&index,"indexYear.bin","rb+");
    int j=getHeaderTOVS(&index,4);
    int i = getHeaderTOVS(&index,1);
    i = i==0 ? 1 : i; //in case of an empty file
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
    writeBlockTOVS(&index,i,buf);
    setHeaderTOVS(&index,1,i);
    setHeaderTOVS(&index,4,j);
    closeTOVS(&index);
}

void loading_index_file() { //creates the index file based on the TOVS file
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
    char year1[20000]="1.0";
    char year2[20000]="2.0";
    char year3[20000]="3.0";
    char year4[20000]="4.0";
    char year5[20000]="5.0";
    while(i<=getHeaderTOVS(&file,1)) {
        readBlockTOVS(&file,i,&buf1);
        int size = i==getHeaderTOVS(&file,1) ? getHeaderTOVS(&file,4) : B;
        int j=0;
        while(j<size) {

            if(cpt==0) {
                pos=j;
                blk=i;
                cpt=1;
            }

            if(buf1.array[j]!=RecSep) {
                str[k]=buf1.array[j];
                k++;
            } else {
                cpt=0;
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

void search(char *year){ //gets all the info of all the students in a specific year
    TOVS file;
    TOVS index;
    TOVSblock buf1;
    TOVSblock buf2;
    openTOVS(&file,"TOVS.bin","rb");
    openTOVS(&index,"indexYear.bin","rb");
    int j=0;
    int cpt =0;
    int i=1;
    int bnb=0;
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
    int pos=0;
    int blk=0;
    int step = cpt==1 ? 4 : 5;
    for (int k = 0; k <step; k++)
    {
        j++;
        if (j>B)
        {
            j=0;
            bnb++;
            readBlockTOVS(&index,bnb,&buf1);
        }
    }
    while (buf1.array[j]!=RecSep)
    {
        char blk_str[6];
        char pos_str[6];
        int k=0;
        while (buf1.array[j]!=TuppleSep)
        {
            blk_str[k]=buf1.array[j];
            k++;
            j++;
            if (j>B)
            {
                j=0;
                bnb++;
                readBlockTOVS(&index,bnb,&buf1);
            }
        }
        blk_str[k]='\0';
        k=0; 
        j++;
        if (j>B)
        {
            j=0;
            bnb++;
            readBlockTOVS(&index,bnb,&buf1);
        }
        while (buf1.array[j]!=FieldSep && buf1.array[j]!=RecSep)
        {
            pos_str[k]=buf1.array[j];
            k++;
            j++;
            if (j>B)
            {
                j=0;
                bnb++;
                readBlockTOVS(&index,bnb,&buf1);
            }
        }
        if (buf1.array[j]!=RecSep) {
            j++;
            if (j>B)
            {
                j=0;
                bnb++;
                readBlockTOVS(&index,bnb,&buf1);
            }
        }
        pos_str[k]='\0';
        blk=atoi(blk_str);
        pos=atoi(pos_str);
        readBlockTOVS(&file,blk,&buf2);
        while (buf2.array[pos]!=RecSep)
        {
            if (buf2.array[pos]==FieldSep)
            {
                printf(" ");
            }else{
                printf("%c",buf2.array[pos]);
            }
            pos++;
            if (pos>B)
            {
                pos=0;
                blk++;
                readBlockTOVS(&file,blk,&buf2);
            }
        }
        printf("\n\n\n");
    }
    
    
    closeTOVS(&file);
    closeTOVS(&index);
}

int main() {

    createTOVS("indexYear.bin");
    loading_index_file();


    // TOVS index;
    // openTOVS(&index,"indexYear.bin","rb");
    // TOVSblock buf;
    // for(int i=1;i<=getHeaderTOVS(&index,1);i++) {
    //     readBlockTOVS(&index,i,&buf);
    //     int size = i==getHeaderTOVS(&index,1) ? getHeaderTOVS(&index,4) : B;
    //     for(int j=0;j<size;j++) {
    //         if (buf.array[j]==RecSep) {
    //             printf("%c\n\n\n\n",buf.array[j]);
    //         } else {
    //             printf("%c",buf.array[j]);
    //         }
    //     }
    // }
    // closeTOVS(&index);


    search("2.0");

    return 0;
}