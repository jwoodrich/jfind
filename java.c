#include "java.h"
#include "binfile.h"
#include "util.h"
#include <stdlib.h>
#include <errno.h>
#define RORZ(X,...) if (!X) { __VA_ARGS__ return 0; }
#define LEFT_MASK1 0x80
#define LEFT_MASK2 0xc0
#define LEFT_MASK3 0xe0
#define LEFT_MASK4 0xf0
#define JUTF8_PFX_ASCII 0x0
#define JUTF8_PFX_2BYTE1 0xc0
#define JUTF8_PFX_2BYTE2 0x80
#define JUTF8_PFX_3BYTE1 0xe0
#define JUTF8_PFX_3BYTE2 0x80
#define JUTF8_PFX_3BYTE3 0X80
#define JUTF8_PFX_6BYTE1 0xed
#define JUTF8_PFX_6BYTE2 0xa0
#define JUTF8_PFX_6BYTE3 0x80
#define JUTF8_PFX_6BYTE4 0xed
#define JUTF8_PFX_6BYTE5 0xb0
#define JUTF8_PFX_6BYTE6 0x80
#define IS_JUTF8_ASCII(B,I) ((B[I]&LEFT_MASK1)==JUTF8_PFX_ASCII)
#define IS_JUTF8_2BYTE(B,I,L) ((B[I]&LEFT_MASK3)==JUTF8_PFX_2BYTE1 && \
                        (I+1)<L && \
                        (B[I+1]&LEFT_MASK2)==JUTF8_PFX_2BYTE2)
#define IS_JUTF8_3BYTE(B,I,L) ((B[I]&LEFT_MASK4)==JUTF8_PFX_3BYTE1 && \
                        (I+2)<L && \
                        (B[I+1]&LEFT_MASK2)==JUTF8_PFX_3BYTE2 && \
                        (B[I+2]&LEFT_MASK2)==JUTF8_PFX_3BYTE3)
#define IS_JUTF8_SUPPLEMENTARY(B,I,L) (B[I]==JUTF8_PFX_6BYTE1 && \
                        (I+5)<L && \
                        (B[I+1]&LEFT_MASK4)==JUTF8_PFX_6BYTE2 && \
                        (B[I+2]&LEFT_MASK2)==JUTF8_PFX_6BYTE3 && \
                        B[I+3]==JUTF8_PFX_6BYTE4 && \
                        (B[I+4]&LEFT_MASK4)==JUTF8_PFX_6BYTE5 && \
                        (B[I+5]&LEFT_MASK2)==JUTF8_PFX_6BYTE6)
#define JUTF8_SIZE(B,I,L) (IS_JUTF8_ASCII(B,I)?1:\
                             IS_JUTF8_SUPPLEMENTARY(B,I,L)?6:\
                             IS_JUTF8_3BYTE(B,I,L)?3:\
                             IS_JUTF8_2BYTE(B,I,L)?2:-1)

int read_cp_info(FILE *fp, char *filename, struct cp_info *cp);

int read_class(FILE *fp, struct class_file *clazz) {
  RORZ(READU2(fp,clazz->minor_version));
  RORZ(READU2(fp,clazz->major_version));
  RORZ(READU2(fp,clazz->constant_pool_count));
  
  return 1;
}

int free_class(struct class_file *clazz) {
  
}

int jutf8_strlen(u2 length, u1 *bytes) {
  int i,size;
  DEBUGOUT("jutf8_strlen(%d,%p)\n",length,bytes);
  for (i=0;i<length;) {
    size=JUTF8_SIZE(bytes,i,length);
    if (size<0) {
      errno=EINVAL;
      return -1;
    }
    i+=size;
  }
  return i;
}

char *jutf8_strncpy(char *dest, u2 length, u1 *source, int cslen) {
  int size=0,i,di=0;
  for (i=0;di<cslen&&i<length;i+=size) {
    size=JUTF8_SIZE(source,i,length);
    if (size==1) {
      dest[di++]=source[i];
    }
    else if (size>1) {
      dest[di++]=JUTF8_UNICODE_CHAR;
    }
    else {
      errno=EINVAL;
      return (char*)NULL;
    }
  }
  // add null terminator
  if (di<cslen) { dest[di]=(char)NULL; }
  else { dest[cslen-1]=(char)NULL; }

  return dest;
}

char *jutf8_strcpy(char *dest, u2 length, u1 *source) {
  int cslen,size,ridx=0;
  cslen=jutf8_strlen(length,source);
  return jutf8_strncpy(dest,length,source,cslen);
}

char *jutf8_strdup(u2 length, u1 *bytes) {
  int cslen,size,ridx=0;
  char *ret;
  cslen=jutf8_strlen(length,bytes);
  if (cslen<0) { return (char*)NULL; }
  ret=CALLOC("jutf8_strdup",cslen+1,sizeof(char));
  return jutf8_strncpy(ret,length,bytes,cslen+1);
}

int read_class_summary(FILE *fp, struct class_summary *clazz) {
  unsigned int constant_pool_count,i,j;
  unsigned short access_flags,this_class,name_index;
  struct cp_info **cp;
  struct CONSTANT_Class_info ci;
  
  RORZ(READU2(fp,clazz->minor_version));
  RORZ(READU2(fp,clazz->major_version));
  RORZ(READU2(fp,constant_pool_count));
  if (constant_pool_count==0) {
    fprintf(stderr,"invalid constant pool count %d!\n",constant_pool_count);
    errno=EINVAL;
    return 0;
  }
  constant_pool_count--;
  cp=CALLOC("read_class_summary cp_info",constant_pool_count,sizeof(struct cp_info *));
  printf("\n---\n");
  DEBUGOUT("- version is %d.%d\n",clazz->major_version,clazz->minor_version);

  for (i=0;i<constant_pool_count;i++) {
    DEBUGOUT("- loading constant_pool[%d] of %d\n",i,constant_pool_count);
    cp[i]=CALLOC("read_class_summary cp_info entry",1,sizeof(struct cp_info));
    if (!read_cp_info(fp,clazz->filename,cp[i])) {
      fprintf(stderr,"failed to read cp_info at index %d for file %s (v%d.%d): %s\n",i,clazz->filename,clazz->major_version,clazz->minor_version,strerror(errno));
      for (j=0;j<=i;j++) { free(cp[j]); }
      free(cp);
      errno=EINVAL;
      return 0;
    }
  }
  RORZ(READU2(fp,access_flags),free_cp_infos(constant_pool_count,cp););
  RORZ(READU2(fp,this_class),free_cp_infos(constant_pool_count,cp););
  this_class--;
  if (this_class>=constant_pool_count) {
    fprintf(stderr,"this_class constant index reference %d is greater than constant pool size of %d!\n",this_class,constant_pool_count);
    free_cp_infos(constant_pool_count,cp);
    errno=EINVAL;
    return 0;
  }
  name_index=((struct CONSTANT_Class_info *)cp[this_class]->info)->name_index;
  name_index--;
  DEBUGOUT("- name index is %u\n",name_index);
  if (name_index>=constant_pool_count) {
    fprintf(stderr,"name_index constant index reference %d is greater than constant pool size of %d!\n",name_index,constant_pool_count);
    free_cp_infos(constant_pool_count,cp);
    errno=EINVAL;
    return 0;
  }
  
  clazz->name=jutf8_strdup(((struct CONSTANT_Utf8_info *)cp[name_index]->info)->length,
                           ((struct CONSTANT_Utf8_info *)cp[name_index]->info)->bytes);
  free_cp_infos(constant_pool_count,cp);
  return 1;
  
}

int free_class_summary(struct class_summary *clazz) {
  free(clazz->name);
  free(clazz);
}

void free_cp_infos(unsigned int count, struct cp_info **cp) {
  unsigned int i;
  for (i=0;i<count;i++) {
    free_cp_info(cp[i]);
  }
  free(cp);
}

void free_cp_info(struct cp_info *cp) {
  switch (cp->tag) {
    case CONSTANT_Utf8:
      free(((struct CONSTANT_Utf8_info *)cp->info)->bytes);
    default:
      free(cp->info);
  }
}

int read_cp_info(FILE *fp, char *filename, struct cp_info *cp) {
  unsigned short i,max;
  char *tmp;
  DEBUGOUT("read_cp_info(%p,%p)\n",fp,cp);
  RORZ(READU1(fp,cp->tag));
  DEBUGOUT("-- cp->tag=%u\n",cp->tag);
  switch (cp->tag) {
    case CONSTANT_Class:
      cp->info=CALLOC("cp->info for CONSTANT_Class",1,sizeof(struct CONSTANT_Class_info));
      RORZ(READU2(fp,((struct CONSTANT_Class_info *)cp->info)->name_index))
      break;
    case CONSTANT_Fieldref:
      cp->info=CALLOC("cp->info for CONSTANT_Fieldref",1,sizeof(struct CONSTANT_Fieldref_info));
      RORZ(READU2(fp,((struct CONSTANT_Fieldref_info *)cp->info)->class_index))
      RORZ(READU2(fp,((struct CONSTANT_Fieldref_info *)cp->info)->name_and_type_index))
      break;
    case CONSTANT_Methodref:
      cp->info=CALLOC("cp->info for CONSTANT_Methodref",1,sizeof(struct CONSTANT_Methodref_info));
      RORZ(READU2(fp,((struct CONSTANT_Methodref_info *)cp->info)->class_index))
      RORZ(READU2(fp,((struct CONSTANT_Methodref_info *)cp->info)->name_and_type_index))
      break;
    case CONSTANT_InterfaceMethodref:
      cp->info=CALLOC("cp->info for CONSTANT_InterfaceMethodref",1,sizeof(struct CONSTANT_InterfaceMethodref_info));
      RORZ(READU2(fp,((struct CONSTANT_InterfaceMethodref_info *)cp->info)->class_index))
      RORZ(READU2(fp,((struct CONSTANT_InterfaceMethodref_info *)cp->info)->name_and_type_index))
      break;
    case CONSTANT_String:
      cp->info=CALLOC("cp->info for CONSTANT_String",1,sizeof(struct CONSTANT_String_info));
      RORZ(READU2(fp,((struct CONSTANT_String_info *)cp->info)->string_index))
      break;
    case CONSTANT_Integer:
      cp->info=CALLOC("cp->info for CONSTANT_Integer",1,sizeof(struct CONSTANT_Integer_info));
      RORZ(READU4(fp,((struct CONSTANT_Integer_info *)cp->info)->bytes))
      break;
    case CONSTANT_Float:
      cp->info=CALLOC("cp->info for CONSTANT_Float",1,sizeof(struct CONSTANT_Float_info));
      RORZ(READU4(fp,((struct CONSTANT_Float_info *)cp->info)->bytes))
      break;
    case CONSTANT_Long:
      cp->info=CALLOC("cp->info for CONSTANT_Long",1,sizeof(struct CONSTANT_Long_info));
      RORZ(READU4(fp,((struct CONSTANT_Long_info *)cp->info)->high_bytes))
      RORZ(READU4(fp,((struct CONSTANT_Long_info *)cp->info)->low_bytes))
      break;
    case CONSTANT_Double:
      cp->info=CALLOC("cp->info for CONSTANT_Double",1,sizeof(struct CONSTANT_Double_info));
      RORZ(READU4(fp,((struct CONSTANT_Double_info *)cp->info)->high_bytes))
      RORZ(READU4(fp,((struct CONSTANT_Double_info *)cp->info)->low_bytes))
      break;
    case CONSTANT_NameAndType:
      cp->info=CALLOC("cp->info for CONSTANT_NameAndType",1,sizeof(struct CONSTANT_NameAndType_info));
      RORZ(READU2(fp,((struct CONSTANT_NameAndType_info *)cp->info)->name_index))
      RORZ(READU2(fp,((struct CONSTANT_NameAndType_info *)cp->info)->descriptor_index))
      break;
    case CONSTANT_Utf8:
      cp->info=CALLOC("cp->info for CONSTANT_Utf8",1,sizeof(struct CONSTANT_Utf8_info));
      RORZ(READU2(fp,max));
      ((struct CONSTANT_Utf8_info *)cp->info)->length=max;
      ((struct CONSTANT_Utf8_info *)cp->info)->bytes=CALLOC("cp->info->bytes for CONSTANT_Utf8",((struct CONSTANT_Utf8_info *)cp->info)->length,sizeof(u1));
      DEBUGOUT("-- read CONSTANT_Utf8 of size %d into %p\n",max,((struct CONSTANT_Utf8_info *)cp->info)->bytes);
      for (i=0;i<max;i++) {
        RORZ(READU1(fp,((struct CONSTANT_Utf8_info *)cp->info)->bytes[i]));
      }
#ifdef DEBUG
      tmp=jutf8_strdup(max,((struct CONSTANT_Utf8_info *)cp->info)->bytes);
      DEBUGOUT("----> value=",NULL);
      if (tmp!=NULL) {
        DEBUGOUT("\"%s\"\n",tmp);
      } else {
        DEBUGOUT("null\n",NULL);
      }
      free(tmp);
#endif
      break;
    case CONSTANT_MethodHandle:
      cp->info=CALLOC("cp->info for CONSTANT_MethodHandle",1,sizeof(struct CONSTANT_MethodHandle_info));
      RORZ(READU1(fp,((struct CONSTANT_MethodHandle_info *)cp->info)->reference_kind));
      RORZ(READU2(fp,((struct CONSTANT_MethodHandle_info *)cp->info)->reference_index));
      break;
    case CONSTANT_MethodType:
      cp->info=CALLOC("cp->info for CONSTANT_MethodType",1,sizeof(struct CONSTANT_MethodType_info));
      RORZ(READU2(fp,((struct CONSTANT_MethodType_info *)cp->info)->descriptor_index));
      break;
    case CONSTANT_InvokeDynamic:
      cp->info=CALLOC("cp->info for CONSTANT_InvokeDynamic",1,sizeof(struct CONSTANT_InvokeDynamic_info));
      RORZ(READU2(fp,((struct CONSTANT_InvokeDynamic_info *)cp->info)->bootstrap_method_attr_index));
      RORZ(READU2(fp,((struct CONSTANT_InvokeDynamic_info *)cp->info)->name_and_type_index));
      break;
    default:
      fprintf(stderr,"unknown tag in class file %s: %u\n",filename,cp->tag);
      errno=EINVAL;
      return 0;
  }
  return 1;
}


