#include <LittleFS.h>               // LittleFS 

#include "utils.h"

extern LittleFS_Program g_little_fs;


static void print_spaces(int num) 
{
  for (int i=0; i < num; i++) {
    Serial.print(" ");
  }
}


static void print_time(const DateTimeFields tm) 
{
  const char *months[12] = {
    "January","February","March","April","May","June",
    "July","August","September","October","November","December"
  };
  if (tm.hour < 10) Serial.print('0');
  Serial.print(tm.hour);
  Serial.print(':');
  if (tm.min < 10) Serial.print('0');
  Serial.print(tm.min);
  Serial.print("  ");
  Serial.print(tm.mon < 12 ? months[tm.mon] : "???");
  Serial.print(" ");
  Serial.print(tm.mday);
  Serial.print(", ");
  Serial.print(tm.year + 1900);
}


void print_directory(File dir, int numSpaces) 
{
   while(true) {
     File entry = dir.openNextFile();
     if (! entry) {
       //Serial.println("** no more files **");
       break;
     }
     yield();

     print_spaces(numSpaces);
     Serial.printf("'%s'", entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       print_directory(entry, numSpaces+2);
     } else {
       // files have sizes, directories do not
       int n = log10f(entry.size());
       yield();
       if (n < 0) n = 10;
       if (n > 10) n = 10;
       print_spaces(50 - numSpaces - strlen(entry.name()) - n);
       Serial.print("  ");
       Serial.print(entry.size(), DEC);
       yield();
       DateTimeFields datetime;
       if (entry.getModifyTime(datetime)) {
         print_spaces(4);
         print_time(datetime);
       }
       yield();
       Serial.println();
     }
     yield();
     entry.close();
   }
}


void 
copy_directory_to_little_fs(File src, const char *dest_root)
{
  uint32_t stop, start = micros();
  while(true) {
    File entry = src.openNextFile();
    if (! entry) {
      //Serial.println("** no more files **");
      break;
    }
    Serial.printf("copy: '%s'\n", entry.name());
    if (entry.isDirectory())
    {
      char dirname[128];
      strcpy(dirname, dest_root);
      strcat(dirname, "/");
      strcat(dirname, entry.name());
      Serial.printf("copy: mkdir (FLASH) '%s'\n", dirname);
      g_little_fs.mkdir(dirname);
      yield();
      copy_directory_to_little_fs(entry, dirname);
    } else
    {
      char filename[128];
      strcpy(filename, dest_root);
      strcat(filename, "/");
      strcat(filename, entry.name());
      Serial.printf("copy to '%s'\n", filename);
      if (g_little_fs.exists(filename))
      {
        g_little_fs.remove(filename);
      }
      yield();
      File dest = g_little_fs.open(filename, FILE_WRITE);
      
      uint32_t counter = 0;
      while (entry.available())
      {
        counter++;
        dest.write(entry.read());
        if (counter % 128)
        {
          stop = micros();
          if ((stop - start) > 2000)
          {
            yield();
            start = stop;
          }
        }
      }
      yield();
      dest.close();
      entry.close();
    }
  }
}


void copy_file_to_little_fs(const char *filename)
{
  uint32_t stop, start = micros();
  if (!SD.exists(filename)) return;

  File src = SD.open(filename);

  if (g_little_fs.exists(filename))
  {
    g_little_fs.remove(filename);
  }
  yield();
  File dest = g_little_fs.open(filename, FILE_WRITE);
  
  uint32_t counter = 0;
  while (src.available())
  {
    counter++;
    dest.write(src.read());
    if (counter % 128)
    {
      stop = micros();
      if ((stop - start) > 2000)
      {
        yield();
        start = stop;
      }
    }
  }
  dest.close();
  src.close();
}

void copy_file_to_SD(const char *filename)
{
  uint32_t stop, start = micros();
  if (!g_little_fs.exists(filename)) return;

  File src = g_little_fs.open(filename);

  if (SD.exists(filename))
  {
    SD.remove(filename);
  }
  yield();
  File dest = SD.open(filename, FILE_WRITE);

  uint32_t counter = 0;
  while (src.available())
  {
    counter++;
    dest.write(src.read());
    if (counter % 128)
    {
      stop = micros();
      if ((stop - start) > 2000)
      {
        yield();
        start = stop;
      }
    }
  }
  dest.close();
  src.close();
}
