#include"TOVS.h"


int main()
{
    bool recreate=false;
    char decision;
    printf("The files are already created\n");
    printf("you can checkout the statistics in the four files provided\n");
    
    printf("do you want to recreate the files ? (y/n) : ");
    scanf("%c",&decision);  
    if (decision=='y')
    {
        recreate=true;
    }else{
        recreate=false;
    }
    
    if (recreate) {

        FILE *statsTOF;
        statsTOF = fopen("statsTOF.txt", "w");
        fclose(statsTOF);

        FILE *statsTOVS;
        statsTOVS = fopen("statsTOVS.txt", "w");
        fclose(statsTOVS);
        
        FILE *missing_val;
        missing_val=fopen("missing_fields.txt","w");
        fclose(missing_val);

        FILE *stats;
        stats=fopen("stats.txt","w");

        createTOF("TOF.bin");
        loading_TOF();

        TOF tof_f;
        openTOF(&tof_f, "TOF.bin", "rb+");
        fprintf(stats,"number of blocks in TOF is :%d\n", getHeaderTOF(&tof_f, 1));
        fprintf(stats,"number of records in TOF is :%d\n", getHeaderTOF(&tof_f, 2));
        closeTOF(&tof_f);

        
        loading_fact();
        frag_stat();
        fprintf(stats,"TOF fragmentation inside each record is %d bytes\n",fragmentation);
        fprintf(stats,"TOF fragmentation rate inside each record is %0.3lf\n",frag_rate);
        fprintf(stats,"loading factor for the TOF file before delete is : %0.3lf\n",avr_fact);

        
        fprintf(stats,"this is for the TOF : \n");
        fprintf(stats,"the number of failed insertions is :%d\n",failed_insert);
        fprintf(stats,"the number of successed insertions is :%d\n",successed_insert);

        createTOVS("TOVS.bin");
        loading_TOVS();

        delete_given_recsTOF();
        loading_fact();
        fprintf(stats,"the number of failed deletions is :%d\n",failed_delete);
        fprintf(stats,"the number of successed deletions is :%d\n",successed_delete);
        fprintf(stats,"loading factor for the TOF file after delete is : %0.3lf\n",avr_fact);

        TOVS tovs_f;
        openTOVS(&tovs_f, "TOVS.bin", "rb+");
        fprintf(stats,"number of blocks in TOVS is :%d\n", getHeaderTOVS(&tovs_f, 1));
        fprintf(stats,"number of records in TOVS is :%d\n", getHeaderTOVS(&tovs_f, 2));
        closeTOVS(&tovs_f);

        delete_given_recsTOVS();

        fprintf(stats,"this is for the TOVS : \n");
        fprintf(stats,"the number of failed insertions is :%d\n",failed_insert);
        fprintf(stats,"the number of successed insertions is %d\n",successed_insert);    
        fprintf(stats,"the number of failed deletions is :%d\n",failed_delete);
        fprintf(stats,"the number of successed deletions is :%d\n",successed_delete);
        fclose(stats);

        // INDEX TESTING
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

    } else {
        printf("These are some of the statistics \n");
        FILE *stats;
        stats=fopen("stats.txt","r");
        char c;
        while ((c = fgetc(stats)) != EOF)
        {
            printf("%c",c);
        }
        fclose(stats);

        FILE *missing;
        missing=fopen("missing_fields.txt","r");
        while ((c = fgetc(missing)) != EOF)
        {
            printf("%c",c);
        }
        fclose(missing);
        printf("Thank you for using our program\n");
    }
    


    return 0;
}
    
    
    
    
    
    // printing the contentes of the files was used for debugging
    
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