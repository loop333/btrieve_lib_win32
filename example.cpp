#include <stdio.h>
#include <conio.h>

#include <btrieve.h>

FIELDINFO Package_field[] =
{
 {"CRC",      BTR_TYPE_INTEGER,4,  0},
 {"PackageID",BTR_TYPE_INTEGER,4,  0},
 {"Brief",    BTR_TYPE_STRING, 20, 0},
 {"Name",     BTR_TYPE_STRING, 40, 0},
 {"Flag",     BTR_TYPE_INTEGER,2,  0},
 {"Version",  BTR_TYPE_STRING, 10, 0},
 {"Build",    BTR_TYPE_INTEGER,2,  0},
 {"Release",  BTR_TYPE_INTEGER,2,  0},
 {"HelpFile", BTR_TYPE_STRING, 20, 0},
 {"Comment",  BTR_TYPE_MEMO,   400,0},
 {0,0,0,0}
};

void main(void)
{
 clrscr();

 if (!Btrieve::loaded())
  {
   printf("Btrieve not loaded\n");
   return;
  }

 Btrieve::debug = 1;

 DAT package("C:\\WF\\DATA\\SYSTEM\\VOC\\package.dat");
 package.setField(Package_field);
 package.viewFile();

 for (package.stepFirst();!package.eof();package.stepNext())
  {
//   package.viewRecord();
   printf("%20.20s %d\n",
          package.fieldString("Name"),
          package.fieldInt("Flag"));
  }

 getch();
}

