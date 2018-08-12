
#ifndef __ENCODE_CONVERT_H__
#define __ENCODE_CONVERT_H__



int convert(const char *from, const char *to, char* save, int savelen, char *src, int srclen);

int getTextEncode(char *specimens, int specimens_len, char *result);
int getFileEncode(char *filename, char *result);


#endif /* !__ENCODE_CONVERT_H__ */
