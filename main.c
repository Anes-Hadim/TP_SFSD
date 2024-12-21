#include"TOVS.h"


int main()
{
    FILE *statsTOF;
    statsTOF = fopen("statsTOF.txt", "w");
    fclose(statsTOF);

    FILE *statsTOVS;
    statsTOVS = fopen("statsTOVS.txt", "w");
    fclose(statsTOVS);
    
    FILE *missing_val;
    missing_val=fopen("missing_fields.txt","w");
    fclose(missing_val);

    createTOF("TOF.bin");
    loading_TOF();
    loading_fact();
    frag_stat();

    
    printf("this is for the TOF : \n");
    printf("the number of failed insertions is :%d\n",failed_insert);
    printf("the number of successed insertions is :%d\n",successed_insert);

    createTOVS("TOVS.bin");
    loading_TOVS();

    delete_given_recsTOF();
    loading_fact();
    printf("the number of failed deletions is :%d\n",failed_delete);
    printf("the number of successed deletions is :%d\n",successed_delete);

    TOVS tovs_f;
    openTOVS(&tovs_f, "TOVS.bin", "rb+");
    printf("number of blocks is :%d\n", getHeaderTOVS(&tovs_f, 1));
    printf("number of records is :%d\n", getHeaderTOVS(&tovs_f, 2));
    closeTOVS(&tovs_f);

    delete_given_recsTOVS();

    printf("this is for the TOVS : \n");
    printf("the number of failed insertions is :%d\n",failed_insert);
    printf("the number of successed insertions is %d\n",successed_insert);    
    printf("the number of failed deletions is :%d\n",failed_delete);
    printf("the number of successed deletions is :%d\n",successed_delete);


    // INDEX TESTING
    loading_index_TOF();
    for (int j = 0; j < globalTOF.index.size; j++)
    {
        printf("the %d id is : %s in block %d\n",j,globalTOF.index.array[j].id,globalTOF.index.array[j].blk);
    }
    int i;
    bool found;
    binary_search_index(&found,&i,"10153");
    printf("found is : %d\n",found);
    printf("the block is : %d\n",i);



    return 0;
}
    
    
    
    
    
    //printing the contentes of the files was used for debugging
    
    // TOF File;
    // openTOF(&File,"TOF.bin","rb+");
    // TOFblock buf1;
    // printf("block number %d\n",getHeaderTOF(&File,1));
    // printf("record number %d\n",getHeaderTOF(&File,2));
    // printf("deleted recs %d\n",getHeaderTOF(&File,3));
    // for (int i = 1; i <= getHeaderTOF(&File,1); i++)
    // {
    //     readBlockTOF(&File,i,&buf1);
    //     printf("block %d\n",i);
    //     for (int j = 0; j < buf1.Nb; j++)
    //     {
    //         printf("%s %d %s %s %s %s\n",buf1.array[j].id,buf1.array[j].del,buf1.array[j].first_name,buf1.array[j].last_name,buf1.array[j].birth_date,buf1.array[j].birth_city);
    //     }
    //     printf("\n\n");
    // }
    // close(&File);
    // TOVS file;
    // openTOVS(&file,"TOVS.bin","rb+");
    // TOVSblock buf;
    // printf("block number %d\n",getHeaderTOVS(&file,1));
    // printf("record number %d\n",getHeaderTOVS(&file,2));
    // printf("deleted number %d\n",getHeaderTOVS(&file,3));
    // printf("pos %d\n",getHeaderTOVS(&file,4));
    // for (int i = 1; i <getHeaderTOVS(&file,1); i++)
    // {
    //     readBlockTOVS(&file,i,&buf);
    //     printf("\nblock %d\n",i);
    //     for (int j = 0; j < B; j++)
    //     {
    //         if (buf.array[j]==RecSep) {
    //             printf("\n");
    //         } else if (buf.array[j]==FieldSep) {
    //             printf(" ");
    //         } else {
    //             printf("%c",buf.array[j]);
    //         }
    //     }
    // }
    // readBlockTOVS(&file,getHeaderTOVS(&file,1),&buf);
    // printf("\nblock %d\n",getHeaderTOVS(&file,1));
    // for (int j = 0; j < getHeaderTOVS(&file,4); j++)
    // {
    //     if (buf.array[j]==RecSep) {
    //         printf("\n");
    //     } else if (buf.array[j]==FieldSep) {
    //         printf(" ");
    //     } else {
    //         printf("%c",buf.array[j]);
    //     }
    // }
    // closeTOVS(&file);