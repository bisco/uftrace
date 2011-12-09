/*
 * prototype.c
 * $Id: prototype.c,v 1.5 2007/06/15 13:24:54 hamano Exp $
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <glib.h>
#include <libelf.h>
#include <libdwarf/libdwarf.h>
#include <libdwarf/dwarf.h>
#include "prototype.h"

int prototype_init(GHashTable *functions, const char *filename)
{
    int fd;
    Elf *elf;

    if(!functions){
        return -1;
    }
    
    elf_version(EV_CURRENT);
    fd = open(filename, O_RDONLY);
    if(fd < 0){
        return -1;
    }
    elf = elf_begin(fd, ELF_C_READ, (Elf*)0);
    prototype_add_elf(elf, functions);
    elf_end(elf);
    return 0;
}

static gboolean prototype_finish_entry(gpointer key, gpointer value,
                                       gpointer user_data)
{
    function_t *func = value;
    GSList *list;
    arg_t *arg;

    if(!func){
        return 0;
    }
    if(func->name){
        free(func->name);
    }

    list = func->args;
    while(list){
        arg = list->data;
        if(arg){
            free(arg->typename);
            free(arg);
        }
        list = g_slist_next(list);
    }

    free(func);
    return 0;
}

void prototype_finish(GHashTable *functions)
{
    if(!functions){
        return;
    }
    g_hash_table_foreach(functions, (GHFunc)prototype_finish_entry, NULL);
}

int prototype_add_elf(Elf *elf, GHashTable *functions)
{
    int ret;
    Dwarf_Debug dbg;
    Dwarf_Die die;
    Dwarf_Error err;
    Dwarf_Unsigned cu_header_length = 0;
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Unsigned next_cu_offset = 0;

    ret = dwarf_elf_init(elf, DW_DLC_READ, NULL, NULL, &dbg, &err);
    if(ret == DW_DLV_NO_ENTRY){
        return -1;
    }
    while ((ret = dwarf_next_cu_header(
                dbg, &cu_header_length, &version_stamp, &abbrev_offset,
                &address_size, &next_cu_offset, &err)) == DW_DLV_OK){
        ret = dwarf_siblingof(dbg, NULL, &die, &err);
        if(ret == DW_DLV_NO_ENTRY){
            continue;
        }else if(ret != DW_DLV_OK){
            break;
        }
        
        prototype_add_cu(dbg, die, functions);
    }
    if(ret == DW_DLV_ERROR){
        dwarf_finish(dbg, &err);
        return -1;
    }
    dwarf_finish(dbg, &err);
    return 0;
}

int prototype_add_cu(Dwarf_Debug dbg, Dwarf_Die die, GHashTable *functions){
    int ret;
    Dwarf_Error err;
    Dwarf_Half tag;
    Dwarf_Die child;
    GHashTable *types;

    while(1){
        ret = dwarf_tag(die, &tag, &err);
        if(ret != DW_DLV_OK){
            return -1;
        }
        if(tag == DW_TAG_compile_unit){
            ret = dwarf_child(die, &child, &err);
            if(ret == DW_DLV_ERROR){
                return -1;
            }if(ret == DW_DLV_OK){
                types = g_hash_table_new((GHashFunc)g_int_hash,
                                         (GCompareFunc)g_int_equal);
                types_add(dbg, child, types);
                prototype_add(dbg, child, types, functions);
                //types_print(types);
                types_free(types);
                g_hash_table_destroy(types);
            }
        }
        ret = dwarf_siblingof(dbg, die, &die, &err);
        if(ret == DW_DLV_NO_ENTRY){
            break;
        }else if(ret != DW_DLV_OK){
            return -1;
        }
    }
    return 0;
}

int prototype_add(Dwarf_Debug dbg, Dwarf_Die die,
                  GHashTable *types, GHashTable *functions){
    int ret;
    Dwarf_Error err;
    Dwarf_Half tag;
    Dwarf_Die child;
    Dwarf_Attribute attr;
    char *name;
    Dwarf_Addr addr;
    function_t *func;

    while(1){
        ret = dwarf_tag(die, &tag, &err);
        if(ret != DW_DLV_OK){
            return -1;
        }   
        if(tag == DW_TAG_subprogram){
            ret = dwarf_attr(die, DW_AT_name, &attr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_formstring(attr, &name, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_attr(die, DW_AT_low_pc, &attr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_formaddr(attr, &addr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }

            func = malloc(sizeof(function_t));
            if(!func){
                return -2;
            }
            memset(func, 0, sizeof(function_t));
            func->addr = (void*)addr;
            func->name = strdup(name);
            func->args = NULL;
            
            ret = dwarf_child(die, &child, &err);
            if(ret == DW_DLV_OK){
                prototype_add_args(dbg, child, types, &func->args);
            }else if(ret == DW_DLV_ERROR){
                return -1;
            }
            g_hash_table_insert(functions, &(func->addr), func);
        }

        ret = dwarf_siblingof(dbg, die, &die, &err);
        if(ret == DW_DLV_NO_ENTRY){
            break;
        }else if(ret != DW_DLV_OK){
            return -1;
        }
    }
    return 0;
}

int prototype_typename2int(const char *name){
    if(!strcmp(name, "pointer")){
        return TYPE_POINTER;
    }else if(!strcmp(name, "int")){
        return TYPE_INT;
    }else if(!strcmp(name, "unsigned int")){
        return TYPE_UINT;
    }else if(!strcmp(name, "short int")){
        return TYPE_SHORT;
    }else if(!strcmp(name, "short unsigned int")){
        return TYPE_USHORT;
    }else if(!strcmp(name, "char")){
        return TYPE_CHAR;
    }else if(!strcmp(name, "unsigned char")){
        return TYPE_UCHAR;
    }else if(!strcmp(name, "long int")){
        return TYPE_LONG;
    }else if(!strcmp(name, "long unsigned int")){
        return TYPE_ULONG;
    }else if(!strcmp(name, "long long int")){
        return TYPE_LONGLONG;
    }else if(!strcmp(name, "long long unsigned int")){
        return TYPE_ULONGLONG;
    }else if(!strcmp(name, "size_t")){
        return TYPE_SIZE_T;
    }else if(!strcmp(name, "__off_t")){
        return TYPE_OFF_T;
    }else if(!strcmp(name, "__off64_t")){
        return TYPE_OFF64_T;
    }

    return -1;
}

int prototype_add_args(Dwarf_Debug dbg, Dwarf_Die die,
                       GHashTable *types, GSList **args){
    int ret;
    Dwarf_Error err;
    Dwarf_Half tag;
    Dwarf_Attribute attr;
    Dwarf_Unsigned size;
    Dwarf_Off off;
    int type;
    char *name;
    arg_t *arg;
    type_t *t;

    while(1){
        ret = dwarf_tag(die, &tag, &err);
        if(ret != DW_DLV_OK){
            return -1;
        }

        if(tag == DW_TAG_formal_parameter){
            ret = dwarf_attr(die, DW_AT_name, &attr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_formstring(attr, &name, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_attr(die, DW_AT_type, &attr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_formref(attr, &off, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            type = off;

            arg = malloc(sizeof(arg_t));
            if(!arg){
                return -2;
            }
            memset(arg, 0, sizeof(arg_t));
            arg->name = strdup(name);
            //printf("type=%lld, name=%s\n", type, name);
            t = (type_t*)g_hash_table_lookup(types, &type);
            if(t){
                arg->type = prototype_typename2int(t->name);
                arg->typename = strdup(t->name);
                arg->size = t->size;
            }else{
                arg->type = -1;
                arg->typename = strdup("unknown");
                arg->size = 0;
            }
            *args = g_slist_append (*args, arg);
        }

        ret = dwarf_siblingof(dbg, die, &die, &err);
        if(ret == DW_DLV_NO_ENTRY){
            break;
        }else if(ret != DW_DLV_OK){
            return -1;
        }
    }
    return 0;
}

static gboolean prototype_print_entry(gpointer key, gpointer value,
                                      gpointer user_data)
{
    int i;
    function_t *func = value;
    GSList *list;
    arg_t *arg;
    printf("0x%x: %s(", *(guint*)key, func->name);
    
    list = func->args;
    while(list){
        arg = list->data;
        printf("%s %s", arg->typename, arg->name);
        list = g_slist_next(list);
        if(list){
            printf(", ");
        }
    }

    printf(")\n");
    return 0;
}

void prototype_print(GHashTable *functions)
{
    if(!functions){
        return;
    }
    g_hash_table_foreach(functions, (GHFunc)prototype_print_entry, NULL);
}

int types_add(Dwarf_Debug dbg, Dwarf_Die die, GHashTable *types){
    int ret;
    Dwarf_Error err;
    Dwarf_Half tag;
    Dwarf_Attribute attr;
    Dwarf_Unsigned size, encoding;
    Dwarf_Off offset;
    char *name;
    type_t *type;

    while(1){
        ret = dwarf_tag(die, &tag, &err);
        if(ret != DW_DLV_OK){
            return -1;
        }

        if(tag == DW_TAG_base_type){
            ret = dwarf_die_CU_offset(die, &offset, &err);
            ret = dwarf_attr(die, DW_AT_name, &attr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_formstring(attr, &name, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_attr(die, DW_AT_byte_size, &attr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_formsdata(attr, &size, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_attr(die, DW_AT_encoding, &attr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_formsdata(attr, &encoding, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }

            type = (type_t*)malloc(sizeof(type_t));
            type->offset = offset;
            type->name = strdup(name);
            type->size = size;
            g_hash_table_insert(types, &(type->offset), type);
        }else if(tag == DW_TAG_pointer_type){
            ret = dwarf_die_CU_offset(die, &offset, &err);
            ret = dwarf_attr(die, DW_AT_byte_size, &attr, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            ret = dwarf_formsdata(attr, &size, &err);
            if(ret != DW_DLV_OK){
                return -1;
            }
            type = (type_t*)malloc(sizeof(type_t));
            type->offset = offset;
            type->name = strdup("pointer");
            type->size = size;
            g_hash_table_insert(types, &(type->offset), type);
        }

        ret = dwarf_siblingof(dbg, die, &die, &err);
        if(ret == DW_DLV_NO_ENTRY){
            break;
        }else if(ret != DW_DLV_OK){
            return -1;
        }
    }
    return 0;
}

static gboolean types_free_entry(gpointer key, gpointer value,
                                       gpointer user_data)
{
    type_t *type = value;
    GSList *list;

    if(!type){
        return 0;
    }
    if(type->name){
        free(type->name);
    }

    free(type);
    return 0;
}

void types_free(GHashTable *types)
{
    if(!types){
        return;
    }
    g_hash_table_foreach(types, (GHFunc)types_free_entry, NULL);
}

static gboolean types_print_entry(gpointer key, gpointer value,
                                  gpointer user_data)
{
    int i;
    type_t *type = value;
    printf("%d: name=%s, size=%d\n", *(guint*)key, type->name, type->size);
    return 0;
}

void types_print(GHashTable *types)
{
    if(!types){
        return;
    }
    g_hash_table_foreach(types, (GHFunc)types_print_entry, NULL);
}
