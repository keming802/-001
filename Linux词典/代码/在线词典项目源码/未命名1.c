#include <stdio.h>

int main()
{
   FILE *fp;
   char str[512];

   /* �����ڶ�ȡ���ļ� */
   fp = fopen("dict.txt" , "r");
   if(fp == NULL) {
      perror("���ļ�ʱ��������");
      return(-1);
   }
   if( fgets (str, 512, fp)!=NULL ) {
      /* ���׼��� stdout д������ */
      puts(str);
   }
   fclose(fp);
   
   return(0);
}
