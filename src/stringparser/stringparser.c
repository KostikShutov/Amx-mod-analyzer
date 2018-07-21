#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sc.h"
#include "../amx/amxdbg.h"

#include "funcs.h"

extern cell global_strip[MAX_STRIP];
extern int counter;

extern FILE *fpamx/*, *fpfunc*/;
extern AMX_HEADER amxhdr;
extern int dbgloaded;
extern AMX_DBG amxdbg;

extern OPCODE opcodelist[];

void func_CreateDynamicObject(char* name, FILE* _fpfunc)
{
    #define params_CreateDynamicObject 14
    float_ieee754 u;
    fprintf(_fpfunc, "%s(", name);
    for(int i = getPositionStrip(counter), j = 0; j < params_CreateDynamicObject; i = getPositionStrip(i), j++)
    {
        if((j >= 1 && j <= 6) || j == 10)
        {
            u.i = global_strip[i];
            fprintf(_fpfunc,"%f",u.f);
        }
        else
        {
            fprintf(_fpfunc,"%d", global_strip[i]);
        }

        if(j != params_CreateDynamicObject - 1)
            fprintf(_fpfunc, ", ");
    }
    fprintf(_fpfunc, ");\n");
}

void func_CreateObject(char* name, FILE* _fpfunc)
{
    #define params_CreateObject 8
    float_ieee754 u;
    fprintf(_fpfunc, "%s(", name);
    for(int i = getPositionStrip(counter), j = 0; j < params_CreateObject; i = getPositionStrip(i), j++)
    {
        if(j >= 1 && j <= 7)
        {
            u.i = global_strip[i];
            fprintf(_fpfunc,"%f",u.f);
        }
        else
        {
            fprintf(_fpfunc,"%d", global_strip[i]);
        }

        if(j != params_CreateObject - 1)
            fprintf(_fpfunc, ", ");
    }
    fprintf(_fpfunc, ");\n");
}

void func_Create3DTextLabel(char* name, FILE* _fpfunc)
{
    #define params_Create3DTextLabel 8
    float_ieee754 u;
    fprintf(_fpfunc, "%s(", name);
    for(int i = getPositionStrip(counter), j = 0; j < params_Create3DTextLabel; i = getPositionStrip(i), j++)
    {
        if(j == 0 || j == 1) // Параметр - строка
        {
            fprintf(_fpfunc,"%08"PRIxC"", global_strip[i]);
        }
        else if(j >= 2 && j <= 5) // Параметр - float
        {
            u.i = global_strip[i];
            fprintf(_fpfunc,"%f",u.f);
        }
        else // Параметр - число
        {
            fprintf(_fpfunc,"%d", global_strip[i]);
        }

        if(j != params_Create3DTextLabel - 1)
            fprintf(_fpfunc, ", ");
    }
    fprintf(_fpfunc, ");\n");
}


int execute(char *mod_name_amx, const char* mod_name)
{
    char name[FILENAME_MAX], data[FILENAME_MAX];
    FILE *fplist, *fpdata;

    strcpy(name, mod_name);
    strcat(name, "/source_");
    strcat(name, mod_name);
    strcat(name, ".txt");

    strcpy(data, mod_name);
    strcat(data, "/data_");
    strcat(data, mod_name);
    strcat(data, ".txt");

    if ((fpamx = fopen(mod_name_amx, "rb")) == NULL)
    {
        printf("Unable to open input file \"%s\"\n", mod_name_amx);
        return 1;
    }
    if ((fplist = fopen(name, "wt")) == NULL)
    {
        printf("Unable to create output file \"%s\"\n", name);
        return 1;
    }
    if ((fpdata = fopen(data, "wt")) == NULL)
    {
        printf("Unable to create output file \"%s\"\n", data);
        return 1;
    }

    fseek(fpamx, 0, SEEK_SET);
    if (fread(&amxhdr, sizeof amxhdr, 1, fpamx) == 0)
    {
        printf("Unable to read AMX header: %s\n", feof(fpamx) ? "End of file reached" : strerror(errno));
        return 1;
    }
    if (amxhdr.magic != AMX_MAGIC)
    {
        printf("Not a valid AMX file\n");
        return 1;
    }
    int codesize = amxhdr.hea - amxhdr.cod;


    cell *code;
    if ((code = malloc(codesize)) == NULL)
    {
        printf("Insufficient memory: need %d bytes\n", codesize);
        return 1;
    }

    fseek(fpamx, amxhdr.cod, SEEK_SET);
    if ((int32_t)fread(code, 1, codesize, fpamx) < amxhdr.size-amxhdr.cod)
    {
        printf("Unable to read code: %s\n", feof(fpamx) ? "End of file reached" : strerror(errno));
        return 1;
    }
    if ((amxhdr.flags & AMX_FLAG_COMPACT) != 0)
        expand((unsigned char *)code,amxhdr.size-amxhdr.cod,amxhdr.hea-amxhdr.cod);

    char line[sLINEMAX], sym;
    FILE *fpsrc;
    int count, i, j;
    cell *cip, tmp;
    OPCODE_PROC func;
    const char *filename;
    long nline, nprevline;

    //Print code
    cip = code;
    codesize = amxhdr.dat-amxhdr.cod;
    nprevline = -1;
    while (((unsigned char*)cip - (unsigned char*)code) < codesize)
    {
        if (dbgloaded)
        {
            dbg_LookupFile(&amxdbg, (cell)(cip-code)*sizeof(cell), &filename);
            dbg_LookupLine(&amxdbg, (cell)(cip-code)*sizeof(cell), &nline);
            if (filename != NULL && nline != nprevline)
            {
                fpsrc = fopen(filename, "r");
                if (fpsrc != NULL)
                {
                    for (i = 0; i <= nline; i++)
                    {
                        if (fgets(line, sizeof(line), fpsrc) == NULL)
                            break;
                        for (j=0; line[j] <= ' ' && line[j] != '\0'; j++)
                            continue;
                        if (line[j] != '\0' && i > nprevline)
                            fputs(line, fplist);
                    }
                    fclose(fpsrc);
                }
                nprevline = nline;
            }
        }
        func = opcodelist[(int)(*cip&0x0000ffff)].func;
        int flag = 0;

        if(opcodelist[(int)(*cip&0x0000ffff)].name == "sysreq.c")
        {
            flag = 1;
        }
        /*if(flag == 0) cip += func(fplist, cip+1, *cip, (cell)(cip-code)*sizeof(cell));
        else cip += func(fpfunc, cip+1, *cip, (cell)(cip-code)*sizeof(cell));*/
        cip += func(fplist, cip+1, *cip, (cell)(cip-code)*sizeof(cell));
        flag = 0;
    }

    //Print data
    cip = (cell*)((unsigned char*)code + (amxhdr.dat - amxhdr.cod));
    codesize = amxhdr.hea - amxhdr.cod;
    count = 0;
    int flag = 0;
    fprintf(fpdata, "[00000000] >> ");
    while (((unsigned char*)cip - (unsigned char*)code) < codesize)
    {
        if (count == 0)
            tmp = (cell)((cip-code) * sizeof(cell) - (amxhdr.dat - amxhdr.cod));
        if(flag == 1)
        {
            fprintf(fpdata,"\n[%08"PRIxC"] >> ", tmp + count * 4);
            flag = 0;
        }
        if((int)*cip == 0)
            flag = 1;
        sym = addchars(*cip, count);
        fprintf(fpdata, "%c", sym);
        count = (count + 1) % 4;
        cip++;
    }

    if (dbgloaded) dbg_FreeInfo(&amxdbg);

    free(code);
    fclose(fpamx);
    fclose(fplist);
    fclose(fpdata);
    //fclose(fpfunc);
    return 0;
}