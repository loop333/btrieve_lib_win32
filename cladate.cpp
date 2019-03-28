#include <stdio.h>
#include <mem.h>
#include <stdlib.h>
#include <string.h>

#include <cladate.h>

int nday[] = {31,28,31,30,31,30,31,31,30,31,30,31};

/****************************************************************
 date2dmy
****************************************************************/
void date2dmy(long d,int *day,int *month,int *year)
{
 int isLeap = 0;

 *day   = 0;
 *month = 0;
 *year  = 0;

 if ((d < 4) || (d > 109211)) // 01.01.1801 - 31.12.2099
  return;

 if (d > 36527)  // 31.12.1900
  d -= 3;
 else
  d -= 4;

 *year = 1801 + 4*(d/1461);
 d %= 1461;
 if (d != 1460)
  {
   *year += d/365;
   *day = d % 365;
  }
 else
  {
   *year += 3;
   *day = 365;
  }

 if (*year < 100)
  *year += 1900;

 if (((*year%4==0) && (*year%100!=0)) || (*year%400==0))
  isLeap = 1;

 for (int i=0;i<12;i++)
  {
   if (i == 1)
    *day -= isLeap;
   *day -= nday[i];
   if (*day < 0)
    {
     if (i == 1)
      *day += isLeap;
     *day += nday[i] + 1;
     *month = i + 1;
     break;
    }
  }
}

/****************************************************************
 dmy2date
****************************************************************/
long dmy2date(int dd,int mm,int yy)
{
 int  isLeap = 0;
 long d;

 if (dd==0 || mm==0 || yy==0)
  return 0;

 d = dd;

 if (((yy%4==0) && (yy%100!=0)) || (yy%400==0))
  isLeap = 1;

 for (int i=0;i<mm-1;i++)
  {
   if (i == 1)
    d += isLeap;
   d += nday[i];
  }

 yy -= 1;

 d += yy*365L + yy/4L - yy/100L + yy/400L - 657433;

 return d;
}

/****************************************************************
 date2day
****************************************************************/
int date2day(long d)
{
 int dd,mm,yy;

 date2dmy(d,&dd,&mm,&yy);
 return dd;
}

/****************************************************************
 date2month
****************************************************************/
int date2month(long d)
{
 int dd,mm,yy;

 date2dmy(d,&dd,&mm,&yy);
 return mm;
}

/****************************************************************
 date2year
****************************************************************/
int date2year(long d)
{
 int dd,mm,yy;

 date2dmy(d,&dd,&mm,&yy);
 return yy;
}

/****************************************************************
 date2dbf
****************************************************************/
void date2dbf(long d,char *s)
{
 int  dd,mm,yy;
 char tmp[9];

 date2dmy(d,&dd,&mm,&yy);
 sprintf(tmp,"%04d%02d%02d",yy,mm,dd);
 memcpy(s,tmp,8);
}

/****************************************************************
 dbf2date
****************************************************************/
long dbf2date(const char *s)
{
 char tmp[5];
 int  dd,mm,yy;

 memcpy(tmp,s,4);
 tmp[4] = 0;
 yy = atoi(tmp);

 memcpy(tmp,s+4,2);
 tmp[2] = 0;
 mm = atoi(tmp);

 memcpy(tmp,s+6,2);
 tmp[2] = 0;
 dd = atoi(tmp);

 return dmy2date(dd,mm,yy);
}

/****************************************************************
 str2date
****************************************************************/
long str2date(const char *str,const char *fmt)
{
 int        dd,mm,cc,yy;
 char       strtmp[3];
 const char *p;

 dd = mm = cc = yy = 0;

 if ((p=strstr(fmt,"DD")) != 0)
  {
   strncpy(strtmp,str+(p-fmt),2);
   strtmp[2] = 0;
   dd = atoi(strtmp);
  }

 if ((p=strstr(fmt,"MM")) != 0)
  {
   strncpy(strtmp,str+(p-fmt),2);
   strtmp[2] = 0;
   mm = atoi(strtmp);
  }

 if ((p=strstr(fmt,"CC")) != 0)
  {
   strncpy(strtmp,str+(p-fmt),2);
   strtmp[2] = 0;
   cc = atoi(strtmp);
  }

 if ((p=strstr(fmt,"YY")) != 0)
  {
   strncpy(strtmp,str+(p-fmt),2);
   strtmp[2] = 0;
   yy = atoi(strtmp);
  }

 if (yy && cc==0)
  {
   cc = 19;
   if (yy < 60)
    cc = 20;
  }

 return dmy2date(dd,mm,cc*100+yy);
}

/****************************************************************
 date2str
****************************************************************/
void date2str(char *str,long d,const char *fmt)
{
 int  dd,mm,yy;
 char *p;
 char strtmp[3];

 date2dmy(d,&dd,&mm,&yy);

 strcpy(str,fmt);

 if ((p=strstr(str,"DD")) != 0)
  {
   sprintf(strtmp,"%02d",dd);
   strncpy(p,strtmp,2);
  }

 if ((p=strstr(str,"MM")) != 0)
  {
   sprintf(strtmp,"%02d",mm);
   strncpy(p,strtmp,2);
  }

 if ((p=strstr(str,"CC")) != 0)
  {
   sprintf(strtmp,"%02d",yy/100);
   strncpy(p,strtmp,2);
  }

 if ((p=strstr(str,"YY")) != 0)
  {
   sprintf(strtmp,"%02d",yy%100);
   strncpy(p,strtmp,2);
  }
}

/****************************************************************
 time2hms
****************************************************************/
void time2hms(long t,int *hour,int *minute,int *seconds,int *hundreds)
{
 *hour     = 0;
 *minute   = 0;
 *seconds  = 0;
 *hundreds = 0;

 if ((t < 1) || (t > 8640000))
  return;

 t -= 1;
 *hour = t/360000;
 t %= 360000;
 *minute = t/6000;
 t %= 6000;
 *seconds = t/100;
 *hundreds %= 100;
}

/****************************************************************
 hms2time
****************************************************************/
long hms2time(int hour,int minute,int seconds,int hundreds)
{
 return ((hour*60L+minute)*60L+seconds)*100L+hundreds+1;
}

