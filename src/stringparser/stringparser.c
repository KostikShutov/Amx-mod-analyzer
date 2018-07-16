#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sc.h"

static FILE *fpamx;
static AMX_HEADER amxhdr;

typedef cell (*OPCODE_PROC)(FILE *ftxt, const cell *params, cell opcode, cell cip);

static void expand(unsigned char *code, long codesize, long memsize)
{
    ucell c;
    struct
    {
        long memloc;
        ucell c;
    } spare[AMX_COMPACTMARGIN];
    int sh = 0, st = 0, sc = 0;
    int shift;
    assert(memsize % sizeof(cell) == 0);
    while (codesize > 0)
    {
        c = 0;
        shift = 0;
        do
        {
            codesize--;
            assert(shift < 8 * sizeof(cell));
            assert(shift > 0 || (code[(size_t)codesize] & 0x80) == 0);
            c |= (ucell)(code[(size_t)codesize] & 0x7f) << shift;
            shift += 7;
        }
        while (codesize > 0 && (code[(size_t)codesize - 1] & 0x80) != 0);

        if ((code[(size_t)codesize] & 0x40) != 0)
        {
            while (shift < (int)(8 * sizeof(cell)))
            {
                c |= (ucell)0xff << shift;
                shift += 8;
            }
        }

        while (sc && (spare[sh].memloc > codesize))
        {
            *(ucell *)(code + (int)spare[sh].memloc) = spare[sh].c;
            sh = (sh + 1) % AMX_COMPACTMARGIN;
            sc--;
        }
        memsize -= sizeof(cell);
        assert(memsize >= 0);
        if ((memsize > codesize) || ((memsize == codesize) && (memsize == 0)))
        {
            *(ucell *)(code + (size_t)memsize) = c;
        }
        else
        {
            assert(sc < AMX_COMPACTMARGIN);
            spare[st].memloc = memsize;
            spare[st].c = c;
            st = (st + 1) % AMX_COMPACTMARGIN;
            sc++;
        }
    }
    assert(memsize == 0);
}

static char addchars(cell value, int pos)
{
    for (int i = 0, v; i < sizeof(cell); i++)
    {
        v = (value >> 8 * (sizeof(cell) - 1)) & 0xff;
        value <<= 8;
        if(v >= 32) return (char)v;
    }
    return ' ';
}

int execute(int argc, char *argv[])
{
    char data[FILENAME_MAX];
    FILE *fpdata;
    int codesize, count;
    cell *code, *cip, tmp;
    char sym;

    strcpy(data, "data.txt");
    if ((fpamx = fopen(argv[1], "rb")) == NULL)
    {
        printf("Unable to open input file \"%s\"\n", argv[1]);
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
    codesize = amxhdr.hea - amxhdr.cod;

    if ((code = malloc(codesize)) == NULL)
    {
        printf("Insufficient memory: need %d bytes\n", codesize);
        return 1;
    }

    fseek(fpamx, amxhdr.cod, SEEK_SET);
    if ((int32_t)fread(code, 1, codesize, fpamx) < amxhdr.size - amxhdr.cod)
    {
        printf("Unable to read code: %s\n", feof(fpamx) ? "End of file reached" : strerror(errno));
        return 1;
    }
    if ((amxhdr.flags & AMX_FLAG_COMPACT) != 0)
        expand((unsigned char *)code, amxhdr.size - amxhdr.cod, amxhdr.hea - amxhdr.cod);

    cip = (cell*)((unsigned char*)code + (amxhdr.dat - amxhdr.cod));
    codesize = amxhdr.hea - amxhdr.cod;
    count = 0;
    fprintf(fpdata, "[00000000] >> ");
    while (((unsigned char*)cip - (unsigned char*)code) < codesize)
    {
        if (count == 0)
        {
            tmp = (cell)((cip-code) * sizeof(cell) - (amxhdr.dat - amxhdr.cod));
        }
        if((int)*cip == 0)
        {
            fprintf(fpdata,"\n[%08"PRIxC"] >> ", tmp);
        }
        sym = addchars(*cip, count);
        fprintf(fpdata, "%c", sym);
        count = (count + 1) % 4;
        cip++;
    }
    free(code);
    fclose(fpamx);
    fclose(fpdata);
    return 0;
}