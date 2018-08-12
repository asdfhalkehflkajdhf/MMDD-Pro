#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <zlib.h>
//#include <bzlib.h>
#include <sys/mman.h>
#include "conf.h"
#include "../archive/Archive/archive.h"
#include "../archive/Archive/archive_entry.h"
#include "../archive/Lzma/LzmaUtil.h"

#include "public.h"
#include "logging.h"
#include "utils.h"
#include "compress.h"

static char *g_gzip_type[UPLOAD_COMPRESS_MAX]={
	"gz",
//	"bz",
//	"bz2",
	"Z",
	"zip",
	
	"7z",
	"lzma",
	"tar",
	"tgz",
	"tar.gz",

	"tar.bz",
	"tar.bz2",
	"tar.Z"
};



int get_compress_type_id(char *str)
{
    if (str == NULL)
    {
        return NEWIUP_FAIL;
    }
	int i;
	for(i=0; i<UPLOAD_COMPRESS_MAX; i++){
	    if (0==strcmp(str, g_gzip_type[i]))
	    {
	        return i; 
	    }
	}

    return NEWIUP_FAIL;
}



char * get_compress_suffix_by_id(int index)
{
	if(index >= UPLOAD_COMPRESS_MAX)
		return NULL;
    return g_gzip_type[index];
}
/*
static int do_compress_bz2(char *buff, int buff_size, const char * DestName,const char *SrcName)
{    
	FILE * fp_in = NULL;
	int len = 0;
    int re = NEWIUP_OK;
	if( NULL == (fp_in = fopen(SrcName,"rb")))
	{
        return NEWIUP_FAIL;
	}
    BZFILE *out = BZ2_bzopen(DestName,"wb6f");
    if(out == NULL)
	{
        return NEWIUP_FAIL;
	}
    for(;;)	{
        len = fread(buff,1,buff_size,fp_in);
        if(ferror(fp_in)){
            re = NEWIUP_FAIL;
            break;
		}
        if(len == 0) break;
        if(BZ2_bzwrite(out, buff, (unsigned)len) != len)
		{
            re = NEWIUP_FAIL;
		}
	}
    BZ2_bzclose(out);
    fclose(fp_in);
    return re;
}
*/

static int do_compress_gz(char *buff, int buff_size, const char * DestName,const char *SrcName)
{
    FILE * fp_in = NULL;
	int len = 0;
    int re = NEWIUP_OK;
	if( NULL == (fp_in = fopen(SrcName,"rb"))){
        return NEWIUP_FAIL;
	}
    gzFile out = gzopen(DestName,"wb6f");
    if(out == NULL){
        return NEWIUP_FAIL;
	}
    for(;;){
        len = fread(buff,1,buff_size,fp_in);
        if(ferror(fp_in)){
            re = NEWIUP_FAIL;
            break;
		}
        if(len == 0) break;
        if(gzwrite(out, buff, (unsigned)len) != len){
            re = NEWIUP_FAIL;
		}
	}
    gzclose(out);
    fclose(fp_in);
    return re;
}



static int
do_compress_archive(int compress, char *buff, int buff_size,const char *outfile, const char *infile)
{
	struct archive *a = NULL;
	struct archive *disk = NULL;
	struct archive_entry *entry = NULL;
	ssize_t len;
	int fd;
	int ret = NEWIUP_FAIL;

	a = archive_write_new();
	switch (compress) {
//	case UPLOAD_COMPRESS_GZ:
	case UPLOAD_COMPRESS_TGZ:
	case UPLOAD_COMPRESS_TAR_GZ:
		archive_write_add_filter_gzip(a);
		archive_write_set_format_ustar(a);
		break;
//	case UPLOAD_COMPRESS_BZ:
	case UPLOAD_COMPRESS_TAR_BZ:
//	case UPLOAD_COMPRESS_BZ2:
	case UPLOAD_COMPRESS_TAR_BZ2:
		archive_write_add_filter_bzip2(a);
		archive_write_set_format_ustar(a);
		break;
	case UPLOAD_COMPRESS_Z:
	case UPLOAD_COMPRESS_TAR_Z:
		archive_write_add_filter_compress(a);
		archive_write_set_format_ustar(a);
		break;
	case UPLOAD_COMPRESS_ZIP:
		archive_write_add_filter_lzip(a);
		archive_write_set_format_zip(a);
		break;
	case UPLOAD_COMPRESS_7Z:
		archive_write_add_filter_lzip(a);
		archive_write_set_format_7zip(a);
		break;
	case UPLOAD_COMPRESS_TAR:
		archive_write_set_format_ustar(a);
		break;
	default:
		goto out;
		//archive_write_add_filter_none(a);
		break;
	}

	archive_write_open_filename(a, outfile);

	disk = archive_read_disk_new();
	int r;

	r = archive_read_disk_open(disk, infile);
	if (r != ARCHIVE_OK) {
		//logging(LOG_INFO,"do1 %s\n",archive_error_string(disk));
		goto out;
	}

	for (;;) {

		entry = archive_entry_new();
		r = archive_read_next_header2(disk, entry);
		if (r == ARCHIVE_EOF)
			break;
		if (r != ARCHIVE_OK) {
			//logging(LOG_INFO,"do2 %s\n",archive_error_string(disk));
			archive_entry_free(entry);
			goto out;
		}
		archive_read_disk_descend(disk);

		r = archive_write_header(a, entry);
		if (r < ARCHIVE_OK) {
			//logging(LOG_INFO,"%s\n",archive_error_string(disk));
		}
		if (r == ARCHIVE_FATAL){
			//logging(LOG_INFO,"%s\n",archive_error_string(disk));
			archive_entry_free(entry);
			goto out;
		}
		if (r > ARCHIVE_FAILED) {
			/* For now, we use a simpler loop to copy data
			 * into the target archive. */
			fd = open(archive_entry_sourcepath(entry), O_RDONLY);
			len = read(fd, buff, buff_size);
			while (len > 0) {
				archive_write_data(a, buff, len);
				len = read(fd, buff, buff_size);
			}
			close(fd);
		}
		archive_entry_free(entry);
	}
	ret = NEWIUP_OK;
out:
	archive_read_close(disk);
	archive_read_free(disk);

	archive_write_close(a);
	archive_write_free(a);
	return ret;
}

int do_compress(int type, char *buff, int buff_size, char * DestName, char *SrcName)
{
	int ret = NEWIUP_FAIL;
	//lzma or 7z,but time is long
	if(type == UPLOAD_COMPRESS_GZ){
		ret = do_compress_gz( buff, buff_size,DestName, SrcName);
//	}else if(type == UPLOAD_COMPRESS_BZ){
//		ret = do_compress_bz2( buff, buff_size,DestName, SrcName);
	}else if(type == UPLOAD_COMPRESS_LZMA){
		ret = LzmaCompress(SrcName, DestName);
	}else if(type <UPLOAD_COMPRESS_MAX){
		ret = do_compress_archive(type,buff, buff_size, DestName, SrcName);
	}
	//ret = do_compress_gz(buff, buff_size, DestName, SrcName); //only compress gz
	//ret = do_compress_bz2(buff, buff_size, DestName, SrcName);//only compress bz2
	return ret;
}
