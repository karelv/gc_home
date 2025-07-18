#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>

#include <SD.h>

void print_directory(File dir, int numSpaces);
void copy_directory_to_little_fs(File src, const char *dest_root);
void copy_file_to_little_fs(const char *filename);
void copy_file_to_SD(const char *filename);

#endif // __UTILS_H__