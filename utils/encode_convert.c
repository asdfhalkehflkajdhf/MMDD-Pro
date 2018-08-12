#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
 
#include <iconv.h>
#include "utils.h"

#include "uchardet.h"
/*
Supported Languages/Encodings

International (Unicode)
	UTF-8
	UTF-16BE / UTF-16LE
	UTF-32BE / UTF-32LE / X-ISO-10646-UCS-4-34121 / X-ISO-10646-UCS-4-21431
Arabic
	ISO-8859-6
	WINDOWS-1256
Bulgarian
	ISO-8859-5
	WINDOWS-1251
Chinese
	ISO-2022-CN
	BIG5
	EUC-TW
	GB18030
	HZ-GB-2312
Danish
	ISO-8859-1
	ISO-8859-15
	WINDOWS-1252
English
	ASCII
Esperanto
	ISO-8859-3
French
	ISO-8859-1
	ISO-8859-15
	WINDOWS-1252
German
	ISO-8859-1
	WINDOWS-1252
Greek
	ISO-8859-7
	WINDOWS-1253
Hebrew
	ISO-8859-8
	WINDOWS-1255
Hungarian:
	ISO-8859-2
	WINDOWS-1250
Japanese
	ISO-2022-JP
	SHIFT_JIS
	EUC-JP
Korean
	ISO-2022-KR
	EUC-KR
Russian
	ISO-8859-5
	KOI8-R
	WINDOWS-1251
	MAC-CYRILLIC
	IBM866
	IBM855
Spanish
	ISO-8859-1
	ISO-8859-15
	WINDOWS-1252
Thai
	TIS-620
	ISO-8859-11
Turkish:
	ISO-8859-3
	ISO-8859-9
Vietnamese:
	VISCII
	Windows-1258
Others
	WINDOWS-1252
*/




/*!
���ַ����������Ա���ת��
param from  ԭʼ���룬����"GB2312",�İ���iconv֧�ֵ�д
param to      ת����Ŀ�ı���
param save  ת��������ݱ��浽���ָ�����Ҫ���ⲿ�����ڴ�
param savelen �洢ת�������ݵ��ڴ��С
param src      ԭʼ��Ҫת�����ַ���
param srclen    ԭʼ�ַ�������
*/
int
convert(const char *from, const char *to, char* save, int savelen, char *src, int srclen)
{
	iconv_t cd;
	char **inbuf = &src;
	char **outbuf = &save;
	size_t outbufsize = savelen;
	size_t inbufsize = srclen;
	
	int status = 0;

	cd = iconv_open(to, from);

	size_t res = iconv(cd, inbuf, &inbufsize, outbuf, &outbufsize);
	

	if (res == (size_t)(-1)) {
		printf("iconv_conv error for [%s]\n",strerror(errno));
		goto done;
	}



done:
	status = strlen(save);
	iconv_close(cd);
	return status;
}


/* �������� */  
#define NUMBER_OF_SAMPLES   (2048)  

int getTextEncode(char *specimens, int specimens_len, char *result)
{
    uchardet_t ud;
    /* ͨ�������ַ������ı����� */  
    ud = uchardet_new();  
    if(uchardet_handle_data(ud, specimens, specimens_len) != 0) /* ��������ַ���������ô�п��ܵ��·���ʧ�� */  
    {  
        printf("��������ʧ�ܣ�\n");  
        return -1;  
    }  
    uchardet_data_end(ud);  
    strcpy(result, uchardet_get_charset(ud));
    uchardet_delete(ud);
	return 0;
}

int getFileEncode(char *filename, char *result)
{
    FILE *fp = fopen(filename, "r");

	char buf[1024]={0};
	int len;
		
    if(fp == NULL)
    {
        return -1;
    }
    len = fread(buf, sizeof(unsigned char), 1024, fp);
    fclose(fp);
	return getTextEncode(buf, len, result);

}

#if 0
int main(int argc, char* argv[])  
{
	//�ļ���С
	int size;
	//�ļ���
	char url[256]={0};
	char title[256]={0};

	char *filePath;
	
	char *buf;
    int sts;
    FILE *pf;

	if(argc !=2)
		return 0;
	
	filePath = argv[1];
	
	size = get_file_size(filePath);
	
    size_t result; //����ֵ�Ƕ�ȡ����������
    pf = fopen(filePath,"r");
    if (pf == NULL) {
        printf(" Open file error\n");
     
        return -1;
    }

	fgets(url, 256, pf);
	fgets(title, 256, pf);
	
	int ret;
	char textEncode[20]={0};
	ret = getTextEncode(title, strlen(title), textEncode);
	if(ret)
		strcpy(textEncode, "GB2312");

	
	printf("encode = %s\n", textEncode);
	printf("title =%s\n\n", title);
	printf("url   =%s\n\n", url);
	
	char dst_url[256]={0};
	char dst_title[256]={0};
	
	
	
	convert(textEncode, "UTF-8", dst_url, 256, url, 256);
//	convert("GB2312", "UTF-8", dst_url, 256, url, 256);
	convert(textEncode, "UTF-8", dst_title, 256, title, 256);
	printf("dst_url     =%s\n\n", dst_url);
	printf("dst_title   =%s\n\n", dst_title);
	
	
   // ��ȡ�ļ���С
   // rewind(pf); // ����rewind()���ļ�ָ���Ƶ���stream(��)ָ���Ŀ�ʼ��, ͬʱ���������صĴ����EOF���  
    buf = (char*)malloc(sizeof(char)*size);
    char *dst_buf = (char*)malloc(sizeof(char)*size);
	
    result = fread(buf,1,size,pf);
    if (result <0) {
        printf(" Read file error result[%d] size[%d]\n", result, size);
     
		fclose(pf);
		free(buf);
		free(dst_buf);
        return -1;
    }
	
	convert(textEncode, "UTF-8", dst_buf, size, buf, size);
	printf("buf   =%s\n\n", dst_buf);

 
    fclose(pf);
	free(buf);
    free(dst_buf);
    return 0;

}  
#endif
