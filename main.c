#include <stdio.h>
#include <stdlib.h>
#include <zip.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fnmatch.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <fcntl.h>
#define _XOPEN_SOURCE 500
#define __USE_XOPEN_EXTENDED
#include <ftw.h>
#include <unistd.h>
#include "util.h"
#include "java.h"
#include "binfile.h"
#define MIN_JAR_SIZE 4
#define CLASS_MAGIC_NUMBER 0xCAFEBABE
#define ZIP_MAGIC_NUMBER 0x504b0304
#define CLASS_EXTENSION ".class"
#define EAR_EXTENSION ".ear"
#define WAR_EXTENSION ".war"
#define JAR_EXTENSION ".jar"
#define FNBUFSIZE 1024

static char error_buf[100];

void print_usage(char *path) {
  printf("usage: %s <base path> <class name>\n",path);
}

int process(char *pathlabel, char *searchstring, char *path) {
  struct stat sb;
  char *base;
  char *subpath;
  int ret=0;
  DEBUGOUT("process(%s,%s,%s)\n",pathlabel,searchstring,path);
  if (subpath=strchr(path,'!')) {
    int basesize=(int)(subpath-path);
    if (!(base=malloc(subpath-path))) {
      fprintf(stderr,"unable to malloc %d bytes for base path of %s\n",subpath-path,path);
      exit(2);
    }
    strncpy(base,path,basesize);
    subpath++;
  } else {
    base=path;
  }
  if (stat(base,&sb) == -1) {
    fprintf(stderr,"unable to stat %s!\n",base);
    if (base!=path) { free(base); }
    return;
  }
  if (S_ISDIR(sb.st_mode)) {
    DIR *dir;
    DEBUGOUT("- processing directory %s\n",base);
    if (!(dir=opendir(base))) {
      fprintf(stderr,"failed to open directory %s: %s\n",base,strerror(errno));
    } else {
      struct dirent *dentry;
      while (dentry=readdir(dir)) {
        char *newlabel, *newpath;
        int newpath_size, newlabel_size;
        if (strcmp(dentry->d_name,".")==0||strcmp(dentry->d_name,"..")==0) {
          continue;
        }
        newpath_size=strlen(base)+strlen(dentry->d_name)+2;
        newlabel_size=strlen(pathlabel)+strlen(dentry->d_name)+2;
        if ((newpath=calloc(newpath_size,sizeof(char))) && (newlabel=calloc(newlabel_size,sizeof(char)))) {
          strcpy(newpath,base);
          strcat(newpath,"/");
          strcat(newpath,dentry->d_name);
          strcpy(newlabel,pathlabel);
          strcat(newlabel,"/");
          strcat(newlabel,dentry->d_name);
          ret+=process(newlabel,searchstring,newpath);
          free(newpath);
        } else {
          fprintf(stderr,"failed to allocate %d bytes for path %s/%s!\n",newpath_size,base,dentry->d_name);
          exit(2);
        }
      }
      closedir(dir);
    }
  } else if (S_ISREG(sb.st_mode)) {
    char *filename=basename(base);
    DEBUGOUT("- processing file %s\n",base);
    if (sb.st_size>4) {
      FILE *fp=fopen(base,"r");
      if (fp) {
        unsigned int magic;
        if (READU4(fp,magic)) {
          DEBUGOUT("magic=%x\n",magic);
          if (magic==CLASS_MAGIC_NUMBER) {
            DEBUGOUT("%s - class file\n",base);
            struct class_summary clazz;
            clazz.filename=base;
            if (read_class_summary(fp,&clazz)) {
              DEBUGOUT("%s - version %u.%u\n",clazz.name,clazz.major_version,clazz.minor_version);
              if (fnmatch(searchstring,clazz.name,0)==0) {
                printf("%s",base);
                printf("\n");
                ret++;
              }
            }
          } else if (magic==ZIP_MAGIC_NUMBER) {
            struct zip *za;
            int err;
            if ((za = zip_open(base, 0, &err)) == NULL) {
              zip_error_to_str(error_buf, sizeof(error_buf), err, errno);
              fprintf(stderr, "%s: can't open zip archive `%s': %s/n", base, error_buf);
            } else {
              char tmpdir[L_tmpnam];
              if (unzip_tmp(&tmpdir,za)) {
                char *newpath;
                int newpath_size;
                newpath_size=strlen(pathlabel)+2;
                newpath=CALLOC("new path with zip",newpath_size,sizeof(char));
                strcpy(newpath,pathlabel);
                strcat(newpath,"!");
                ret+=process(newpath,searchstring,tmpdir);
                free(newpath);
                rm_tempdir(tmpdir);
              }
            }
            printf("%s - zip file\n",base);
          }
        } else {
          fprintf(stderr,"Failed to read from %s: %s\n",base,strerror(errno));
        }
        fclose(fp);
      } else {
        fprintf(stderr,"Failed to open %s: %s\n",base,strerror(errno));
      }
    }
  } else {
    fprintf(stderr,"Unhandled file type for %s!",base);
  }
  if (base!=path) { free(base); }
  return ret;
}
int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv) {
        perror(fpath);
    }
    return rv;
}
int rm_tempdir(char *tmpdir) {
  return nftw(tmpdir, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}
int unzip_tmp(char *tmpdir,struct zip *za) {
  struct zip_stat sb;
  struct zip_file *zf;
  int entries,fd,i,bufsize=FNBUFSIZE,len;
  long long sum;
  char *fpath=CALLOC("temporary zip file path",bufsize,sizeof(char));
  char buf[100];
  
  if (tmpnam(tmpdir)==NULL) {
    fprintf(stderr,"unable to generate temporary directory name!\n");
    return 0;
  }
  if (mkdir(tmpdir,0700)<0) {
    fprintf(stderr,"unable to create temporary directory %s!\n",tmpdir);
    return 0;
  }
  entries=zip_get_num_entries(za, 0); 
  for (i=0;i<entries;i++) {
    if (zip_stat_index(za, i, 0, &sb) == 0) {
      len = strlen(sb.name);
      snprintf(fpath,bufsize,"%s/%s",tmpdir,sb.name);
      if (sb.name[len - 1] == '/') {
        DEBUGOUT("- unzip creating directory %s\n",fpath);
        if (mkdir(fpath,0700)<0) {
          fprintf(stderr,"unable to create directory %s from zip\n",fpath);
          DEBUGOUT("- remove temporary directory %s ...\n",tmpdir);
          rm_tempdir(tmpdir);
          return 0;
        }
      } else {
        DEBUGOUT("- unzip creating file %s\n",fpath);
        if (!(zf=zip_fopen_index(za,i,0))) {
          fprintf(stderr,"unable to open file %s in zip at index %d!\n",&sb.name,i);
        } else if ((fd=open(fpath,O_RDWR | O_TRUNC | O_CREAT, 0600))<0) {
          fprintf(stderr,"unable to open file %s for writing!\n",fpath);
        } else {
          sum=0;
          while (sum!=sb.size) {
            if ((len=zip_fread(zf, buf, 100))<0) {
              fprintf(stderr,"failed to read %s from zip!\n",sb.name);
              break;
            } else {
              write(fd, buf, len);
              sum += len;
            }
          } // end while
          close(fd);
          zip_fclose(zf);
        } // end else file open is successful
      } // end else: is file
    } else {
      fprintf(stderr,"failed to state file %s at index %d!\n",sb.name,i);
    }
  }
  if (zip_close(za)==-1) {
    fprintf(stderr,"failed to close zip!\n");
  }
  return 1;
}

char *format_search(char *searchstring) {
  char* ret=strdup(searchstring);
  int i;
  if (ret==(char*)NULL) { return ret; }
  for (i=0;searchstring[i]!=(char)NULL;i++) { 
    ret[i]=searchstring[i]!='.'?searchstring[i]:'/';
  }
  return ret;
}

int main(int argc, char **argv) {
  char *basepath;
  char *searchstring;
  char *searchstring_converted;
  int matches=0;
  
  if (argc!=3) {
    print_usage(argv[0]);
    exit(1);
  }
  basepath=argv[1];
  searchstring=argv[2];
  searchstring_converted=format_search(searchstring);
  matches=process(basepath,searchstring_converted,basepath);
  free(searchstring_converted);
  if (matches==0) { return 1; }
  return 0;
}

