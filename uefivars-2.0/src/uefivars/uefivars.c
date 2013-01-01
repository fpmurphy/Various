/*
    uefivars.c - Lists UEFI variables as exported in /sys/firmware/efi

    Copyright (C) Finnbarr P. Murphy  <fpmATfpmurphy.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Parts of this code come from efibootmgr.c which has the following 
    copyright:
 
    Copyright (C) 2001-2004 Dell, Inc. <Matt_Domsch@dell.com>
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include "list.h"
#include "efi.h"
#include "efichar.h"


typedef struct _var_entry {
    struct dirent *name;
    uint16_t       num;
    efi_variable_t var_data;
    list_t         list;
} var_entry_t;

static LIST_HEAD(list_head);

void
free_vars(list_t *head)
{
    list_t *pos, *n;
    var_entry_t *boot;

    list_for_each_safe(pos, n, head) {
        boot = list_entry(pos, var_entry_t, list);
        list_del(&(boot->list));
        free(boot);
    }
}

void
read_vars(struct dirent **namelist,
          int num_boot_names,
          list_t *head)
{
    efi_status_t status;
    var_entry_t *entry;
    int i;

    if (!namelist) return;

    for (i=0; i < num_boot_names; i++) {
        if (namelist[i]) {
            entry = malloc(sizeof(var_entry_t));
            if (!entry) return;
            memset(entry, 0, sizeof(var_entry_t));

            status = read_variable(namelist[i]->d_name, &entry->var_data);
            if (status == EFI_SUCCESS) { 
                entry->name = namelist[i];
                list_add_tail(&entry->list, head);
            } else 
                free(entry);
        }
    }
    return;
}


void
free_dirents(struct dirent **ptr, 
             int num_dirents)
{
    int i;
    if (!ptr) return;
    for (i=0; i < num_dirents; i++) {
        if (ptr[i]) {
            free(ptr[i]);
            ptr[i] = NULL;
        }
    }
    free(ptr);
}


// UTF16 can have the byte order two ways (Big Endian, Little Endian).  
// In order to determine if a string is UTF16 and what the byte order is
// requires reading the first two bytes of the data 
int
test_for_utf16(uint8_t *data, uint8_t size)
{
   if (size > 2 && *data == 0 && *(data + 1) != 0) 
       return 1;                                     // encoding = utf16_be
   if (size > 2 && *data != 0 && *(data + 1) == 0) 
       return 1;                                     // encoding = utf16_le
   
   return 0;                                         // not utf16 encoding
}


void
usage()
{
    printf("usage: uefivars [-v | --version]\n");
    printf("       uefivars [-a | --attributes] [-u | --unknown]\n");
}


int
main(int argc, 
     char **argv)
{
    int c, first, showunknown = 0, showattr = 0;
    int option_index = 0;
    int num_all = 0;
    list_t *pos;
    var_entry_t *var;
    char desc[1032];
    char tmp[1024];
    char attributes[12];
    struct dirent  **all = NULL;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"unknown", no_argument, 0, 'u'},
        {"version", no_argument, 0, 'v'},
        {"attributes", no_argument, 0, 'a'},
        {0, 0, 0, 0}
    };

    while ((c = getopt_long(argc, argv, "huva", long_options, &option_index)) != -1) {
        switch (c) {
            case 'a':
                 showattr = 1;
                 break;
            case 'h':
                 usage();
                 exit(EXIT_SUCCESS);
             case 'u':
                 showunknown = 1;
                 break;
             case 'v':
                 fprintf(stdout, "version %s\n", UEFIVARS_VERSION);
                 exit(EXIT_SUCCESS);
             default: /* '?' */
                 usage();
                 exit(EXIT_FAILURE);
        }
    }

    set_fs_kernel_calls();
        
    /* retrieve all entries */
    num_all = read_all_var_names(&all);
    read_vars(all, num_all, &list_head);

    /* loop through each entry and parse */
    list_for_each(pos, &list_head) {
        var = list_entry(pos, var_entry_t, list);
        efichar_to_char(desc, var->var_data.VariableName , sizeof(desc));

        /* parse attributes and build attributes text string */
        memset(attributes, 0, 12);
        if (showattr) {
            first = 1;
            if (var->var_data.Attributes & EFI_VARIABLE_NON_VOLATILE) {
                if (first) {
                   strcpy(attributes, " (");
                   first = 0;
                } else strcat(attributes, ","); 
                strcat(attributes, "NV");
            }
            if (var->var_data.Attributes & EFI_VARIABLE_BOOTSERVICE_ACCESS) {
                if (first) {
                    strcpy(attributes, " (");
                    first = 0;
                } else strcat(attributes, ","); 
                strcat(attributes, "BS");
            }
            if (var->var_data.Attributes & EFI_VARIABLE_RUNTIME_ACCESS) {   
                if (first) {
                    strcpy(attributes, " (");
                    first = 0;
                } else strcat(attributes, ","); 
                strcat(attributes, "RT");
            }
            if (!first) strcat(attributes, ")");
        }  

	if (!strncmp(desc, "Boot", 4) &&
             isxdigit(desc[4]) && 
             isxdigit(desc[5]) &&
             isxdigit(desc[6]) && 
             isxdigit(desc[7])) {

            char description[80];
            EFI_LOAD_OPTION *load_option;
            EFI_DEVICE_PATH *path;
            char text_path[1024], *p;
            unsigned long optional_data_len=0;

            load_option = (EFI_LOAD_OPTION *) var->var_data.Data;
            efichar_to_char(description, load_option->description, sizeof(description));
            memset(text_path, 0, sizeof(text_path));
            path = load_option_path(load_option);

            if (var->name)
                printf("%.8s%s:", var->name->d_name, attributes);
            else
                printf("Boot%04X%s:", var->num, attributes);

            if (load_option->attributes & LOAD_OPTION_ACTIVE)
                printf("* ");
            else  
                printf("  ");

            printf("%s", description);

            unparse_path(text_path, path, load_option->file_path_list_length);
            optional_data_len = var->var_data.DataSize - load_option->file_path_list_length -
                                ((char *)path - (char *)load_option);
            if (optional_data_len) {
                 p = text_path;
                 p += strlen(text_path);
                 unparse_raw_text(p, ((uint8_t *)path) + load_option->file_path_list_length, optional_data_len);
            }
            printf("\t%s\n", text_path);
        } else if (!strcmp(desc, "BootCurrent") ||
                   !strcmp(desc, "BootNext") ||
                   !strcmp(desc, "BootOptionSupport")) {
            uint16_t *n = (uint16_t *)(var->var_data.Data);
            printf("%s%s: %04X\n", desc, attributes, (unsigned int)*n);
        } else if (!strcmp(desc, "BootOrder")) {
            int i;
            uint16_t *data = (uint16_t *)(var->var_data.Data);
            int count = (var->var_data.DataSize)/2;
            printf("BootOrder%s: ", attributes);
            for (i = 0; i < count; i++) {
                printf("%04X", (unsigned int)*(data +i));
                if (i < count - 1)
                    printf(",");
            }
            printf("\n");
        } else if (!strcmp(desc, "ConIn") || 
                   !strcmp(desc, "ConInDev") ||
                   !strcmp(desc, "ConOut") ||
                   !strcmp(desc, "ConOutDev") ||
                   !strcmp(desc, "ErrOut") ||
                   !strcmp(desc, "ErrOutDev")) {
            printf("%s%s: ", desc, attributes);
            efi_print_device_path(var->var_data.Data);
        } else if (!strcmp(desc, "Lang") || 
                   !strcmp(desc, "PlatformLang") ||
                   !strcmp(desc, "LangCodes") ||
                   !strcmp(desc, "PlatformLangCodes")) {
            int i;
            uint8_t *data = (uint8_t *)(var->var_data.Data);
            if (test_for_utf16(var->var_data.Data, var->var_data.DataSize) > 0 ) { 
               efichar_to_char(tmp, (efi_char16_t *)(var->var_data.Data), var->var_data.DataSize);
            } else {
               for (i = 0; i < var->var_data.DataSize; i++) {
                  tmp[i] = *(data + i);
               } 
               tmp[i] = '\0';
            }
            printf("%s%s: %s\n", desc, attributes, tmp);
        } else if (!strcmp(desc, "Timeout")) {
            uint16_t *n = (uint16_t *)(var->var_data.Data);
            printf("Timeout%s: %d secs\n", attributes, (int)*n);
        } else if (showunknown) {
            printf("%s %s: UNKNOWN\n", desc, attributes);
        }
    }

    free_dirents(all, num_all);
    free_vars(&list_head);

    return 0;
}
