#include <stdio.h>

int main()
{
   FILE *fp;
   char str[512];

   /* 打开用于读取的文件 */
   fp = fopen("dict.txt" , "r");
   if(fp == NULL) {
      perror("打开文件时发生错误");
      return(-1);
   }
   if( fgets (str, 512, fp)!=NULL ) {
      /* 向标准输出 stdout 写入内容 */
      puts(str);
   }
   fclose(fp);
   
   return(0);
}
