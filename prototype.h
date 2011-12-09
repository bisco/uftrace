/*
 * prototype.h
 */

#define TYPE_UNKNOWN   -1
#define TYPE_POINTER    0
#define TYPE_INT        1
#define TYPE_UINT       2
#define TYPE_SHORT      3
#define TYPE_USHORT     4
#define TYPE_CHAR       5
#define TYPE_UCHAR      6
#define TYPE_LONG       7
#define TYPE_ULONG      8
#define TYPE_LONGLONG   9
#define TYPE_ULONGLONG  10
#define TYPE_SIZE_T     11
#define TYPE_OFF_T      12
#define TYPE_OFF64_T    13

typedef struct{
    char *name;
    int type;
    char *typename;
    size_t size;
}arg_t;

typedef struct{
    off_t offset;
    char *name;
    size_t size;
    //char encoding;
}type_t;

typedef struct{
    void *addr;
    unsigned char *name;
    GSList *args;
}function_t;

