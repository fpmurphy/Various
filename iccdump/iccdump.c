
/* 
 * Copyright 1997 - 2012 Graeme W. Gill
 *
 * This material is licensed with an "MIT" free use license:-
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include "icc.h"

#define MXTGNMS 30

void
error(char *fmt, ...)
{
    va_list args;

    fprintf(stderr,"ERROR: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(1);
}


void 
usage(void) {
    fprintf(stderr,"usage: iccdump infile\n");
    exit(1);
}


int
main(int argc, char *argv[]) {
    int offset = 0;        /* Offset to read profile from */
    int found;
    icmFile *fp, *op;
    icc *icco;
    int rv = 0;
    
    if (argc < 2)
        usage();

    /* Open up the file for reading */
    if ((fp = new_icmFileStd_name(argv[1],"r")) == NULL)
        error("Cannot open file '%s'", argv[1]);

    if ((icco = new_icc()) == NULL)
        error("Creation of ICC object failed");

    /* open output stream */
    if ((op = new_icmFileStd_fp(stdout)) == NULL)
        error("Cannot open stdout stream");

    do {
        found = 0;

        /* Dumb search for magic number */
        int fc = 0;
        char c;
        
        if (fp->seek(fp, offset) != 0)
            break;

        while(found == 0) {
            if (fp->read(fp, &c, 1, 1) != 1)
                break;
            
            offset++;
                
            switch (fc) {
                case 0:
                    if (c == 'a')
                        fc++;
                    else
                        fc = 0;
                    break;
                case 1:
                    if (c == 'c')
                        fc++;
                    else
                        fc = 0;
                    break;
                case 2:
                    if (c == 's')
                        fc++;
                    else
                        fc = 0;
                    break;
                case 3:
                    if (c == 'p') {
                        found = 1;
                        offset -= 40;
                    } else
                        fc = 0;
                    break;
            }
        }

        if (found) {
            printf("Embedded ICC profile found at file offset %d (0x%x)\n",offset,offset);
            if ((rv = icco->read(icco,fp,offset)) != 0)
                error("%d, %s", rv, icco->err);
            else 
                icco->dump(icco, op, 3);
            offset += 128;
        }
    } while (found != 0);

    icco->del(icco);
    op->del(op);
    fp->del(fp);

    return 0;
}



