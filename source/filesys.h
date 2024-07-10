#ifndef __HEADER_FILESYS
#define __HEADER_FILESYS

#include <3ds.h>

int mkdir_p(const char *path);
bool file_exists (char * filename);
s32 get_directories(char * directory, char * container, u32 c_size);
char * read_file(const char * filename, char * container, u32 maxread);
int write_file(const char * filename, const char * data);
int write_citropng(u32 * rawdata, u16 width, u16 height, char * filepath);

#endif