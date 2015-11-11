#ifndef _JAVA_H
#define _JAVA_H
#include <stdio.h>
#define CONSTANT_Class	7
#define CONSTANT_Fieldref	9
#define CONSTANT_Methodref	10
#define CONSTANT_InterfaceMethodref	11
#define CONSTANT_String	8
#define CONSTANT_Integer	3
#define CONSTANT_Float	4
#define CONSTANT_Long	5
#define CONSTANT_Double	6
#define CONSTANT_NameAndType	12
#define CONSTANT_Utf8	1
#define CONSTANT_MethodHandle	15
#define CONSTANT_MethodType	16
#define CONSTANT_InvokeDynamic	18
#define JUTF8_UNICODE_CHAR '_'
#define u1 unsigned char
#define u2 unsigned short
#define u4 unsigned int

struct cp_info {
  unsigned char tag;
  void* info;
};

struct class_summary {
  unsigned short minor_version;
  unsigned short major_version;
  char *name;
  char *filename;
};

struct class_file {
  unsigned int magic;
  unsigned short minor_version;
  unsigned short major_version;
  unsigned short constant_pool_count;
  struct cp_info *constant_pool;
  unsigned short access_flags;
  unsigned short this_class;
  unsigned short super_class;
  unsigned short interfaces_count;
  unsigned short *interfaces;
};

struct CONSTANT_Class_info {
    u2 name_index;
};

struct CONSTANT_Fieldref_info {
    u2 class_index;
    u2 name_and_type_index;
};

struct CONSTANT_Methodref_info {
    u2 class_index;
    u2 name_and_type_index;
};

struct CONSTANT_InterfaceMethodref_info {
    u2 class_index;
    u2 name_and_type_index;
};

struct CONSTANT_String_info {
    u2 string_index;
};

struct CONSTANT_Integer_info {
    u4 bytes;
};

struct CONSTANT_Float_info {
    u4 bytes;
};

struct CONSTANT_Long_info {
    u4 high_bytes;
    u4 low_bytes;
};

struct CONSTANT_Double_info {
    u4 high_bytes;
    u4 low_bytes;
};

struct CONSTANT_NameAndType_info {
    u2 name_index;
    u2 descriptor_index;
};

struct CONSTANT_Utf8_info {
    u2 length;
    u1 *bytes;
};

struct CONSTANT_MethodHandle_info {
    u1 reference_kind;
    u2 reference_index;
};

struct CONSTANT_MethodType_info {
    u2 descriptor_index;
};

struct CONSTANT_InvokeDynamic_info {
    u2 bootstrap_method_attr_index;
    u2 name_and_type_index;
};

char *jutf8_strncpy(char *dest, u2 length, u1 *source, int cslen);
char *jutf8_strcpy(char *dest, u2 length, u1 *source);
int jutf8_strlen(u2 length, u1 *bytes);
char *jutf8_strdup(u2 length, u1 *bytes);
int read_class(FILE *fp, struct class_file *clazz);
int free_class(struct class_file *clazz);
int read_class_summary(FILE *fp, struct class_summary *clazz);
int free_class_summary(struct class_summary *clazz);
void free_cp_infos(unsigned int count, struct cp_info **cp);
void free_cp_info(struct cp_info *cp);

#endif
