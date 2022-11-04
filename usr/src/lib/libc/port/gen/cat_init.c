/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/cat_init.c	1.1"
#ifdef __STDC__
	#pragma weak cat_init = _cat_init
	#pragma weak cat_malloc_init = _cat_malloc_init
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <nl_types.h>

extern	caddr_t mmap();
extern	void	munmap();
extern char *malloc();
extern int errno;


/*
 * Read a catalog and init the internal structure
 */
cat_init(name, res)
  char *name;
  nl_catd res;
{
  struct cat_hdr hdr;
  char *mem;
  int fd;
  long magic;

  /*
   * Read file header
   */
  if((fd=open(name,0)) < 0) {
    /*
     * Need read permission
     */
    return 0;
  }

  if (read(fd, (char *)&magic, sizeof(long)) == sizeof(long)){
    if (magic == CAT_MAGIC)
      return cat_malloc_init(fd,res);
    else
      return cat_mmp_init(fd,name,res);
  }
  return 0;
}

/*
 * Read a malloc catalog and init the internal structure
 */
cat_malloc_init(fd, res)
  int fd;
  nl_catd res;
{
  struct cat_hdr hdr;
  char *mem;

  lseek(fd,0L,0);
  if (read(fd, (char *)&hdr, sizeof(struct cat_hdr)) != sizeof(struct cat_hdr))
    return 0;
  if ((mem = malloc(hdr.hdr_mem)) != (char*)0){

    if (read(fd, mem, hdr.hdr_mem) == hdr.hdr_mem){
      res->info.m.sets = (struct cat_set_hdr*)mem;
      res->info.m.msgs = (struct cat_msg_hdr*)(mem + hdr.hdr_off_msg_hdr);
      res->info.m.data = mem + hdr.hdr_off_msg;
      res->set_nr = hdr.hdr_set_nr;
      res->type = MALLOC;
      close(fd);
      return 1;
    } else
      free(mem);
  }

  close(fd);
  return 0;
}


extern int _mmp_opened;

/*
 * Do the gettxt stuff
 */
static
cat_mmp_init (fd,catname,res)
  char  *catname;
  nl_catd res;
{
  struct m_cat_set *sets;
  int no_sets;
  char symb_name[MAXNAMLEN];
  char symb_path[MAXNAMLEN];
  char message_file[MAXNAMLEN];
  char buf[MAXNAMLEN];
  int bytes;
  struct stat sb;
  caddr_t addr;
  static int count = 1;
  extern char *getcwd();


  if (_mmp_opened == NL_MAX_OPENED) {
    close(fd);
    return 0;
  }
  res->type = MKMSGS;

  /*
   * get the number of sets
   * of a set file
   */
  if (fstat(fd, &sb) == -1) {
    close(fd);
    return 0;
  }

  addr = mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  
  if ( addr == (caddr_t)-1 ) {
    close(fd);
    return 0;
  }
  no_sets = *((int*)(addr));
  if (no_sets > NL_SETMAX) {
    munmap(addr, sb.st_size);
    close(fd);
    return 0;
  }

  res->set_nr = no_sets;
  res->info.g.sets = (struct set_info *)addr;
  res->info.g.size = sb.st_size;
  res->info.g.fd = fd;

  /*
   * Create the link for gettxt
   */

  sprintf(symb_name,"gencat.%x.%x",getpid(),count++);
  sprintf(symb_path,"%s/%s",XOPEN_DIRECTORY,symb_name);
  if (catname[0] == '/')
    sprintf(message_file,"%s%s",catname,M_EXTENSION);
  else 
    sprintf(message_file,"%s/%s%s",getcwd(buf,MAXNAMLEN),catname,M_EXTENSION);
  if (symlink(message_file,symb_path) < 0)  {
    munmap(addr, sb.st_size);
    close(fd);
    return 0;

  }
  
  res->info.g.link = malloc(strlen(symb_name)+1);
  if (res->info.g.link == (char*)NULL ) {
    unlink(symb_name);
    munmap(addr, sb.st_size);
    close(fd);
    return 0;
  }
  strcpy(res->info.g.link,symb_name);
  if (fcntl(fd,F_SETFD,1) == -1) {
    unlink(symb_name);
    munmap(addr, sb.st_size);
    close(fd);
    return 0;
  }
  _mmp_opened++;
  return 1;
}

