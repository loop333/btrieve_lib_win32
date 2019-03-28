#if !defined(__BTRIEVE_H)
#define __BTRIEVE_H

/*************************************************************
 Constants
*************************************************************/
#define BTR_CONST_MAXKEYSIZE    255
#define BTR_CONST_MAXSEGNUM     119
#define BTR_CONST_POSLEN        128
#define BTR_CONST_NOKEY          -1
#define BTR_CONST_ACSSIZE       265
#define BTR_CONST_NAMELEN        20
#define BTR_CONST_FLDNAMELEN     20
#define BTR_CONST_FILENAMELEN   256
#define BTR_CONST_MAXINFO         5

/*************************************************************
 Types
*************************************************************/
#define BTR_TYPE_STRING       0
#define BTR_TYPE_INTEGER      1
#define BTR_TYPE_FLOAT        2
#define BTR_TYPE_DATE         3
#define BTR_TYPE_TIME         4
#define BTR_TYPE_DECIMAL      5
#define BTR_TYPE_MONEY        6
#define BTR_TYPE_LOGICAL      7
#define BTR_TYPE_NUMERIC      8
#define BTR_TYPE_BFLOAT       9
#define BTR_TYPE_LSTRING     10
#define BTR_TYPE_ZSTRING     11
#define BTR_TYPE_MEMO        12
//#define BTR_TYPE_13        13
#define BTR_TYPE_UNSIGNED    14
#define BTR_TYPE_AUTOINC     15
//#define BTR_TYPE_16        16
#define BTR_TYPE_STS         17
#define BTR_TYPE_UNKNOWN     -1

/*************************************************************
 Operations
*************************************************************/
#define BTR_OP_OPEN               0
#define BTR_OP_CLOSE              1
#define BTR_OP_INSERT             2
#define BTR_OP_UPDATE             3
#define BTR_OP_DELETE             4
#define BTR_OP_GET_E              5
#define BTR_OP_GET_NEXT           6
#define BTR_OP_GET_PREV           7
#define BTR_OP_GET_G              8
#define BTR_OP_GET_GE             9
#define BTR_OP_GET_L             10
#define BTR_OP_GET_LE            11
#define BTR_OP_GET_FIRST         12
#define BTR_OP_GET_LAST          13
#define BTR_OP_CREATE            14
#define BTR_OP_STATUS            15
#define BTR_OP_EXTEND            16
#define BTR_OP_SET_DIR           17
#define BTR_OP_GET_DIR           18
#define BTR_OP_BEGIN_TRAN        19
#define BTR_OP_END_TRAN          20
#define BTR_OP_ABORT_TRAN        21
#define BTR_OP_GET_POS           22
#define BTR_OP_GET_DIRECT        23
#define BTR_OP_STEP_NEXT         24
#define BTR_OP_STOP              25
#define BTR_OP_VERSION           26
#define BTR_OP_UNLOCK            27
#define BTR_OP_RESET             28
#define BTR_OP_SET_OWNER         29
#define BTR_OP_CLEAR_OWNER       30
#define BTR_OP_CREATE_INDEX      31
#define BTR_OP_DROP_INDEX        32
#define BTR_OP_STEP_FIRST        33
#define BTR_OP_STEP_LAST         34
#define BTR_OP_STEP_PREV         35
#define BTR_OP_GET_NEXT_EXT      36
#define BTR_OP_GET_PREV_EXT      37
#define BTR_OP_STEP_NEXT_EXT     38
#define BTR_OP_STEP_PREV_EXT     39
#define BTR_OP_INSERT_EXT        40

/*************************************************************
 File flags
*************************************************************/
#define BTR_FILE_VAR      0x0001
#define BTR_FILE_COMPRESS 0x0002
#define BTR_FILE_PREALLOC 0x0004
//#define BTR_FILE_ODBC     0x10
#define BTR_FILE_BALANCED 0x0020
#define BTR_FILE_0100     0x0100

/*************************************************************
 Key flags
*************************************************************/
#define BTR_KEY_DUP          0x0001
#define BTR_KEY_MOD          0x0002
#define BTR_KEY_BIN          0x0004
#define BTR_KEY_NUL          0x0008
#define BTR_KEY_SEG          0x0010
#define BTR_KEY_ALT          0x0020
#define BTR_KEY_DESC         0x0040
#define BTR_KEY_REP          0x0080
#define BTR_KEY_EXT          0x0100
#define BTR_KEY_MAN          0x0200
#define BTR_KEY_NOCASE       0x0400
#define BTR_KEY_ACSNAME      0x0800
#define BTR_KEY_KEYONLY      0x4000
#define BTR_KEY_PENDING      0x8000

/*************************************************************
 Errors
*************************************************************/
#define BTR_ERR_KEYNOTFOUND       4
#define BTR_ERR_INVALIDKEY        6
#define BTR_ERR_EOF               9
#define BTR_ERR_CANTFIND         12
#define BTR_ERR_NOTLOADED        20
#define BTR_ERR_CANTCREATE       25
#define BTR_ERR_UNKNOWN        5000
#define BTR_ERR_NOMEMORY       5001

/*************************************************************
 Data types
*************************************************************/
typedef void           BTR_VOID;
typedef char           BTR_CHAR;
typedef char           BTR_BOOL;
typedef unsigned char  BTR_BYTE;
typedef short          BTR_SINT;
typedef unsigned short BTR_WORD;
typedef long           BTR_LONG;
typedef unsigned long  BTR_DWORD;
typedef double         BTR_DOUBLE;

#ifdef __WIN32__
#pragma pack(push,1)
#endif
struct BTR_VERSION
{
 BTR_SINT Version;
 BTR_SINT Revision;
 BTR_CHAR MKDEId;
};

struct BTR_FILE
{
 BTR_WORD  recLen;          //  Record length
 BTR_WORD  pageSize;        //  Page size
 BTR_WORD  keyNum;          //  Number of keys
 BTR_DWORD recNum;          //  Number of records
 BTR_WORD  flags;           //  File flags
 BTR_BYTE  dupPointers;     //  ???
 BTR_BYTE  reserved;        //  ???
 BTR_WORD  allocations;     //  ???
};

struct BTR_SEG
{
 BTR_WORD  position;        // Key position in record buffer
 BTR_WORD  length;          // Key length 
 BTR_WORD  flags;           // Key flags
 BTR_DWORD keyNum;          // Number of keys
 BTR_BYTE  type;            // Key type
 BTR_WORD  nulval;          // Null value
 BTR_BYTE  unknown1;        // ???
 BTR_BYTE  unknown2;        // ???
 BTR_BYTE  acs;             // ACS Number
};

struct BTR_STATUS
{
 BTR_FILE file;
 BTR_SEG  seg[BTR_CONST_MAXSEGNUM];
 BTR_CHAR acs[BTR_CONST_ACSSIZE]; // Add space if ACS
};
#ifdef __WIN32__
#pragma pack(pop)
#endif

/*************************************************************
 BTRV
*************************************************************/
extern BTR_SINT BTRV(BTR_WORD operation,
                     BTR_VOID *posBlock,
                     BTR_VOID *dataBuffer,
                     BTR_WORD *dataLength,
                     BTR_VOID *keyBuffer,
                     BTR_SINT keyNumber);

/*************************************************************
 FIELDINFO
*************************************************************/
struct FIELDINFO
{
// BTR_CHAR name[BTR_CONST_FLDNAMELEN+1];
 BTR_CHAR *name;
 BTR_SINT type;
 BTR_WORD length;
 BTR_WORD dec;
};

/*************************************************************
 KEYINFO
*************************************************************/
struct KEYINFO
{
 BTR_CHAR *name;
};

struct FIELD;

/*************************************************************
 SEG
*************************************************************/
struct SEG
{
 BTR_WORD key;
 BTR_WORD position;
 BTR_WORD length;
 BTR_SINT type;
 BTR_WORD flags;
 BTR_WORD nulval;
 BTR_WORD acs;
 FIELD    *field;
};

/*************************************************************
 FIELD
*************************************************************/
struct FIELD
{
// BTR_CHAR name[BTR_CONST_FLDNAMELEN+1];
 BTR_CHAR *name;
 BTR_WORD position;
 BTR_WORD length;
 BTR_WORD dec;
 BTR_SINT type;
};

/*************************************************************
 Btrieve
*************************************************************/
class Btrieve
{
public:
 static BTR_WORD  infoNum;
 static FIELDINFO *info[BTR_CONST_MAXINFO];
 static BTR_WORD  defPageSize;
 static BTR_WORD  defFlags;

public:
 Btrieve(BTR_VOID);
 ~Btrieve(BTR_VOID);
 static BTR_BOOL isLoaded;
 static BTR_SINT debug;

 static BTR_BOOL  loaded(BTR_VOID) {return isLoaded;};
 static BTR_VOID  debugView(BTR_SINT l,BTR_CHAR *s,...);
 static BTR_VOID  CreateDDF(BTR_CHAR *path);
 static BTR_VOID  AddToDDF(BTR_CHAR *path,BTR_CHAR *name,BTR_CHAR *loc,
                           FIELDINFO *field,KEYINFO *key,BTR_WORD aFlags=0);
 static FIELDINFO *loadField(BTR_CHAR *ddfpath,BTR_CHAR *name);
 static BTR_SINT  create(BTR_CHAR *name,
                         FIELDINFO *field,
                         KEYINFO *key,
                         BTR_VOID *acs=0);
 static BTR_SINT  BeginTransaction(BTR_VOID);
 static BTR_SINT  EndTransaction(BTR_VOID);
 static BTR_SINT  AbortTransaction(BTR_VOID);
 static BTR_CHAR  *type(BTR_SINT type);
};

/*************************************************************
 DAT
*************************************************************/
class DAT
{
public:
 BTR_CHAR  *fileName;
 BTR_CHAR  pos[BTR_CONST_POSLEN];
 BTR_BOOL  isOpened;
 BTR_WORD  segNum;
 BTR_WORD  keyNum;
 BTR_WORD  fieldNum;
 SEG       *seg;
 FIELD     *field;
 BTR_BOOL  isEof;
 BTR_SINT  currentKey;
 BTR_CHAR  *recBuf;
 BTR_CHAR  *keyBuf;
 BTR_WORD  recLen;
 BTR_WORD  recBufLen;
 BTR_WORD  recVarLen;
 BTR_WORD  pageSize;
 BTR_WORD  flags;
 BTR_DWORD recNum;

 DAT(BTR_VOID);
 DAT(BTR_CHAR *name,BTR_CHAR *owner=0);
 ~DAT(BTR_VOID);

 BTR_VOID   init(BTR_VOID);
 BTR_SINT   open(BTR_CHAR *name,BTR_CHAR *owner=0);
 BTR_SINT   close(BTR_VOID);
 BTR_SINT   status(BTR_VOID);
 BTR_SINT   setField(FIELDINFO *info);
 BTR_SINT   loadField(BTR_CHAR *path,BTR_CHAR *aname=0);
 BTR_VOID   keySelect(BTR_SINT key);
 BTR_VOID   keySelect(BTR_CHAR *name);
 BTR_VOID   viewFile(BTR_VOID);
 BTR_VOID   viewHeader(BTR_VOID);
 BTR_VOID   viewFields(BTR_VOID);
 BTR_VOID   viewKeys(BTR_VOID);
 BTR_VOID   viewSegs(BTR_VOID);
 BTR_VOID   viewRecord(BTR_VOID);
 BTR_VOID   viewField(BTR_CHAR *name);
 BTR_BOOL   eof(BTR_VOID);
 BTR_BOOL   opened(BTR_VOID);

 BTR_WORD   fieldOffset(BTR_CHAR *name);
 BTR_VOID   *fieldPtr(BTR_CHAR *name);
 BTR_CHAR   *fieldString(BTR_CHAR *name);
 BTR_WORD   fieldLen(BTR_CHAR *name);
 BTR_WORD   fieldMemoLen(BTR_CHAR *name,BTR_WORD offset=0,BTR_WORD len=0);
 BTR_WORD   fieldDec(BTR_CHAR *name);
 BTR_SINT   fieldType(BTR_CHAR *name);
 BTR_CHAR   *fieldName(BTR_CHAR *name);
 BTR_CHAR   *fieldZString(BTR_CHAR *name);
 BTR_CHAR   *fieldZMemo(BTR_CHAR *name,BTR_WORD offset=0,BTR_WORD len=0);
 BTR_SINT   fieldChar(BTR_CHAR *name);
 BTR_SINT   fieldInt(BTR_CHAR *name);
 BTR_LONG   fieldLong(BTR_CHAR *name);
 BTR_DOUBLE fieldDouble(BTR_CHAR *name);
 BTR_DOUBLE fieldDecimal(BTR_CHAR *name);

 BTR_VOID   clear(BTR_VOID);
 BTR_SINT   assignString(BTR_CHAR *name,BTR_CHAR *value);
 BTR_SINT   assignChar(BTR_CHAR *name,BTR_SINT value);
 BTR_SINT   assignInt(BTR_CHAR *name,BTR_SINT value);
 BTR_SINT   assignLong(BTR_CHAR *name,BTR_LONG value);
 BTR_SINT   assignDouble(BTR_CHAR *name,BTR_DOUBLE value);
 BTR_SINT   assignDecimal(BTR_CHAR *name,BTR_DOUBLE value);

 BTR_VOID   rec2key(BTR_SINT key);

 BTR_SINT   getFirst(BTR_SINT key=BTR_CONST_NOKEY);
 BTR_SINT   getLast(BTR_SINT key=BTR_CONST_NOKEY);
 BTR_SINT   getNext(BTR_SINT key=BTR_CONST_NOKEY);
 BTR_SINT   getE(BTR_SINT key=BTR_CONST_NOKEY);
 BTR_SINT   getL(BTR_SINT key=BTR_CONST_NOKEY);
 BTR_SINT   getGE(BTR_SINT key=BTR_CONST_NOKEY);
 BTR_SINT   stepFirst(BTR_VOID);
 BTR_SINT   stepNext(BTR_VOID);
 BTR_SINT   stepLast(BTR_VOID);
 BTR_SINT   insert(BTR_SINT key=BTR_CONST_NOKEY);
 BTR_SINT   update(BTR_SINT key=BTR_CONST_NOKEY);
 BTR_SINT   del(BTR_SINT key=BTR_CONST_NOKEY);
};

extern FIELDINFO file_field[];
extern FIELDINFO field_field[];
extern FIELDINFO index_field[];
extern FIELDINFO comment_field[];

extern KEYINFO file_key[];
extern KEYINFO field_key[];
extern KEYINFO index_key[];
extern KEYINFO comment_key[];


#if !defined(__WIN32__)

#define TYPE_INTEGER  BTR_TYPE_INTEGER
#define TYPE_STRING   BTR_TYPE_STRING
#define TYPE_UNKNOWN  BTR_TYPE_UNKNOWN
#define TYPE_UNSIGNED BTR_TYPE_UNSIGNED
#define TYPE_FLOAT    BTR_TYPE_FLOAT

#define SIZE_INT  2
#define SIZE_LONG 4

typedef FIELDINFO FIELD_INFO;

#endif

#endif

