#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <mem.h>
#include <dir.h>

#ifdef __WIN32__
#include <windows.h>
#else
#include <dos.h>
#endif

#include <btrieve.h>
// This define to show classes in Builder ClassExplorer
#if 0
#include "btrieve.h"
#endif

#ifdef __WIN32__
// Function type for call from DLL
typedef BTR_SINT (__import __stdcall *BTRCALL)(BTR_WORD  operation,
                                               BTR_VOID  *posBlock,
                                               BTR_VOID  *dataBuffer,
                                               BTR_DWORD *dataLength,
                                               BTR_VOID  *keyBuffer,
                                               BTR_BYTE  keyLength,
                                               BTR_CHAR  ckeynum);
//#ifdef __cplusplus
//extern "C" BTRCALL btrcall;
//#else
//extern BTRCALL btrcall;
//#endif

HINSTANCE hdll     = 0;
BTRCALL   cbtrcall = 0;
#else
#define BTR_INT     0x7B
#define BTR_OFFSET  0x33
#define VARIABLE_ID 0x6176

struct BTRIEVE_PARMS     /* structure passed to Btrieve Record Manager */
{
 BTR_VOID *bufAddress;   /* callers data buffer Address  */
 BTR_WORD bufLength;     /* length of data buffer        */
 BTR_VOID *curAddress;   /* user position block Address  */
 BTR_VOID *fcbAddress;   /* Address of disk FCB          */
 BTR_WORD function;      /* requested function           */
 BTR_VOID *keyAddress;   /* Address of user's key buffer */
 BTR_BYTE keyLength;     /* length of user's key buffer  */
 BTR_BYTE keyNumber;     /* key of reference for request */
 BTR_SINT *statAddress;  /* Address of status word       */
 BTR_WORD xfaceID;       /* language identifier          */
} xData;
#endif

/********************************************************************
 Structures for DDF files
********************************************************************/
FIELDINFO file_field[] =
{
 {"Xf$Id",      BTR_TYPE_INTEGER,2, 0},
 {"Xf$Name",    BTR_TYPE_STRING, 20,0},
 {"Xf$Loc",     BTR_TYPE_STRING, 64,0},
 {"Xf$Flags",   BTR_TYPE_INTEGER,1, 0},
 {"Xf$Reserved",BTR_TYPE_UNKNOWN,10,0},
 {0,0,0,0}
};

KEYINFO file_key[] =
{
 {"+(B)Xf$Id"},
 {"+(MA00)Xf$Name"},
 {0}
};

FIELDINFO field_field[] =
{
 {"Xe$Id",      BTR_TYPE_INTEGER, 2, 0},
 {"Xe$File",    BTR_TYPE_INTEGER, 2, 0},
 {"Xe$Name",    BTR_TYPE_STRING,  20,0},
 {"Xe$DataType",BTR_TYPE_INTEGER, 1, 0},
 {"Xe$Offset",  BTR_TYPE_INTEGER, 2, 0},
 {"Xe$Size",    BTR_TYPE_INTEGER, 2, 0},
 {"Xe$Dec",     BTR_TYPE_STRING,  1, 0},
 {"Xe$Flags",   BTR_TYPE_INTEGER, 2, 0},
 {0,0,0,0}
};

KEYINFO field_key[] =
{
 {"+(B)Xe$Id"},
 {"+(DB)Xe$File"},
 {"+(DMA00)Xe$Name"},
 {"+(MB)Xe$File+(MA00)Xe$Name"},
 {"+(DMRE)Xe$File+(DMRE)Xe$Offset+(DMRE)Xe$Dec"},
 {0}
};

FIELDINFO index_field[] =
{
 {"Xi$File",  BTR_TYPE_INTEGER,2,0},
 {"Xi$Field", BTR_TYPE_INTEGER,2,0},
 {"Xi$Number",BTR_TYPE_INTEGER,2,0},
 {"Xi$Part",  BTR_TYPE_INTEGER,2,0},
 {"Xi$Flags", BTR_TYPE_INTEGER,2,0},
 {0,0,0,0}
};

KEYINFO index_key[] =
{
 {"+(DB)Xi$File"},
 {"+(DB)Xi$Field"},
 {"+(MRE)Xi$File+(MRE)Xi$Number+(MRE)Xi$Part"},
 {0}
};

FIELDINFO comment_field[] =
{
 {"Xc$File",   BTR_TYPE_INTEGER,2,  0},
 {"Xc$Field",  BTR_TYPE_INTEGER,2,  0},
 {"Xc$Comment",BTR_TYPE_STRING, 100,0},
 {0,0,0,0}
};

KEYINFO comment_key[] =
{
 {0}
};

/********************************************************************
 BTRV
********************************************************************/
#ifdef __WIN32__
BTR_SINT BTRV(BTR_WORD operation,
              BTR_VOID *posBlock,
              BTR_VOID *dataBuffer,
              BTR_WORD *dataLength,
              BTR_VOID *keyBuffer,
              BTR_SINT keyNumber)
{
 BTR_BYTE  keyLen;
 BTR_SINT  error;
 BTR_CHAR  keyNum;
 BTR_DWORD len;

 keyLen = BTR_CONST_MAXKEYSIZE;
 keyNum = keyNumber;
 if (dataLength)
  len = *dataLength;
 else
  len = 0;

 error = cbtrcall(operation,posBlock,dataBuffer,&len,keyBuffer,keyLen,keyNum);

 if (dataLength)
  *dataLength = (BTR_WORD) len;

 return error;
}
#else
BTR_SINT BTRV(BTR_WORD operation,
	      BTR_VOID      *posBlock,
              BTR_VOID      *dataBuf,
              BTR_WORD      *dataLen,
              BTR_VOID      *keyBuf,
              BTR_SINT      keyNum)
{
 struct REGPACK regs;
 BTR_SINT       stat;

 stat = 0;

// Move user parameters to xData, the block where Btrieve expects them.
 xData.function    = operation;
 xData.statAddress = &stat;
 xData.fcbAddress  = posBlock;
 xData.curAddress  = (BTR_CHAR *) posBlock + 38;
 xData.bufAddress  = dataBuf;
 xData.bufLength   = *dataLen;
 xData.keyAddress  = keyBuf;
 xData.keyLength   = 255;          /* use max since we don't know */
 xData.keyNumber   = keyNum;
 xData.xfaceID     = VARIABLE_ID;

// Make call to the Btrieve Record Manager.
// parameter block is expected to be in DS:DX
 regs.r_ds = FP_SEG(&xData);
 regs.r_dx = FP_OFF(&xData);
 intr(BTR_INT,&regs);
 *dataLen = xData.bufLength;

 return stat; /* return status */
}
#endif

/********************************************************************
 Btrieve
********************************************************************/
Btrieve initBtrieve;

BTR_BOOL  Btrieve::isLoaded    = 0;
BTR_SINT  Btrieve::debug       = 0;
BTR_WORD  Btrieve::infoNum     = 0;
BTR_WORD  Btrieve::defPageSize = 4096;
BTR_WORD  Btrieve::defFlags    = BTR_FILE_BALANCED | BTR_FILE_0100;
FIELDINFO *Btrieve::info[BTR_CONST_MAXINFO];

Btrieve::Btrieve(BTR_VOID)
{
 Btrieve::debugView(100,"Btrieve::Btrieve\n");

#ifdef __WIN32__
 hdll = LoadLibrary("WBTRV32");
 if (hdll == 0)
  {
   debugView(1,"Btrieve: DLL load error\n");
   return;
  }

 cbtrcall = (BTRCALL) GetProcAddress(hdll,"BTRCALL");
 if (cbtrcall == 0)
  {
   debugView(1,"Btrieve: Error GetProcAddress(BTRCALL)\n");
   return;
  }
#else
 BTR_VERSION version;
 BTR_WORD    len;

// Check to see that the Btrieve Record Manager has been started.
 if (FP_OFF(getvect(BTR_INT)) != BTR_OFFSET)
  {
   Btrieve::debugView(1,"Btrieve: Btrieve not loaded\n");
   return;
  }

 memset(&version,0,sizeof(version));
 len = sizeof(version);
 if (BTRV(BTR_OP_VERSION,0,&version,&len,0,0))
  {
   Btrieve::debugView(1,"Btrieve: Btrieve not loaded\n");
   return;
  }
 if (version.Version == 0)
  {
   Btrieve::debugView(1,"Btrieve: Unknown btrieve version\n");
   return;
  }
#endif

 isLoaded = 1;
}

Btrieve::~Btrieve(BTR_VOID)
{
 for (BTR_SINT i=0;i<infoNum;i++)
  {
   delete[] info[i]->name;
   delete[] info[i];
   info[i] = 0;
  }
 infoNum = 0;

#ifdef __WIN32__
 if (isLoaded)
  BTRV(BTR_OP_STOP,0,0,0,0,0);
 FreeLibrary(hdll);
#endif
}

BTR_VOID Btrieve::debugView(BTR_SINT l,BTR_CHAR *s,...)
{
 va_list vl;

 if (l > debug)
  return;

 va_start(vl,s);
 vprintf(s,vl);
 va_end(vl);
}

BTR_VOID Btrieve::CreateDDF(BTR_CHAR *path)
{
 BTR_CHAR fname[BTR_CONST_FILENAMELEN];
 BTR_CHAR acs[BTR_CONST_ACSSIZE];
 BTR_WORD oldFlags;
 BTR_WORD oldPageSize;

 memcpy(acs,"\xACUPPER   ",9);
 for (BTR_SINT i=0;i<256;i++)
  acs[9+i] = i;

 oldFlags    = defFlags;
 oldPageSize = defPageSize;
 defFlags    = 0;
 defPageSize = 512;

 sprintf(fname,"%s\\file.ddf",path);
 Btrieve::create(fname,file_field,file_key,acs);

 sprintf(fname,"%s\\field.ddf",path);
 Btrieve::create(fname,field_field,field_key,acs);

 sprintf(fname,"%s\\index.ddf",path);
 Btrieve::create(fname,index_field,index_key);

 defFlags    = oldFlags;
 defPageSize = oldPageSize;

 AddToDDF(path,"X$File","file.ddf",file_field,file_key,16);
 AddToDDF(path,"X$Field","field.ddf",field_field,field_key,16);
 AddToDDF(path,"X$Index","index.ddf",index_field,index_key,16);
}

BTR_VOID Btrieve::AddToDDF(BTR_CHAR *path,BTR_CHAR *name,BTR_CHAR *loc,
                           FIELDINFO *field,KEYINFO *key,BTR_WORD aFlags)
{
 BTR_CHAR fname[BTR_CONST_FILENAMELEN];
 BTR_WORD maxfile;
 BTR_WORD maxfield;
 BTR_WORD position;
 BTR_WORD keyNum;
 BTR_WORD segNum;
 BTR_CHAR strbuf[1+2+20+BTR_CONST_FLDNAMELEN+1];
 BTR_WORD flags;

 sprintf(fname,"%s\\file.ddf",path);
 DAT file_ddf(fname);
 file_ddf.setField(file_field);
// file_ddf.viewFile();

 sprintf(fname,"%s\\field.ddf",path);
 DAT field_ddf(fname);
 field_ddf.setField(field_field);
// field_ddf.viewFile();

 sprintf(fname,"%s\\index.ddf",path);
 DAT index_ddf(fname);
 index_ddf.setField(index_field);

 maxfile = 1;
 file_ddf.keySelect("+Xf$Id");
 if (file_ddf.getLast() == 0)
  maxfile = (BTR_WORD)(file_ddf.fieldInt("Xf$Id") + 1);

 maxfield = 1;
 field_ddf.keySelect("+Xe$Id");
 if (field_ddf.getLast() == 0)
  maxfield = (BTR_WORD)(field_ddf.fieldInt("Xe$Id") + 1);

 file_ddf.clear();
 file_ddf.assignInt("Xf$Id",maxfile);
 file_ddf.assignString("Xf$Name",name);
 file_ddf.assignString("Xf$Loc",loc);
 file_ddf.assignChar("Xf$Flags",aFlags);
 file_ddf.insert();

 position = 0;
 for (BTR_SINT f=0;field[f].name;f++)
  {
   field_ddf.clear();
   field_ddf.assignInt("Xe$Id",(BTR_WORD)(maxfield+f));
   field_ddf.assignInt("Xe$File",maxfile);
   field_ddf.assignString("Xe$Name",field[f].name);
   field_ddf.assignChar("Xe$DataType",field[f].type);
   field_ddf.assignInt("Xe$Offset",position);
   field_ddf.assignInt("Xe$Size",field[f].length);
   field_ddf.assignChar("Xe$Dec",field[f].dec);
   field_ddf.assignInt("Xe$Flags",0);
   field_ddf.insert();
   position += field[f].length;
  }

 for (keyNum=0;key[keyNum].name;keyNum++)
  {
   segNum = 0;
   for (BTR_CHAR *pseg=key[keyNum].name;pseg;pseg=strpbrk(pseg+1,"+-"))
    {
     memset(strbuf,0,sizeof(strbuf));
     memcpy(strbuf,pseg,strcspn(pseg+1,"+-")+1);
     BTR_CHAR *pfld = strbuf + 1;
     if (strchr(pfld,')'))
      pfld = strchr(pfld,')') + 1;

     for (BTR_SINT f=0;field[f].name;f++)
      {
       if (stricmp(pfld,field[f].name) == 0)
        {
         flags = 0;
     	 flags |= BTR_KEY_SEG;
         if (strbuf[0] == '-')
          flags |= BTR_KEY_DESC;
         if (strchr(strbuf,')'))
          {
           *strchr(strbuf,')') = 0;
	   if (strchr(strbuf,'M'))
	    flags |= BTR_KEY_MOD;
	   if (strchr(strbuf,'D'))
	    flags |= BTR_KEY_DUP;
	   if (strchr(strbuf,'E'))
	    flags |= BTR_KEY_EXT;
	   if (strchr(strbuf,'I'))
	    flags |= BTR_KEY_NOCASE;
	   if (strchr(strbuf,'B'))
	    flags |= BTR_KEY_BIN;
	   if (strchr(strbuf,'R'))
	    flags |= BTR_KEY_REP;
	   if (strchr(strbuf,'A'))
	    flags |= BTR_KEY_ALT;
	   if (strchr(strbuf,'N'))
	    flags |= BTR_KEY_NUL;
	  }
         index_ddf.clear();
         index_ddf.assignInt("Xi$File",maxfile);
         index_ddf.assignInt("Xi$Field",(BTR_WORD)(maxfield+f));
         index_ddf.assignInt("Xi$Number",keyNum);
         index_ddf.assignInt("Xi$Part",segNum);
         index_ddf.assignInt("Xi$Flags",flags);
         index_ddf.insert();
         break;
        }
      } // for (BTR_SINT f=0;...)
     segNum++;
    } // for (pseg=...)
   index_ddf.assignInt("Xi$Flags",(BTR_WORD)(flags&~BTR_KEY_SEG));
   index_ddf.update();
  } // for (keyNum=0;...)
}

FIELDINFO *Btrieve::loadField(BTR_CHAR *ddfpath,BTR_CHAR *name)
{
 BTR_WORD fieldNum;
 BTR_CHAR fname[BTR_CONST_FILENAMELEN+1];

 Btrieve::debugView(100,"Btrieve::loadField\n");

 if (infoNum >= sizeof(info)/sizeof(info[0]))
  {
   debugView(1,"loadField: No more FIELDINFOs\n");
   return 0;
  }

 sprintf(fname,"%s\\%s",ddfpath,"FILE.DDF");
 DAT file_ddf(fname);
 file_ddf.setField(file_field);

 sprintf(fname,"%s\\%s",ddfpath,"FIELD.DDF");
 DAT field_ddf(fname);
 field_ddf.setField(field_field);

 if (!file_ddf.opened() || !field_ddf.opened())
  {
   Btrieve::debugView(1,"loadField: No DDF files\n");
   return 0;
  }

 file_ddf.keySelect("+Xf$Name");
 file_ddf.assignString("Xf$Name",name);
 if (file_ddf.getE())
  {
   Btrieve::debugView(1,"loadField: No file description\n");
   return 0;
  }

// Count fieldNum
 fieldNum = 0;
 field_ddf.keySelect("+Xe$File+Xe$Offset+Xe$Dec");
 field_ddf.getFirst();
 field_ddf.assignInt("Xe$File",file_ddf.fieldInt("Xf$Id"));
 field_ddf.assignInt("Xe$Offset",0);
 field_ddf.assignChar("Xe$Dec",0);
 for (field_ddf.getGE();!field_ddf.eof();field_ddf.getNext())
  {
   if (field_ddf.fieldInt("Xe$File") != file_ddf.fieldInt("Xf$Id"))
    break;
   if (field_ddf.fieldInt("Xe$Size") == 0)
    continue;
   fieldNum++;
  }

 info[infoNum] = new FIELDINFO[fieldNum+1];
 if (info[infoNum] == 0)
  {
   Btrieve::debugView(1,"loadField: No memory\n");
   return 0;
  }

 fieldNum = 0;
 field_ddf.keySelect("+Xe$File+Xe$Offset+Xe$Dec");
 field_ddf.getFirst();
 field_ddf.assignInt("Xe$File",file_ddf.fieldInt("Xf$Id"));
 field_ddf.assignInt("Xe$Offset",0);
 field_ddf.assignChar("Xe$Dec",0);
 for (field_ddf.getGE();!field_ddf.eof();field_ddf.getNext())
  {
   if (field_ddf.fieldInt("Xe$File") != file_ddf.fieldInt("Xf$Id"))
    break;
   if (field_ddf.fieldInt("Xe$Size") == 0)
    continue;

   info[infoNum][fieldNum].name =
    new BTR_CHAR[strlen(field_ddf.fieldZString("Xe$Name"))+1];
   if (info[infoNum][fieldNum].name)
    strcpy(info[infoNum][fieldNum].name,field_ddf.fieldZString("Xe$Name"));
   info[infoNum][fieldNum].length = field_ddf.fieldInt("Xe$Size");
   info[infoNum][fieldNum].type   = field_ddf.fieldChar("Xe$DataType");
   info[infoNum][fieldNum].dec    = field_ddf.fieldChar("Xe$Dec");
   fieldNum++;
  }

 memset(&info[infoNum][fieldNum],0,sizeof(info[0]));

 infoNum++;

 return info[infoNum-1];
}

/********************************************************************
 Transactions
********************************************************************/
BTR_SINT Btrieve::BeginTransaction(BTR_VOID)
{
 BTR_SINT error;

 Btrieve::debugView(100,"Btrieve::BeginTransaction\n");

 error = BTRV(BTR_OP_BEGIN_TRAN,0,0,0,0,0);
 if (error)
  Btrieve::debugView(1,"BeginTransaction: Error %d\n",error);

 return error;
}

BTR_SINT Btrieve::EndTransaction(BTR_VOID)
{
 BTR_SINT error;

 Btrieve::debugView(100,"Btrieve::EndTransaction\n");

 error = BTRV(BTR_OP_END_TRAN,0,0,0,0,0);
 if (error)
  Btrieve::debugView(1,"EndTransaction: Error %d\n",error);

 return error;
}

BTR_SINT Btrieve::AbortTransaction(BTR_VOID)
{
 BTR_SINT error;

 Btrieve::debugView(100,"Btrieve::AbortTransaction\n");

 error = BTRV(BTR_OP_ABORT_TRAN,0,0,0,0,0);
 if (error)
  Btrieve::debugView(1,"AbortTransaction: Error %d\n",error);

 return error;
}

BTR_SINT Btrieve::create(BTR_CHAR *name,FIELDINFO *field,KEYINFO *key,BTR_VOID *acs)
{
 BTR_CHAR   pos[BTR_CONST_POSLEN];
 BTR_SINT   error;
 BTR_WORD   keyNum;
 BTR_WORD   segNum;
 BTR_WORD   recLen;
 BTR_STATUS *stat;
 BTR_WORD   len;
 BTR_CHAR   strbuf[1+2+20+BTR_CONST_FLDNAMELEN+1];
 BTR_WORD   position;

 Btrieve::debugView(100,"Btrieve::create\n");

 if (field == 0)
  {
   debugView(1,"create: field = 0\n");
   return BTR_ERR_UNKNOWN;
  }

 stat = new BTR_STATUS;
 if (stat == 0)
  {
   debugView(1,"create: No memory\n");
   return BTR_ERR_NOMEMORY;
  }

 stat->file.flags = defFlags;
 recLen = 0;
 for (BTR_SINT f=0;field[f].name;f++)
  {
   if (field[f].type != BTR_TYPE_MEMO)
    recLen += field[f].length;
   else
    stat->file.flags |= BTR_FILE_VAR;
  }

 segNum = 0;
 for (keyNum=0;key[keyNum].name;keyNum++)
  {
   for (BTR_CHAR *pseg=key[keyNum].name;pseg;pseg=strpbrk(pseg+1,"+-"))
    {
     memset(&stat->seg[segNum],0,sizeof(stat->seg[0]));
     memset(strbuf,0,sizeof(strbuf));
     memcpy(strbuf,pseg,strcspn(pseg+1,"+-")+1);
     BTR_CHAR *pfld = strbuf + 1;
     if (strchr(pfld,')'))
      pfld = strchr(pfld,')') + 1;

     position = 0;
     for (BTR_SINT f=0;field[f].name;f++)
      {
       if (stricmp(pfld,field[f].name) == 0)
        {
     	 stat->seg[segNum].flags |= BTR_KEY_SEG;
         if (strbuf[0] == '-')
          stat->seg[segNum].flags |= BTR_KEY_DESC;
         if (strchr(strbuf,')'))
          {
           *strchr(strbuf,')') = 0;
	   if (strchr(strbuf,'M'))
	    stat->seg[segNum].flags |= BTR_KEY_MOD;
	   if (strchr(strbuf,'D'))
	    stat->seg[segNum].flags |= BTR_KEY_DUP;
	   if (strchr(strbuf,'E'))
	    stat->seg[segNum].flags |= BTR_KEY_EXT;
	   if (strchr(strbuf,'I'))
	    stat->seg[segNum].flags |= BTR_KEY_NOCASE;
	   if (strchr(strbuf,'B'))
	    stat->seg[segNum].flags |= BTR_KEY_BIN;
	   if (strchr(strbuf,'R'))
	    stat->seg[segNum].flags |= BTR_KEY_REP;
	   if (strchr(strbuf,'A'))
            {
	     stat->seg[segNum].flags |= BTR_KEY_ALT;
	     stat->seg[segNum].acs = 0;
	     sscanf(strchr(strbuf,'A')+1,"%X",&stat->seg[segNum].acs);
            }
	   if (strchr(strbuf,'N'))
	    {
	     stat->seg[segNum].flags |= BTR_KEY_NUL;
	     stat->seg[segNum].nulval = 0;
	     sscanf(strchr(strbuf,'N')+1,"%X",&stat->seg[segNum].nulval);
	    }
	  }
         stat->seg[segNum].position = (BTR_WORD)(position + 1);
         stat->seg[segNum].length = field[f].length;
         stat->seg[segNum].type = field[f].type;
         break;
        }
       position += field[f].length;
      }

     segNum++;
    } // for (pseg=...)

   stat->seg[segNum-1].flags &= ~BTR_KEY_SEG;
  } // for (keyNum=0;...)

 stat->file.recLen      = recLen;
 stat->file.pageSize    = defPageSize;
 stat->file.keyNum      = keyNum;
 stat->file.recNum      = 0;
 stat->file.dupPointers = 0;
 stat->file.reserved    = 0;
 stat->file.allocations = 0;

 len = (BTR_WORD)(sizeof(BTR_FILE) + segNum*sizeof(BTR_SEG));
 if (acs)
  {
   memcpy((BTR_CHAR *)stat+len,acs,BTR_CONST_ACSSIZE);
   len += (BTR_WORD)BTR_CONST_ACSSIZE;
  }
 error = BTRV(BTR_OP_CREATE,pos,stat,&len,name,0);
 if (error)
  {
   debugView(1,"create: Error %d %s\n",error,name);
   delete stat;
   return error;
  }

 delete stat;

 return error;
}

BTR_CHAR *Btrieve::type(BTR_SINT type)
{
 static BTR_CHAR typestr[20];

 switch (type)
  {
   case BTR_TYPE_STRING:
    return "string";
   case BTR_TYPE_INTEGER:
    return "integer";
   case BTR_TYPE_FLOAT:
    return "float";
   case BTR_TYPE_MEMO:
    return "memo";
   case BTR_TYPE_UNSIGNED:
    return "unsigned";
   default:
    sprintf(typestr,"unknown %d",type);
    return typestr;
  }
}

/********************************************************************
 DAT
********************************************************************/
DAT::DAT(BTR_VOID)
{
 Btrieve::debugView(100,"DAT::DAT(void)\n");

 init();
}

/********************************************************************
 DAT
********************************************************************/
DAT::DAT(BTR_CHAR *name,BTR_CHAR *owner)
{
 Btrieve::debugView(100,"DAT::DAT(char *)\n");

 init();
 open(name,owner);
}

/********************************************************************
 ~DAT
********************************************************************/
DAT::~DAT(BTR_VOID)
{
 Btrieve::debugView(100,"DAT::~DAT\n");

 if (isOpened)
  close();
}

/********************************************************************
 init
********************************************************************/
BTR_VOID DAT::init(BTR_VOID)
{
 Btrieve::debugView(100,"DAT::init\n");

 fileName = 0;
 isOpened = 0;
 fieldNum = 0;
 field = 0;
 segNum = 0;
 seg = 0;
 keyNum = 0;
 isEof = 1;
 currentKey = BTR_CONST_NOKEY;
 recLen = 0;
 recBufLen = 0;
 recVarLen = 0;
 recBuf = 0;
 keyBuf = 0;
 pageSize = 0;
 flags = 0;
 recNum = 0;
}

/********************************************************************
 open
********************************************************************/
BTR_SINT DAT::open(BTR_CHAR *name,BTR_CHAR *owner)
{
 BTR_SINT error;

 Btrieve::debugView(100,"DAT::open\n");

 if (isOpened)
  close();

 error = BTRV(BTR_OP_OPEN,pos,owner,0,name,0);
 if (error)
  {
   Btrieve::debugView(1,"open: Error %d %s\n",error,name);
   return error;
  }

 isOpened = 1;

 delete[] keyBuf;
 keyBuf = new BTR_CHAR[BTR_CONST_MAXKEYSIZE];

 delete[] fileName;
 fileName = new BTR_CHAR[strlen(name)+1];
 if (fileName == 0)
  {
   Btrieve::debugView(1,"open: No memory\n");
   return BTR_ERR_NOMEMORY;
  }
 strcpy(fileName,name);

 status();

 return 0;
}

/********************************************************************
 close
********************************************************************/
BTR_SINT DAT::close(BTR_VOID)
{
 BTR_SINT error;

 Btrieve::debugView(100,"DAT::close\n");

 error = BTRV(BTR_OP_CLOSE,pos,0,0,0,0);
 if (error)
  {
   Btrieve::debugView(1,"close: Error %d\n",error);
   return error;
  }

 isOpened = 0;

 delete[] fileName;
 fileName = 0;

 delete[] seg;
 seg = 0;

 for (BTR_SINT f=0;f<fieldNum;f++)
  delete[] field[f].name;
 delete[] field;
 fieldNum = 0;
 field = 0;

 delete[] recBuf;
 recBuf = 0;
 recBufLen = 0;
 recVarLen = 0;

 delete[] keyBuf;
 keyBuf = 0;

 return error;
}

/********************************************************************
 status
********************************************************************/
BTR_SINT DAT::status(BTR_VOID)
{
 BTR_SINT   error;
 BTR_STATUS *stat;
 BTR_WORD   len;
 BTR_CHAR   ext[64];

 Btrieve::debugView(100,"DAT::status\n");

 stat = new BTR_STATUS;
 if (stat == 0)
  {
   Btrieve::debugView(1,"status: No memory\n");
   return BTR_ERR_NOMEMORY;
  }

 len = sizeof(BTR_STATUS);
 error = BTRV(BTR_OP_STATUS,pos,stat,&len,ext,0);
 if (error)
  {
   delete stat;
   Btrieve::debugView(1,"status: Error %d\n",error);
   return error;
  }

 flags    = stat->file.flags;
 recLen   = stat->file.recLen;
 pageSize = stat->file.pageSize;
 recNum   = stat->file.recNum;

 if (recBuf == 0)
  {
   recBufLen = recLen;
   recVarLen = recLen;
   recBuf = new BTR_CHAR[recBufLen];
   if (recBuf == 0)
    {
     delete stat;
     Btrieve::debugView(1,"status: No memory\n");
     return BTR_ERR_NOMEMORY;
    }
  }

 if (seg == 0)
  {
   // Count segNum keyNum
   for (segNum=keyNum=0;keyNum<stat->file.keyNum;segNum++)
    if ((stat->seg[segNum].flags&BTR_KEY_SEG) == 0)
     keyNum++;

   seg = new SEG[segNum];
   if (seg == 0)
    {
     delete stat;
     Btrieve::debugView(1,"status: No memory\n");
     return BTR_ERR_NOMEMORY;
    }

   for (segNum=keyNum=0;keyNum<stat->file.keyNum;segNum++)
    {
     seg[segNum].key      = keyNum;
     seg[segNum].position = (BTR_WORD)(stat->seg[segNum].position - 1);
     seg[segNum].length   = stat->seg[segNum].length;
     seg[segNum].type     = stat->seg[segNum].type;
     seg[segNum].nulval   = stat->seg[segNum].nulval;
     seg[segNum].acs      = stat->seg[segNum].acs;
     seg[segNum].flags    = stat->seg[segNum].flags;
     seg[segNum].field    = 0;
     if ((stat->seg[segNum].flags&BTR_KEY_SEG) == 0)
      keyNum++;
    }
  } // if (seg == 0)

 delete stat;

 return 0;
}

/********************************************************************
 setField
********************************************************************/
BTR_SINT DAT::setField(FIELDINFO *info)
{
 BTR_SINT f;
 BTR_WORD position;

 Btrieve::debugView(100,"DAT::setField\n");

 if (info == 0)
  {
   Btrieve::debugView(1,"setField: info = 0\n");
   return BTR_ERR_UNKNOWN;
  }

 if (!isOpened)
  {
   Btrieve::debugView(1,"setField: File not open\n");
   return BTR_ERR_UNKNOWN;
  }

 for (f=0;f<fieldNum;f++)
  delete[] field[f].name;
 delete[] field;
 field = 0;

 for (fieldNum=0;info[fieldNum].name;fieldNum++)
  ; // Count fieldNum

 field = new FIELD[fieldNum];
 if (field == 0)
  {
   fieldNum = 0;
   Btrieve::debugView(1,"setField: No memory\n");
   return BTR_ERR_NOMEMORY;
  }

 position = 0;
 for (f=0;f<fieldNum;f++)
  {
   field[f].name = new BTR_CHAR[strlen(info[f].name)+1];
   if (field[f].name)
    strcpy(field[f].name,info[f].name);
   field[f].position = position;
   field[f].length   = info[f].length;
   field[f].type     = info[f].type;
   field[f].dec      = info[f].dec;
   position += info[f].length;
  }

 if (position != recLen)
  Btrieve::debugView(1,"setField: field sum len (%d) != recLen (%d)\n",position,recLen);

//
 if (position > recBufLen)
  {
   delete[] recBuf;
   recBufLen = position;
   recVarLen = position;
   recBuf = new BTR_CHAR[recBufLen];
   if (recBuf == 0)
    {
     Btrieve::debugView(1,"setField: No memory\n");
     return BTR_ERR_NOMEMORY;
    }
  }

 for (BTR_SINT s=0;s<segNum;s++)
  for (BTR_SINT f=0;f<fieldNum;f++)
   if ((seg[s].position==field[f].position) &&
       (seg[s].length==field[f].length))
    {
     seg[s].field = &field[f];
     break;
    }

 return 0;
};

/********************************************************************
 loadField
********************************************************************/
BTR_SINT DAT::loadField(BTR_CHAR *path,BTR_CHAR *aname)
{
 BTR_CHAR fname[BTR_CONST_FILENAMELEN+1];
 BTR_CHAR name[BTR_CONST_NAMELEN+1];
 BTR_WORD position;

 Btrieve::debugView(100,"DAT::loadField\n");

 if (!isOpened)
  {
   Btrieve::debugView(1,"loadField: File not opened\n");
   return BTR_ERR_UNKNOWN;
  }

 if (aname == 0)
  fnsplit(fileName,0,0,name,0);
 else
  strcpy(name,aname);

 sprintf(fname,"%s\\%s",path,"FILE.DDF");
 DAT file_ddf(fname);
 file_ddf.setField(file_field);

 sprintf(fname,"%s\\%s",path,"FIELD.DDF");
 DAT field_ddf(fname);
 field_ddf.setField(field_field);

 if (!file_ddf.opened() || !field_ddf.opened())
  {
   Btrieve::debugView(1,"loadField: No DDF files\n");
   return BTR_ERR_UNKNOWN;
  }

 file_ddf.keySelect("+Xf$Name");
 file_ddf.assignString("Xf$Name",name);
 if (file_ddf.getE())
  {
   Btrieve::debugView(1,"loadField: No file description\n");
   return BTR_ERR_UNKNOWN;
  }

 for (BTR_SINT f=0;f<fieldNum;f++)
  delete[] field[f].name;
 delete[] field;
 field = 0;
 fieldNum = 0;

// Count fieldNum
 field_ddf.keySelect("+Xe$File+Xe$Offset+Xe$Dec");
 field_ddf.getFirst();
 field_ddf.assignInt("Xe$File",file_ddf.fieldInt("Xf$Id"));
 field_ddf.assignInt("Xe$Offset",0);
 field_ddf.assignChar("Xe$Dec",0);
 for (field_ddf.getGE();!field_ddf.eof();field_ddf.getNext())
  {
   if (field_ddf.fieldInt("Xe$File") != file_ddf.fieldInt("Xf$Id"))
    break;
   if (field_ddf.fieldInt("Xe$Size") == 0)
    continue;

   fieldNum++;
  }

 field = new FIELD[fieldNum];
 if (field == 0)
  {
   fieldNum = 0;
   Btrieve::debugView(1,"setField: No memory\n");
   return BTR_ERR_NOMEMORY;
  }

 fieldNum = 0;
 position = 0;
 field_ddf.keySelect("+Xe$File+Xe$Offset+Xe$Dec");
 field_ddf.getFirst();
 field_ddf.assignInt("Xe$File",file_ddf.fieldInt("Xf$Id"));
 field_ddf.assignInt("Xe$Offset",0);
 field_ddf.assignChar("Xe$Dec",0);
 for (field_ddf.getGE();!field_ddf.eof();field_ddf.getNext())
  {
   if (field_ddf.fieldInt("Xe$File") != file_ddf.fieldInt("Xf$Id"))
    break;
   if (field_ddf.fieldInt("Xe$Size") == 0)
    continue;

   field[fieldNum].name =
    new BTR_CHAR[strlen(field_ddf.fieldZString("Xe$Name"))+1];
   if (field[fieldNum].name)
    strcpy(field[fieldNum].name,field_ddf.fieldZString("Xe$Name"));
   field[fieldNum].position = field_ddf.fieldInt("Xe$Offset");
   field[fieldNum].length   = field_ddf.fieldInt("Xe$Size");
   field[fieldNum].type     = field_ddf.fieldChar("Xe$DataType");
   field[fieldNum].dec      = field_ddf.fieldChar("Xe$Dec");
   position += field_ddf.fieldInt("Xe$Size");
   fieldNum++;
  }

 if (position > recBufLen)
  {
   delete[] recBuf;
   recBufLen = position;
   recVarLen = position;
   recBuf = new BTR_CHAR[recBufLen];
   if (recBuf == 0)
    {
     Btrieve::debugView(1,"setField: No memory\n");
     return BTR_ERR_NOMEMORY;
    }
  }

 for (BTR_SINT s=0;s<segNum;s++)
  for (BTR_SINT f=0;f<fieldNum;f++)
   if ((seg[s].position==field[f].position) &&
       (seg[s].length==field[f].length))
    {
     seg[s].field = &field[f];
     break;
    }

 return 0;
}

/********************************************************************
 viewXXXXX
********************************************************************/
BTR_VOID DAT::viewFile(BTR_VOID)
{
 if (!isOpened)
  return;

 viewHeader();
 viewFields();
 viewSegs();
 viewKeys();
}

BTR_VOID DAT::viewHeader(BTR_VOID)
{
 printf("RecLen=%d recBufLen=%d recVarLen=%d pageSize=%d keyNum=%d recNum=%ld\n",
        recLen,recBufLen,recVarLen,pageSize,keyNum,recNum);
 printf("flags=%04X ",flags);
 if (flags & 0x0001)
  printf("VAR ");
 if (flags & 0x0002)
  printf("0x0002 ");
 if (flags & 0x0004)
  printf("0x0004 ");
 if (flags & 0x0008)
  printf("COMPRESS ");
 if (flags & 0x0010)
  printf("KEYONLY ");
 if (flags & 0x0020)
  printf("BAL ");
 if (flags & 0x0040)
  printf("0x0040 ");
 if (flags & 0x0080)
  printf("0x0080 ");
 if (flags & 0x0100)
  printf("0x0100 ");
 if (flags & 0x0200)
  printf("0x0200 ");
 if (flags & 0x0400)
  printf("0x0400 ");
 if (flags & 0x0800)
  printf("0x0800 ");
 if (flags & 0x1000)
  printf("0x1000 ");
 if (flags & 0x2000)
  printf("0x2000 ");
 if (flags & 0x4000)
  printf("0x4000 ");
 if (flags & 0x8000)
  printf("0x8000 ");
 printf("\n");
}

BTR_VOID DAT::viewFields(BTR_VOID)
{
 for (BTR_SINT f=0;f<fieldNum;f++)
  printf("FIELD %-20s %-10.10s %4d %4d %2d\n",
         field[f].name,
         Btrieve::type(field[f].type),
         field[f].length,
         field[f].position,
         field[f].dec);
}

BTR_VOID DAT::viewSegs(BTR_VOID)
{
 BTR_CHAR nulval[5];
 BTR_CHAR acs[5];

 for (BTR_SINT s=0;s<segNum;s++)
  {
   sprintf(nulval,"%02X",(BTR_WORD) seg[s].nulval);
   sprintf(acs,"%02X",(BTR_WORD) seg[s].acs);
   printf("SEG %2d: %-10.10s %3d %4d %-20.20s %04X %c%c%c%c%s%c%c%s%c%c%c%c%c%c%c%c%c%c\n",
          seg[s].key,
          Btrieve::type(seg[s].type),
          seg[s].length,
          seg[s].position,
          (seg[s].field)?seg[s].field->name:"???",
          seg[s].flags,
          (seg[s].flags&BTR_KEY_DUP)?'D':' ',
          (seg[s].flags&BTR_KEY_MOD)?'M':' ',
          (seg[s].flags&BTR_KEY_BIN)?'B':' ',
          (seg[s].flags&BTR_KEY_NUL)?'N':' ',
          (seg[s].flags&BTR_KEY_NUL)?nulval:"  ",
          (seg[s].flags&BTR_KEY_SEG)?'S':' ',
          (seg[s].flags&BTR_KEY_ALT)?'A':' ',
          (seg[s].flags&BTR_KEY_ALT)?acs:"  ",
          (seg[s].flags&0x0040)?'?':' ',
          (seg[s].flags&BTR_KEY_REP)?'R':' ',
          (seg[s].flags&BTR_KEY_EXT)?'E':' ',
          (seg[s].flags&0x0200)?'?':' ',
          (seg[s].flags&BTR_KEY_NOCASE)?'I':' ',
          (seg[s].flags&0x0800)?'?':' ',
          (seg[s].flags&0x1000)?'?':' ',
          (seg[s].flags&0x2000)?'?':' ',
          (seg[s].flags&0x4000)?'?':' ',
          (seg[s].flags&0x8000)?'?':' ');
  }
}

BTR_VOID DAT::viewKeys(BTR_VOID)
{
 for (BTR_SINT k=0;k<keyNum;k++)
  {
   printf("KEY %2d: ",k);
   for (BTR_SINT s=0;s<segNum;s++)
    if (seg[s].key == k)
     printf("+%s",(seg[s].field)?seg[s].field->name:"???");
   printf("\n");
  }
}

BTR_VOID DAT::viewRecord(BTR_VOID)
{
 for (BTR_SINT f=0;f<fieldNum;f++)
  viewField(field[f].name);
}

BTR_VOID DAT::viewField(BTR_CHAR *name)
{
 BTR_WORD len   = fieldLen(name);
 BTR_WORD type  = fieldType(name);
 BTR_CHAR *fptr = (BTR_CHAR *) fieldPtr(name);

 printf("%-20s (%4d): ",name,len);

 switch (type)
  {
   case BTR_TYPE_INTEGER:
    switch (len)
     {
      case 1:
       printf("%d\n",fieldChar(name));
       break;
      case 2:
       printf("%d\n",fieldInt(name));
       break;
      case 4:
       printf("%ld\n",fieldLong(name));
       break;
      default:
       printf("Unknown integer type size %d\n",len);
       break;
     }
    break;
   case BTR_TYPE_FLOAT:
    printf("%f\n",fieldDouble(name));
    break;
   case BTR_TYPE_UNSIGNED:
    switch (len)
     {
      case 2:
       printf("%04X\n",(BTR_WORD)fieldInt(name));
       break;
      case 4:
       printf("%08lX\n",(BTR_DWORD)fieldLong(name));
       break;
      default:
       printf("Unknown unsigned type size %d\n",len);
       break;
     }
    break;
   case BTR_TYPE_STRING:
    if (len == 1)
     printf("%c (%02X)\n",fieldChar(name),(BTR_BYTE) fieldChar(name));
    else
     printf("%s\n",fieldZString(name));
    break;
   case BTR_TYPE_DECIMAL:
    printf("%f\n",fieldDecimal(name));
    break;
   case BTR_TYPE_MEMO:
    for (BTR_CHAR *p=fptr;p-fptr<fieldMemoLen(name);p++)
     printf("%c",*p);
    printf("\n");
    break;
   case BTR_TYPE_UNKNOWN:
    for (BTR_CHAR *p2=fptr;p2-fptr<fieldLen(name);p2++)
     printf("%02X ",(BTR_BYTE) *p2);
    printf("\n");
    break;
   default:
    printf("%-20s (%4d): Unknown type %d\n",name,fieldLen(name),fieldType(name));
    break;
  }
}

/********************************************************************
 keySelect
********************************************************************/
BTR_VOID DAT::keySelect(BTR_SINT key)
{
 currentKey = key;
}

BTR_VOID DAT::keySelect(BTR_CHAR *name)
{
 BTR_CHAR strbuf[256];

 Btrieve::debugView(100,"DAT::keySelect\n");

 if (!isOpened)
  {
   Btrieve::debugView(1,"keySelect: File not open\n");
   return;
  }

 for (BTR_SINT k=0;k<keyNum;k++)
  {
   strbuf[0] = 0;
   for (BTR_SINT s=0;s<segNum;s++)
    if (seg[s].key == k)
     {
      strcat(strbuf,"+");
      strcat(strbuf,(seg[s].field)?seg[s].field->name:"???");
     }
   if (stricmp(strbuf,name) == 0)
    {
     currentKey = k;
     return;
    }
  }
 Btrieve::debugView(1,"keySelect: Key %s not found\n",name);
 currentKey = BTR_CONST_NOKEY;
}

/********************************************************************
 flags
********************************************************************/
BTR_BOOL DAT::eof(BTR_VOID)
{
 return isEof;
}

BTR_BOOL DAT::opened(BTR_VOID)
{
 return isOpened;
}

/********************************************************************
 fieldXXX
********************************************************************/
BTR_SINT DAT::fieldType(BTR_CHAR *name)
{
 for (BTR_SINT f=0;f<fieldNum;f++)
  if (strcmp(field[f].name,name) == 0)
   return field[f].type;

 Btrieve::debugView(1,"fieldType: Field %s not found\n",name);
 return BTR_TYPE_UNKNOWN;
}

BTR_VOID *DAT::fieldPtr(BTR_CHAR *name)
{
 for (BTR_SINT f=0;f<fieldNum;f++)
  if (strcmp(field[f].name,name) == 0)
   return recBuf + field[f].position;

 Btrieve::debugView(1,"fieldPtr: Field %s not found\n",name);
 return 0;
}

BTR_WORD DAT::fieldOffset(BTR_CHAR *name)
{
 for (BTR_SINT f=0;f<fieldNum;f++)
  if (strcmp(field[f].name,name) == 0)
   return field[f].position;

 Btrieve::debugView(1,"fieldOffset: Field %s not found\n",name);
 return 0;
}

BTR_WORD DAT::fieldLen(BTR_CHAR *name)
{
 for (BTR_SINT f=0;f<fieldNum;f++)
  if (strcmp(field[f].name,name) == 0)
   return field[f].length;

 Btrieve::debugView(1,"fieldLen: Field %s not found\n",name);
 return 0;
}

BTR_WORD DAT::fieldMemoLen(BTR_CHAR *name,BTR_WORD offset,BTR_WORD len)
{
 if (len == 0)
  len = (BTR_WORD) (fieldLen(name) - offset);

 if (recVarLen-(fieldOffset(name)+offset) < len)
  len = (BTR_WORD) (recVarLen-(fieldOffset(name)+offset));

 if (recVarLen < fieldOffset(name)+offset)
  len = 0;

 return len;
}

BTR_WORD DAT::fieldDec(BTR_CHAR *name)
{
 for (BTR_SINT f=0;f<fieldNum;f++)
  if (strcmp(field[f].name,name) == 0)
   return field[f].dec;

 Btrieve::debugView(1,"fieldDec: Field %s not found\n",name);
 return 0;
}

BTR_CHAR *DAT::fieldString(BTR_CHAR *name)
{
 if (fieldType(name) != BTR_TYPE_STRING &&
     fieldType(name) != BTR_TYPE_MEMO)
  {
   Btrieve::debugView(1,"fieldString: Invalid field type\n");
   return 0;
  }

 BTR_CHAR *p = (BTR_CHAR *) fieldPtr(name);

 return p;
}

BTR_CHAR *DAT::fieldZString(BTR_CHAR *name)
{
 static BTR_CHAR strbuf[256];
 BTR_CHAR        *p;
 BTR_WORD        len;

 if (fieldType(name) != BTR_TYPE_STRING)
  {
   Btrieve::debugView(1,"fieldZString: Invalid field type\n");
   return 0;
  }

 p = (BTR_CHAR *) fieldPtr(name);
 if (p == 0)
  {
   sprintf(strbuf,"Field %s not found",name);
   return strbuf;
  }

 len = fieldLen(name);
 if (len > sizeof(strbuf)-1)
  len = sizeof(strbuf) - 1;

 memset(strbuf,0,sizeof(strbuf));
 memcpy(strbuf,p,len);

 for (p=strchr(strbuf,0)-1;p>strbuf && *p==' ';p--)
  *p = 0;

 return strbuf;
}

BTR_CHAR *DAT::fieldZMemo(BTR_CHAR *name,BTR_WORD offset,BTR_WORD len)
{
 static BTR_CHAR strbuf[256];
 BTR_CHAR        *p;

 if (fieldType(name) != BTR_TYPE_MEMO)
  {
   Btrieve::debugView(1,"fieldZMemo: Invalid field type\n");
   return 0;
  }

 p = (BTR_CHAR *) fieldPtr(name);
 if (p == 0)
  {
   sprintf(strbuf,"Field %s not found",name);
   return strbuf;
  }

 len = fieldMemoLen(name,offset,len);
 if (len > sizeof(strbuf)-1)
  len = sizeof(strbuf) - 1;

 memset(strbuf,0,sizeof(strbuf));
 memcpy(strbuf,p+offset,len);

 for (p=strchr(strbuf,0)-1;p>strbuf && *p==' ';p--)
  *p = 0;

 return strbuf;
}

BTR_SINT DAT::fieldChar(BTR_CHAR *name)
{
 if (fieldType(name) != BTR_TYPE_STRING &&
     fieldType(name) != BTR_TYPE_INTEGER)
  {
   Btrieve::debugView(1,"fieldChar: Invalid field type\n");
   return 0;
  }

 if (fieldLen(name) != 1)
  {
   Btrieve::debugView(1,"fieldChar: Invalid field size\n");
   return 0;
  }

 BTR_CHAR *p = (BTR_CHAR *) fieldPtr(name);
 if (p == 0)
  return 0;

 return *p;
}

BTR_SINT DAT::fieldInt(BTR_CHAR *name)
{
 if (fieldType(name) != BTR_TYPE_INTEGER &&
     fieldType(name) != BTR_TYPE_UNSIGNED)
  {
   Btrieve::debugView(1,"fieldInt: Invalid field type\n");
   return 0;
  }

 if (fieldLen(name) != 2)
  {
   Btrieve::debugView(1,"fieldInt: Invalid field size\n");
   return 0;
  }

 BTR_SINT *p = (BTR_SINT *) fieldPtr(name);
 if (p == 0)
  return 0;

 return *p;
}

BTR_LONG DAT::fieldLong(BTR_CHAR *name)
{
 if (fieldType(name) != BTR_TYPE_INTEGER &&
     fieldType(name) != BTR_TYPE_UNSIGNED)
  {
   Btrieve::debugView(1,"fieldLong: Invalid field type\n");
   return 0;
  }

 if (fieldLen(name) != 4)
  {
   Btrieve::debugView(1,"fieldLong: Invalid field size\n");
   return 0;
  }

 BTR_LONG *p = (BTR_LONG *) fieldPtr(name);
 if (p == 0)
  return 0;

 return *p;
}

BTR_DOUBLE DAT::fieldDouble(BTR_CHAR *name)
{
 if (fieldType(name) != BTR_TYPE_FLOAT)
  {
   Btrieve::debugView(1,"fieldDouble: Invalid field type\n");
   return 0;
  }

 if (fieldLen(name) != 8)
  {
   Btrieve::debugView(1,"fieldDouble: Invalid field size\n");
   return 0.0;
  }

 BTR_DOUBLE *p = (BTR_DOUBLE *) fieldPtr(name);
 if (p == 0)
  return 0.0;

 return *p;
}

BTR_DOUBLE DAT::fieldDecimal(BTR_CHAR *name)
{
 BTR_CHAR strbuf[256];
 BTR_CHAR *p1;

 if (fieldType(name) != BTR_TYPE_DECIMAL)
  {
   Btrieve::debugView(1,"fieldDecimal: Invalid field type\n");
   return 0;
  }

 BTR_BYTE *p = (BTR_BYTE *) fieldPtr(name);
 if (p == 0)
  return 0.0;

 BTR_WORD l = fieldLen(name);

 memset(strbuf,0,sizeof(strbuf));
 p1 = strbuf;

 switch (p[l-1]&0x0F)
  {
   case 0x0F:
   case 0x0C:
    break;
   case 0x0D:
    *p1 = '-';
    break;
   default:
    Btrieve::debugView(1,"fieldDecimal: Unknown sign\n");
    return 0.0;
  }

 for (BTR_SINT i=0;i<l;i++)
  {
   *p1 = (BTR_CHAR)('0' + (p[i]>>4));
   p1++;
   if (i < l-1)
    {
     *p1 = (BTR_CHAR)('0' + (p[i]&0x0F));
     p1++;
    }
  }

 sprintf(p1,"E-%d",fieldDec(name));

 return atof(strbuf);
}

BTR_VOID DAT::clear(BTR_VOID)
{
 recVarLen = recBufLen;
 memset(recBuf,0,recBufLen);
 for (BTR_SINT f=0;f<fieldNum;f++)
  if (field[f].type == BTR_TYPE_STRING)
   memset(recBuf+field[f].position,' ',field[f].length);
}

/********************************************************************
 assignXXX
********************************************************************/
BTR_SINT DAT::assignString(BTR_CHAR *name,BTR_CHAR *value)
{
 BTR_WORD len;

 BTR_CHAR *p = (BTR_CHAR *) fieldPtr(name);
 if (p == 0)
  return BTR_ERR_UNKNOWN;

 len = fieldLen(name);
 memset(p,' ',len);

 if ((BTR_WORD)strlen(value) < len)
  len = (BTR_WORD) strlen(value);

 memcpy(p,value,len);

 return 0;
}

BTR_SINT DAT::assignChar(BTR_CHAR *name,BTR_SINT value)
{
 if (fieldType(name) != BTR_TYPE_STRING &&
     fieldType(name) != BTR_TYPE_INTEGER)
  {
   Btrieve::debugView(1,"assignChar: Invalid field type\n");
   return 0;
  }

 if (fieldLen(name) != 1)
  {
   Btrieve::debugView(1,"assignChar: Invalid field size\n");
   return 0;
  }

 BTR_CHAR *p = (BTR_CHAR *) fieldPtr(name);
 if (p == 0)
  return BTR_ERR_UNKNOWN;

 *p = value;

 return 0;
}

BTR_SINT DAT::assignInt(BTR_CHAR *name,BTR_SINT value)
{
 if (fieldType(name) != BTR_TYPE_INTEGER &&
     fieldType(name) != BTR_TYPE_UNSIGNED)
  {
   Btrieve::debugView(1,"assignInt: Invalid field type\n");
   return 0;
  }

 if (fieldLen(name) != 2)
  {
   Btrieve::debugView(1,"assignInt: Invalid field size\n");
   return 0;
  }

 BTR_SINT *p = (BTR_SINT *) fieldPtr(name);
 if (p == 0)
  return BTR_ERR_UNKNOWN;

 *p = value;

 return 0;
}

BTR_SINT DAT::assignLong(BTR_CHAR *name,BTR_LONG value)
{
 if (fieldType(name) != BTR_TYPE_INTEGER)
  {
   Btrieve::debugView(1,"assignLong: Invalid field type\n");
   return 0;
  }

 if (fieldLen(name) != 4)
  {
   Btrieve::debugView(1,"assignLong: Invalid field size\n");
   return 0;
  }

 BTR_LONG *p = (BTR_LONG *) fieldPtr(name);
 if (p == 0)
  return BTR_ERR_UNKNOWN;

 *p = value;

 return 0;
}

BTR_SINT DAT::assignDouble(BTR_CHAR *name,BTR_DOUBLE value)
{
 if (fieldType(name) != BTR_TYPE_FLOAT)
  {
   Btrieve::debugView(1,"assignDouble: Invalid field type\n");
   return 0;
  }

 if (fieldLen(name) != 8)
  {
   Btrieve::debugView(1,"assignDouble: Invalid field size\n");
   return 0;
  }

 BTR_DOUBLE *p = (BTR_DOUBLE *) fieldPtr(name);
 if (p == 0)
  return BTR_ERR_UNKNOWN;

 *p = value;

 return 0;
}

BTR_SINT DAT::assignDecimal(BTR_CHAR *name,BTR_DOUBLE value)
{
 BTR_CHAR strbuf[50];

 if (fieldType(name) != BTR_TYPE_DECIMAL)
  {
   Btrieve::debugView(1,"assignDecimal: Invalid field type\n");
   return 0;
  }

 BTR_CHAR *p = (BTR_CHAR *) fieldPtr(name);
 if (p == 0)
  return BTR_ERR_UNKNOWN;
 BTR_SINT l = fieldLen(name);
 BTR_SINT d = fieldDec(name);

 sprintf(strbuf,"%+0*.*f",2*l+1,d,value);

 memset(p,0,l);

 BTR_SINT i=0;
 for (BTR_CHAR *ps=strbuf+1;*ps;ps++)
  {
   if (*ps == '.')
    continue;
   if (i%2 == 0)
    p[i/2] |= (BTR_CHAR)((*ps-'0')<<4);
   else
    p[i/2] |= (BTR_CHAR)(*ps-'0');
   i++;
  }

 switch (strbuf[0])
  {
   case '+':
    p[l-1] |= 0x0F;
    break;
   case '-':
    p[l-1] |= 0x0D;
    break;
  }

 return 0;
}

/********************************************************************
 rec2key
********************************************************************/
BTR_VOID DAT::rec2key(BTR_SINT key)
{
 BTR_WORD position;

 Btrieve::debugView(100,"DAT::rec2key\n");

 position = 0;
 for (BTR_SINT s=0;s<segNum;s++)
  if (seg[s].key == key)
   {
    memcpy(keyBuf+position,recBuf+seg[s].position,seg[s].length);
    position += seg[s].length;
   }
}

/********************************************************************
 getE
********************************************************************/
BTR_SINT DAT::getE(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::getE\n");

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

 if (key == BTR_CONST_NOKEY)
  {
   Btrieve::debugView(1,"getE: Invalid key number\n");
   isEof = 1;
   return BTR_ERR_INVALIDKEY;
  }

 rec2key(key);

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_GET_E,pos,recBuf,&len,keyBuf,key);
 if (error && error!=BTR_ERR_KEYNOTFOUND)
  Btrieve::debugView(1,"getE: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_KEYNOTFOUND)
  isEof = 1;

 return error;
}

/********************************************************************
 getGE
********************************************************************/
BTR_SINT DAT::getGE(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::getGE\n");

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

 if (key == BTR_CONST_NOKEY)
  {
   Btrieve::debugView(1,"getGE: Invalid key number\n");
   isEof = 1;
   return BTR_ERR_INVALIDKEY;
  }

 rec2key(key);

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_GET_GE,pos,recBuf,&len,keyBuf,key);
 if (error && error!=BTR_ERR_EOF)
  Btrieve::debugView(1,"getGE: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_EOF)
  isEof = 1;

 return error;
}

/********************************************************************
 getL
********************************************************************/
BTR_SINT DAT::getL(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::getL\n");

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

 if (key == BTR_CONST_NOKEY)
  {
   Btrieve::debugView(1,"getL: Invalid key number\n");
   isEof = 1;
   return BTR_ERR_INVALIDKEY;
  }

 rec2key(key);

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_GET_L,pos,recBuf,&len,keyBuf,key);
 if (error && error!=BTR_ERR_EOF)
  Btrieve::debugView(1,"getL: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_EOF)
  isEof = 1;

 return error;
}

/********************************************************************
 getFirst
********************************************************************/
BTR_SINT DAT::getFirst(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::getFirst\n");

 if (!isOpened)
  {
   Btrieve::debugView(1,"getFirst: File not open\n");
   return BTR_ERR_UNKNOWN;
  }

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

 if (key == BTR_CONST_NOKEY)
  {
   Btrieve::debugView(1,"getFirst: Invalid key number\n");
   isEof = 1;
   return BTR_ERR_INVALIDKEY;
  }

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_GET_FIRST,pos,recBuf,&len,keyBuf,key);
 if (error && error!=BTR_ERR_EOF)
  Btrieve::debugView(1,"getFirst: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_EOF)
  isEof = 1;

 return error;
}

/********************************************************************
 getLast
********************************************************************/
BTR_SINT DAT::getLast(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::getLast\n");

 if (!isOpened)
  {
   Btrieve::debugView(1,"getLast: File not open\n");
   return BTR_ERR_UNKNOWN;
  }

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

 if (key == BTR_CONST_NOKEY)
  {
   Btrieve::debugView(1,"getLast: Invalid key number\n");
   isEof = 1;
   return BTR_ERR_INVALIDKEY;
  }

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_GET_LAST,pos,recBuf,&len,keyBuf,key);
 if (error && error!=BTR_ERR_EOF)
  Btrieve::debugView(1,"getLast: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_EOF)
  isEof = 1;

 return error;
}

/********************************************************************
 getNext
********************************************************************/
BTR_SINT DAT::getNext(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::getNext\n");

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

 if (key == BTR_CONST_NOKEY)
  {
   Btrieve::debugView(1,"getNext: Invalid key number");
   isEof = 1;
   return BTR_ERR_INVALIDKEY;
  }

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_GET_NEXT,pos,recBuf,&len,keyBuf,key);
 if (error && error!=BTR_ERR_EOF)
  Btrieve::debugView(1,"getNext: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_EOF)
  isEof = 1;

 return error;
}

/********************************************************************
 stepFirst
********************************************************************/
BTR_SINT DAT::stepFirst(BTR_VOID)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::stepFirst\n");

 if (!isOpened)
  {
   Btrieve::debugView(1,"stepFirst: File not open\n");
   return BTR_ERR_UNKNOWN;
  }

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_STEP_FIRST,pos,recBuf,&len,keyBuf,BTR_CONST_NOKEY);
 if (error && error!=BTR_ERR_EOF)
  Btrieve::debugView(1,"stepFirst: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_EOF)
  isEof = 1;

 return error;
}

/********************************************************************
 stepNext
********************************************************************/
BTR_SINT DAT::stepNext(BTR_VOID)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::stepNext\n");

 if (!isOpened)
  {
   Btrieve::debugView(1,"stepNext: File not open\n");
   return BTR_ERR_UNKNOWN;
  }

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_STEP_NEXT,pos,recBuf,&len,keyBuf,BTR_CONST_NOKEY);
 if (error && error!=BTR_ERR_EOF)
  Btrieve::debugView(1,"stepNext: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_EOF)
  isEof = 1;

 return error;
}

/********************************************************************
 stepLast
********************************************************************/
BTR_SINT DAT::stepLast(BTR_VOID)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::stepLast\n");

 if (!isOpened)
  {
   Btrieve::debugView(1,"stepLast: File not open\n");
   return BTR_ERR_UNKNOWN;
  }

 len = recBufLen;
 isEof = 0;
 error = BTRV(BTR_OP_STEP_LAST,pos,recBuf,&len,keyBuf,BTR_CONST_NOKEY);
 if (error && error!=BTR_ERR_EOF)
  Btrieve::debugView(1,"stepLast: Error %d\n",error);
 recVarLen = len;

 if (error == BTR_ERR_EOF)
  isEof = 1;

 return error;
}

/********************************************************************
 insert
********************************************************************/
BTR_SINT DAT::insert(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::insert\n");

 if (!isOpened)
  {
   Btrieve::debugView(1,"insert: File not open\n");
   return BTR_ERR_UNKNOWN;
  }

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

// if (key == BTR_CONST_NOKEY)
//  {
//   Btrieve::debugView("insert: Invalid key number");
//   isEof = 1;
//   return BTR_ERR_INVALIDKEY;
//  }

 len = recVarLen;
 error = BTRV(BTR_OP_INSERT,pos,recBuf,&len,keyBuf,key);
 if (error)
  Btrieve::debugView(1,"insert: Error %d %s\n",error,fileName);

 return error;
}

/********************************************************************
 update
********************************************************************/
BTR_SINT DAT::update(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::update\n");

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

// if (key == BTR_CONST_NOKEY)
//  {
//   Btrieve::debugView("insert: Invalid key number");
//   isEof = 1;
//   return BTR_ERR_INVALIDKEY;
//  }

 len = recVarLen;
 error = BTRV(BTR_OP_UPDATE,pos,recBuf,&len,keyBuf,key);
 if (error)
  Btrieve::debugView(1,"update: Error %d %s\n",error,fileName);

 return error;
}

/********************************************************************
 del
********************************************************************/
BTR_SINT DAT::del(BTR_SINT key)
{
 BTR_SINT error;
 BTR_WORD len;

 Btrieve::debugView(100,"DAT::del\n");

 if (key == BTR_CONST_NOKEY)
  key = currentKey;

// if (key == BTR_CONST_NOKEY)
//  {
//   Btrieve::debugView("insert: Invalid key number");
//   isEof = 1;
//   return BTR_ERR_INVALIDKEY;
//  }

 len = recBufLen;
 error = BTRV(BTR_OP_DELETE,pos,recBuf,&len,keyBuf,key);
 if (error)
  Btrieve::debugView(1,"del: Error %d\n",error);

 return error;
}

