#include "filesys.h"

#include <png.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

// C lets you redefine stuff... right?
#define LOGDBG(f_, ...)

//Taken verbatim from https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    const size_t len = strlen(path);
    char _path[PATH_MAX];
    char *p; 

    errno = 0;

    /* Copy string so its mutable */
    if (len > PATH_MAX-1) {
        errno = ENAMETOOLONG;
        LOGDBG("MKDIR_P: DIRECTORY TOO LONG: %d, %s\n", PATH_MAX, path);
        return -1; 
    }   
    strcpy(_path, path);

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (mkdir(_path, S_IRWXU) != 0 && errno != EEXIST) {
               LOGDBG("MKDIR_P: Couldn't make inner path: %s\n", _path);
               return -1; 
            }

            *p = '/';
        }
    }   

    if (mkdir(_path, S_IRWXU) != 0 && errno != EEXIST) {
       LOGDBG("MKDIR_P: Couldn't make final path: %s\n", _path);
       return -1; 
    }   

    LOGDBG("Created directory: %s\n", path);

    return 0;
}

//Taken from https://stackoverflow.com/a/230070/1066474
bool file_exists (char * filename)
{
   struct stat buffer;
   return (stat (filename, &buffer) == 0);
}

//Get directories in menu format (separated by \0, last item has 2 \0)
s32 get_directories(char * directory, char * container, u32 c_size)
{
   s32 count = 0;
   char * current_file = container;
   DIR * dir = opendir(directory);

   if(!dir) 
   {
      LOGDBG("ERR: Couldn't open dir %s\n", directory);
      return -1;
   }

   struct dirent * entry = readdir(dir);

   while(entry != NULL)
   {
      if(entry->d_type == DT_DIR)
      {
         u32 len = strlen(entry->d_name);

         //No more files will fit
         if(current_file - container + len + 2 > c_size)
         {
            LOGDBG("WARN: No more room in directory container for %s", directory);
            break;
         }

         //Copy entry into just past the last slot (where the 0 is)
         memcpy(current_file, entry->d_name, len);
         current_file += len;
         *current_file = 0;
         current_file++;

         count++;
      }

      entry = readdir(dir);
   }

   *current_file = 0;

   return count;
}

int write_file(const char * filename, const char * data)
{
   int result = 0;
   FILE * savefile = fopen(filename, "w");
   if(!savefile)
   {
      LOGDBG("ERR: Couldn't open file %s", filename);
      return -1;
   }
   else if(fputs(data, savefile) == EOF)
   {
      LOGDBG("ERR: Couldn't write data to %s", filename);
      result = -2;
   }
   fclose(savefile);
   return result;
}

char * read_file(const char * filename, char * container, u32 maxread)
{
   char * result = NULL;
   FILE * loadfile = fopen(filename, "r");
   if(loadfile == NULL)
   {
      LOGDBG("ERR: Couldn't open file %s", filename);
      return NULL;
   }
   else
   {
      result = fgets(container, maxread, loadfile);
      if(result == NULL)
      {
         LOGDBG("ERR: Couldn't read file %s", filename);
      }
      else
      {
         result = container + strlen(container);
      }
   }

   fclose(loadfile);
   return result;
}

#define MYPNG_FORMAT PNG_COLOR_TYPE_RGBA
#define MYPNG_BYTESPER 4

//Given a citro-formatted array of u32 colors, write a png to the given 
//location. Rawdata needs to be linear and row first. The 

//No, the data must be in a linear array, row first, of PNG formatted RGBA
//bytes. Ironically, citro-formatted colors on the 3ds are already like this,
//as the 3ds is little endian just like png
int write_citropng(u32 * rawdata, u16 width, u16 height, char * filepath)
{
   //A lot of this is taken from http://www.labbookpages.co.uk/software/imgProc/libPNG.html
   int code = 0;
   FILE *fp = NULL;
   png_structp png_ptr = NULL;
   png_infop info_ptr = NULL;

   // Open file for writing (binary mode)
   fp = fopen(filepath, "wb");
   if (fp == NULL) {
      LOGDBG("ERR: Couldn't open png for writing: %s\n", filepath);
      code = 1;
      goto finalise;
   }

   // Initialize write structure
   png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (png_ptr == NULL) {
      LOGDBG("ERR: Could not allocate png write struct\n");
      code = 1;
      goto finalise;
   }

   // Initialize info structure
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      LOGDBG("ERR: Could not allocate png info struct\n");
      code = 1;
      goto finalise;
   }

   // Setup Exception handling
   if (setjmp(png_jmpbuf(png_ptr))) {
      LOGDBG("General error during png creation\n");
      code = 1;
      goto finalise;
   }

   //Use the filepointer we got before for png io
   png_init_io(png_ptr, fp);

   // Write header (8 bit colour depth)
   png_set_IHDR(png_ptr, info_ptr, width, height,
         8, MYPNG_FORMAT, PNG_INTERLACE_NONE,
         PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

   png_write_info(png_ptr, info_ptr);

   // Write image data
   for (u32 y=0 ; y<height ; y++) {
      png_write_row(png_ptr, (png_bytep)(rawdata + y * width));
   }

   // End write
   png_write_end(png_ptr, NULL);

finalise:
   if (fp != NULL) fclose(fp);
   if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
   if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

   return code;

}