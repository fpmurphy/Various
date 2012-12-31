/*
  efivars_efivarfs.[ch] - Manipulates EFI variables as exported in /sys/firmware/efi/vars

  Copyright (C) 2001,2003 Dell Computer Corporation <Matt_Domsch@dell.com>

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
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <fcntl.h>

#include "efi.h"
#include "efichar.h"
//  #include "efibootmgr.h"
#include "efivars_efivarfs.h"


static efi_status_t
efivarfs_read_variable(const char *name, efi_variable_t *var)
{
    char filename[PATH_MAX];
    int fd;
    size_t readsize, datasize;
    struct stat buf;
    void *buffer;

    if (!name || !var) 
        return EFI_INVALID_PARAMETER;

    snprintf (filename, PATH_MAX-1, "%s/%s", EFIVARFS_DIR_EFI_VARS, name);
    fd = open (filename, O_RDONLY);
    if (fd == -1) {
        return EFI_NOT_FOUND;
    }
 
    if (fstat (fd, &buf) != 0) {
        close (fd);
        return EFI_INVALID_PARAMETER;
    }
 
    readsize = read (fd, &var->Attributes, sizeof(uint32_t));
    if (readsize != sizeof(uint32_t)) {
        close (fd);
        return EFI_INVALID_PARAMETER;
    }
 
    datasize = buf.st_size - sizeof(uint32_t);
 
    buffer = malloc (datasize);
    if (buffer == NULL) {
        close (fd);
        return EFI_OUT_OF_RESOURCES;
    }
 
    readsize = read (fd, buffer, datasize);
    if (readsize < 1) {
        close (fd);
        free (buffer);
        return EFI_INVALID_PARAMETER;
    }
    var->Data = buffer;
    var->DataSize = readsize;
 
    close (fd);
    return var->Status;
}


static efi_status_t
efivarfs_write_variable (const char *filename, efi_variable_t *var)
{
    int fd;
    size_t writesize;
    void *buffer;
    unsigned long total;
 
    if (!filename || !var)
        return EFI_INVALID_PARAMETER;
 
    buffer = malloc(var->DataSize + sizeof(uint32_t));
    if (buffer == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }
 
    memcpy (buffer, &var->Attributes, sizeof(uint32_t));
    memcpy (buffer + sizeof(uint32_t), var->Data, var->DataSize);
    total = var->DataSize + sizeof(uint32_t);
 
    fd = open (filename, O_WRONLY | O_CREAT,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd == -1) {
        free (buffer);
        return EFI_INVALID_PARAMETER;
    }
 
    writesize = write (fd, buffer, total);
    if (writesize != total) {
        free (buffer);
        close (fd);
        return EFI_INVALID_PARAMETER;
    }
 
    close(fd);
    free(buffer);
    return EFI_SUCCESS;
}


static efi_status_t
efivarfs_edit_variable(const char *name, efi_variable_t *var)
{
    char filename[PATH_MAX];

    if (!var) 
            return EFI_INVALID_PARAMETER;

    snprintf(filename, PATH_MAX-1, "%s/%s", EFIVARFS_DIR_EFI_VARS, name);

    return efivarfs_write_variable(filename, var);
}


static efi_status_t
efivarfs_create_variable(efi_variable_t *var)
{
    char filename[PATH_MAX];
    char name[PATH_MAX];

    if (!var) 
        return EFI_INVALID_PARAMETER;

    variable_to_name(var, name);
    snprintf(filename, PATH_MAX-1, "%s/%s", EFIVARFS_DIR_EFI_VARS, name);

    return efivarfs_write_variable(filename, var);
}


static efi_status_t
efivarfs_delete_variable(efi_variable_t *var)
{
    char name[PATH_MAX];
    char filename[PATH_MAX];
 
    if (!var)
        return EFI_INVALID_PARAMETER;
 
    variable_to_name(var, name);
    snprintf(filename, PATH_MAX-1, "%s/%s", EFIVARFS_DIR_EFI_VARS, name);
 
    if (unlink (filename) == 0)
        return EFI_SUCCESS;
 
    return EFI_OUT_OF_RESOURCES;
}


struct efivar_kernel_calls efivarfs_kernel_calls = {
    .read = efivarfs_read_variable,
    .edit = efivarfs_edit_variable,
    .create = efivarfs_create_variable,
    .delete = efivarfs_delete_variable,
    .path = EFIVARFS_DIR_EFI_VARS,
};
