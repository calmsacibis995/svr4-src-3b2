/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ports:ports.c	1.7.7.1"
/*
 *	Copyright 1988 AT&T
 */

#include "stdio.h"
#include "fcntl.h"
#include "string.h"
#include "ctype.h"
#include "grp.h"
#include "sac.h"
#include "sys/mkdev.h"
#include "sys/edt.h"
#include "sys/sys3b.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "dirent.h"
#include "sys/cio_defs.h"
#include "sys/pp_dep.h"
#include "sys/queue.h"
#include "sys/stream.h"
#include "sys/stropts.h"
#include "sys/strpump.h"
#include "sys/strppc.h"

#define	NO_OF_AP_ENTERIES	4
#define	PORTS_AP_FILE		"/etc/ports.ap"
#define PORTS_PUMP_FILE		"/usr/lib/pump/ports"
#define HPP_PUMP_FILE		"/usr/lib/pump/ports.hpp"
#define EPORTS_PUMP_FILE	"/usr/lib/pump/eports"
#define MIL_PUMP_FILE		"/usr/lib/pump/milports"
#define	DEV_TERM		"/dev/term/"
#define	INIT_MOD_LIST		"ldterm"
#define DEFAULT_TTY_GROUP	"tty"
#define TIMEOUT		120	/* I_STR timeout value */
#define	TABLE_SIZE	512	/* Maximum number of lines in the PORTS_AP_FILE */
#define	MAX_PORTS	9	/* Maximum number of ports/board supported + 1 */
#define	ERROR	-1
#define PORTS_CODE	3
#define EPORTS_CODE	0x102
#define	MILSTD_CODE	0x103
#define MAXSLOTS	9
#define MAXSUBDEV	30
#define PPC_VERS	(('v'<<8)|1)
#define HIPORTS		2
#define NOWRITE		0x7f
#define INSTALLED	1
#define NOT_INSTALLED	0

extern int errno;

/*
 * The array index[] is used for storing info on special files 
 * in /dev/term which correspond to the ith slot on the i/o board.
 *
 * The array ttycor is used  to denote  the special files in /dev/term
 * ttycor[i] is the number of tty files with  major number i.
 *
 * The array slot[] is used to denote those tty-type boards (PORTS,
 * EPORTS, etc) which are configured. The slot[i] is set equal to
 * the board's code, if one is configured, and 0 otherwise.
 */
short index[MAXSLOTS] ={ 0, 0, 0, 0, 0, 0, 0, 0, 0};
short ttycor[MAXSLOTS] ={ 0, 0, 0, 0, 0, 0, 0, 0, 0};
short slot[MAXSLOTS] ={ 0, 0, 0, 0, 0, 0, 0, 0, 0};

struct table {
	struct stat statb;
	char pathname[25];
	short istty;
} table[MAXSLOTS][MAXSUBDEV];

struct table temp;

DIR	*dirp;
struct	dirent	*dp;

struct edtsize {
	short esize;
	short ssize;
} edtsize;

struct 	edt *edtptr;

struct	strioctl	ioctlbuf;	/* STREAMS I_STR control structure */
struct pm_buf {
	int svctag;	/* the svc tag value */
	char pmtag[15];	/* the pm tag name */
};

char	header_comment[] =	"#\n\
# This is the ports auto-push file.\n\
# This file should only contain entries for 3b2 ports, hiports,\n\
# eports and mil-ports boards.\n\
# Auto-push information for other boards should go in other files.\n\
#\n\
#     Major    Minor Lastminor   Modules\n";

main()
{
	char	temp_buffer[512];
	char	buffer[TABLE_SIZE][128];
	char	cmd[80];
	char	cmdline[256];
	char	device[25];
	char	modules[80];
	char	pumpfile[25];
	char	*tty_num;	/* pointer to a tty name */
	char	*ptr;
	char	*path = DEV_TERM;
	char	*ports_pump_file = PORTS_PUMP_FILE;
	char	*hpp_pump_file = HPP_PUMP_FILE;
	char	*eports_pump_file = EPORTS_PUMP_FILE;
	char	*mil_pump_file = MIL_PUMP_FILE;
	char	*get_tty_name();

	int	n;
	int	no_tty_entry;
	int	no_maj_entry;
	int	numents;
	int	scanf_ret;
	int	devmajor;
	int	devminor;
	int	lastminor;
	int	sys_ret;
	int	tty_num_len;
	int	fd_tmp_ap;
	int	fd_ports_ap;
	int	new_ports_ap = 0;
	int	group_field = 1;
	int	edt_size;
	int	i;
	int	j;
	int	numnodes;
	int	ports_installed; /* A flag indicating whether PORTS is installed or not. */
	int	hpp_installed; /* A flag indicating whether HIPORTS is installed or not. */
	int	eports_installed; /* A flag indicating whether EPORTS is installed or not. */
	int	mil_installed; /* A flag indicating whether MILPORTS is installed or not. */

	mode_t	cmask = 0;

	major_t	maj;

	struct	pm_buf pm_buf[MAXSLOTS][MAX_PORTS];
	struct group	*gr_name_ptr;

	FILE	*fs_ports_ap;
	FILE	*fs_tmp_ap;
	FILE	*fs_popen;


     	if ( sys3b( S3BEDT, &edtsize, sizeof( struct edtsize)) != 0) {
		printf( "Ports: Sys3b call to get edt table failed.\n");
		exit( 1);
	}
    
     	edt_size = edtsize.esize*sizeof( struct edt) + edtsize.ssize*sizeof( struct subdevice) + sizeof( struct edtsize);

     	if (( edtptr = (struct edt *)malloc( edt_size)) == NULL) {
		printf( "Ports: Not enough space to allocate memory for the extened device table.\n");
		exit( 1);
	}

     	if ( sys3b( S3BEDT, edtptr, edt_size) != 0) {
		printf( "Ports: Sys3b(S3BEDT) call to get edt table failed.\n");
		exit( 1);
	}

	edtptr = (struct edt *)((char *)edtptr + 4);

     	for ( i = 0; i < edtsize.esize; ++i) {
		if ( edtptr->opt_code == PORTS_CODE
			|| edtptr->opt_code == EPORTS_CODE
			|| edtptr->opt_code == MILSTD_CODE)
			if ( edtptr->opt_slot >= MAXSLOTS)
				printf( "Ports: warning board at postion %d not pumped. Internal array too small\n", edtptr->opt_slot);
			else
				slot[edtptr->opt_slot] = edtptr->opt_code;
		edtptr++;
	}
	for ( i = 0; i < MAXSLOTS; i++)
		if ( slot[i] != 0)
			break;

	if ( i == MAXSLOTS)
		/*
		 * If there are no ports board on the machine just return quitely
		 */
		exit( 0);

	/*
	 * Determine what terminal firmware is installed
	 */
	if ( access( ports_pump_file, 04) == -1)
		ports_installed = NOT_INSTALLED;
	else
		ports_installed = INSTALLED;

	if ( access( hpp_pump_file, 04) == -1)
		hpp_installed = NOT_INSTALLED;
	else
		hpp_installed = INSTALLED;

	if ( access( eports_pump_file, 04) == -1)
		eports_installed = NOT_INSTALLED;
	else 
		eports_installed = INSTALLED;
	
	if ( access( mil_pump_file, 04) == -1)
		mil_installed = NOT_INSTALLED;
	else
		mil_installed = INSTALLED;


	/*
	 * Read the device directory into buffer
	 */
	if (( dirp = opendir( path)) == NULL) {
		printf( "Ports: Cannot open %s directory, errno %d\n", path, errno);
		exit( errno);
	}

	while ( dp = readdir( dirp)) {
		if ( dp->d_name[0] != '.') {
			sprintf( temp.pathname, "%s/%s", path, dp->d_name);

			if ( stat( temp.pathname, &temp.statb) < 0)
				continue;

			if ((( temp.statb.st_mode&S_IFMT) == S_IFCHR) && ( dp->d_ino != 0)) {
				maj = major( temp.statb.st_rdev);

				if ( maj < MAXSLOTS) {
					table[maj][index[maj]] = temp;
					if ( maj <= 9) {
						if (( dp->d_name[0] - '0') == maj) {
							ttycor[maj] += 1;
							table[maj][index[maj]].istty = 1;
						}
					} else {
						if ((( dp->d_name[0] - '0')*10) + (dp->d_name[1] - '0') == maj) {
							ttycor[maj] += 1;
							table[maj][index[maj]].istty = 1;
						}
					}
					index[maj]++;
				}
			}
		}
	}

	closedir( dirp);
	umask( cmask);

	/*
	 * All default tty device groups created have DEFAULT_TTY_GROUP group mode
	 */
	if (( gr_name_ptr = getgrnam( DEFAULT_TTY_GROUP)) == NULL) {
		group_field = 0;
		printf( "WARNING: ports can't find group %s in /etc/group\n", DEFAULT_TTY_GROUP);
	}

	for ( i = 0; i < MAXSLOTS; i++) {
		if ( slot[i] == 0) {
			/*
			 * Board does not exist, remove associated /dev/term entries
			 */
			if ( ttycor[i] != 0) {
				for ( j = 0; j < index[i]; j++) {
					if ( table[i][j].istty == 0)
						printf( "Warning: %s is being removed.\n", table[i][j].pathname);
					unlink( table[i][j].pathname);
				}
			}
		} else {
			/*
			 * Board exist in slot i
			 */
			numnodes = getnum( i);
			switch ( slot[i]) {

				case EPORTS_CODE:
					strcpy( pumpfile, "eports");
					break;

				case PORTS_CODE:
					strcpy( pumpfile, "ports");
					break;

				case MILSTD_CODE:
					strcpy( pumpfile, "milports");
					break;

				default:
					printf(  "Ports: Unknown board id.\n");
					exit( 1);
			}
			if( index[i] != 0) {
				/*
				 * /dev/term file exists for this major
				 */
				if ( ttycor[i] != 0 && ttycor[i] != numnodes)
					ttycor[i] = 0;

				if ( ttycor[i] == 0) {

					/*
					 * Not a tty file or an entry in /dev/term
					 * that represents a port that doesn't
					 * match the board installed
					 */ 
					for ( j = 0; j < index[i]; j++) {
						if ( table[i][j].istty == 0)
							printf( "Ports warning: %s is being removed.\n", table[i][j].pathname);
						unlink( table[i][j].pathname);
					}
					for ( j = 1; j <= numnodes; j++) {
						sprintf( device, "%s/%d%d", path, i, j);
						mknod( device, 0020620, makedev( (major_t)i, (minor_t)(j - 1)));
						if ( group_field)
							chown( device, temp.statb.st_uid, gr_name_ptr->gr_gid);
					}
				}
			} else {
				/*
				 * File does not exist for this major
				 */
				for ( j = 1; j <= numnodes; j++) {
					sprintf( device, "%s/%d%d", path,i,j);
					mknod( device, 0020620, makedev( (major_t)i, (minor_t)(j - 1)));
					if ( group_field)
						chown( device, temp.statb.st_uid, gr_name_ptr->gr_gid);
				}
			}
			/*
			 * Pump the tty board
			 */
			if (  slot[i] == PORTS_CODE && ( hpp_installed || ports_installed)
			   || slot[i] == EPORTS_CODE && eports_installed
			   || slot[i] == MILSTD_CODE && mil_installed) {
				/*
				 * Create an administrative device with
				 * major i and minor A_MINOR.  This is
				 * the device that will be pumped.
				 */
				sprintf( device, "%s%d%d", path, i, A_MINOR);
				mknod( device, 0020620, makedev( (major_t)i, (minor_t)A_MINOR));
				if ( slot[i] == PORTS_CODE && ( ports_installed || hpp_installed))
					if ( ver( device, pumpfile) == -1) {
						printf( "PORTS: version error\n");
						continue;
					}

				sprintf( cmd, "/sbin/npump %s /usr/lib/pump/%s", device, pumpfile);
				system( cmd);
				/*
				 * Remove the administrative device file
				 */
				unlink( device);
			}
		}
	}

	/*
	 * Set up saf database files
	 */
	for ( i = 0; i < MAXSLOTS; i++)
		for ( j = 0; j < MAX_PORTS; j++) {
			pm_buf[i][j].svctag = 0;
			*pm_buf[i][j].pmtag = '\0';
		}

	/*
	 * Determine which pmtab entries already exist.
	 * This is noted in the pm_buf array.
	 */
	sprintf( cmdline, "/usr/sbin/pmadm -L -t ttymon");
	sys_ret = execute( cmdline);

	if (( sys_ret>>8) != E_NOEXIST)
		if (( fs_popen = popen( cmdline, "r")) != NULL) {
			while ( fgets( temp_buffer, 512, fs_popen) != NULL) 
				if (( tty_num = get_tty_name( temp_buffer)) != NULL) {
					tty_num_len = strlen( tty_num);
					if ( tty_num_len <= 1 || tty_num_len > 3) {
						printf( "Ports: warning: tty device name is not in standard format, length %d\n", tty_num_len);
						continue;
					}
					if ( tty_num_len == 2)
						sscanf( tty_num, "%1d%1d", &devmajor, &devminor);
					else
						sscanf( tty_num, "%2d%1d", &devmajor, &devminor);
					if ( devmajor > MAXSLOTS) {
						printf( "Ports: warning: Major %d exceeds maximum %d\n", devmajor, MAXSLOTS);
						continue;
					}
					if ( devminor > MAX_PORTS) {
						printf( "Ports: warning: Minor %d exceeds maximum %d\n", devminor, MAX_PORTS);
						continue;
					}
					pm_buf[devmajor][devminor].svctag = devmajor*10 + devminor;
					ptr = strtok( temp_buffer, ":");
					strcpy( pm_buf[devmajor][devminor].pmtag, ptr);
				}
		}

	for ( i = 1; i < MAXSLOTS; i++) {
		/*
		 * Check if there is a port monitor entry
		 */

		if ( slot[i] == 0) {
			for ( j = 1; j < MAX_PORTS && pm_buf[i][j].svctag != 0; j++) { 
				sprintf( cmdline, "/usr/sbin/pmadm -r -p %s -s %d", pm_buf[i][j].pmtag, pm_buf[i][j].svctag);
				execute( cmdline);
			}

			sprintf( cmdline, "/usr/sbin/pmadm -L -p ttymon%d", i);
			sys_ret = execute( cmdline);
			if (( sys_ret>>8) == E_NOEXIST) {
				/*
				 * If port monitor entries do exist for a
				 * board not installed then need to clean
				 * them up
				 */
				sprintf( cmdline, "/usr/sbin/sacadm -r -p ttymon%d", i);
				execute( cmdline);
			}
			continue;
		}

		numnodes = getnum( i);
		/*
		 * The PORTS and HIPORTS boards
		 * have one CENTRONICS port and there is
		 * no port monitor entry for it.
		 * Adjust accordingly for this.
		 */
		if ( slot[i] == PORTS_CODE)
			numnodes--;

		for ( j = 1; j <= numnodes; j++) {
			if ( pm_buf[i][j].svctag)
				continue;

			sprintf( cmdline, "/usr/sbin/sacadm -l -p ttymon%d", i);
			sys_ret = execute( cmdline);
			if (( sys_ret>>8) == E_NOEXIST) {
				sprintf( cmdline, "/usr/sbin/sacadm -a -n 2 -p ttymon%d -t ttymon -c /usr/lib/saf/ttymon -v \"`/usr/sbin/ttyadm -V`\" -y ttymon%d", i, i);
				sys_ret = execute( cmdline);
			}
			/*
			 * Add port monitor entry
			 */
			sprintf( cmdline, "/usr/sbin/pmadm -a -p ttymon%d -s %d%d -i root -v `/usr/sbin/ttyadm -V` -fux -y\"/dev/term/%d%d\" -m \"`/usr/sbin/ttyadm -d /dev/term/%d%d -s /usr/bin/login -l 9600 -p \\\"login: \\\"`\"", i, i, j, i, j, i, j);
			execute( cmdline);
		}
	}

	/*
	 * Administrate the ports auto-push file
	 */
	if ( access( "/etc/ports.ap", 00) == -1)
		new_ports_ap = 1;

	if (( fd_ports_ap = open( "/etc/ports.ap", O_RDONLY|O_CREAT, 0000666)) < 0) {
		printf( "Ports: Error %d - Wasn't able to create the /etc/ports.ap \n", errno);
		exit( errno);
	}
	if (( fs_ports_ap = fdopen( fd_ports_ap, "r")) == NULL) {
		printf( "/etc/ports.ap cannot be opened for reading.  Please call your local service representative.\n");
		exit( 1);
	}

	if (( fd_tmp_ap = open( "/tmp/tmp.ports.ap", O_RDWR|O_APPEND|O_CREAT|O_TRUNC, 0000666)) < 0) {
		printf( "Ports: Error %d - Wasn't able to create a temporary file\n", errno);
		exit( errno);
	}

	if (( fs_tmp_ap = fdopen( fd_tmp_ap, "r+w+a+")) == NULL) {
		printf( "/tmp/tmp.ports.ap cannot be opened for reading and writing.  Please call your local service representative.\n");
		exit( 1);
	}

	i = 0;
	while ( fgets( buffer[i], 128, fs_ports_ap) != NULL) {

		if ( i >= TABLE_SIZE) {
			printf( "PORTS: Warning %s has too many enteries.\n", PORTS_AP_FILE);
			printf( "Lines after the %d are ignored\n", TABLE_SIZE);
			break;
		}
		if ( buffer[i][0] == '#') {
			i++;
			continue;	/* Skip comment lines */
		}
		scanf_ret = sscanf( buffer[i], "%d%d%d%s", &devmajor, &devminor, &lastminor, modules);
		if ( scanf_ret != NO_OF_AP_ENTERIES) {
			printf( "Warning: Format error in the %s file. Entry removed\n", PORTS_AP_FILE);
			continue;
		}

		/*
		 * If slot[major_device] == 0 then no tty board is configured
		 * there, so don't rewrite the line in ports.ap.
		 * If slot[major_device] != 0 and ttycor[major_device] == 0,
		 * then two tty boards of different types were swapped.
		 * In this case, don't rewrite the old auto-push entry.
		 */
		if (( slot[devmajor] == 0) || ( ttycor[devmajor] == 0))
			buffer[i][0] = NOWRITE;
		i++;
	}

	for ( j = 0; j < MAXSLOTS; j++) {
		if ( slot[j] == 0)
			/*
			 * Skip entries that have no hardware attached
			 */
			continue;

		numents = getnum( j);
		no_maj_entry = 1;
		for ( n = 0; n < numents; n++) {
			/*
			 * Check if there is a corresponding entry
			 * in the ports_ap file.
			 * If not, indicate the need for one.
			 */
			rewind( fs_ports_ap);
			no_tty_entry = 0;
			while ( fgets( temp_buffer, 512, fs_ports_ap) != NULL) {

				if ( temp_buffer[0] == '#')
					continue;	/* Skip comment lines */

				sscanf( temp_buffer, "%d%d%d%s", &devmajor, &devminor, &lastminor, modules);
				/*
				 * Skip enteries that are not applicable now
				 */
				if (( slot[devmajor] == 0) || ( ttycor[devmajor] == 0))
					continue;


				if ( devmajor == j) {
					no_maj_entry = 0;
					if ( devminor == -1 /* All minors are valid */
					   || n == devminor
					   || (( n <= lastminor) && ( n >= devminor))) {
						no_tty_entry = 0; 
						break;
					} else
						no_tty_entry = 1; 
				}
			}

			if ( no_tty_entry) 
				sprintf( buffer[i++], "\t%d\t%d\t0\t%s\n", j, n, INIT_MOD_LIST);
		}
		if ( no_maj_entry)
			sprintf( buffer[i++], "\t%d\t-1\t%d\t%s\n", j, numents - 1, INIT_MOD_LIST);
	}

	if ( new_ports_ap)
		fputs( header_comment, fs_tmp_ap);

	j = 0;
	while ( j < i) {
		if ( buffer[j][0] != NOWRITE)
			fputs( buffer[j], fs_tmp_ap);
		j++;
	}

	fclose( fs_ports_ap);
	fclose( fs_tmp_ap);
	rename( "/tmp/tmp.ports.ap", "/etc/ports.ap");
	chown( "/etc/ports.ap", temp.statb.st_uid, temp.statb.st_gid);
	chmod( "/etc/ports.ap", temp.statb.st_mode&S_IAMB);
	/*
	 * Finally execute the autopush command to set up the
	 * ports module structure
	 */
	sprintf( cmdline, "/sbin/autopush -f %s", PORTS_AP_FILE);
	sys_ret = execute( cmdline);
	if ( sys_ret != 0) {
		printf( "Ports: warning autopush failed on %s\n", PORTS_AP_FILE);
		exit( 1);
	}
	exit( 0);
}

/*
 * ver returns 1 on success
 *	       -1 if error
 */

int
ver( device, pumpfile)
char *device;
char *pumpfile;
{
	long version;

	int fd;

	
	if (( fd = open( device, O_WRONLY)) < 0) {
		fprintf( stderr, "Ports: version error %d: Can't open %s\n", errno, device);
		return( -1);
	}


	ioctlbuf.ic_cmd = P_RST;
	ioctlbuf.ic_timout = TIMEOUT;
	ioctlbuf.ic_len = 0;
	ioctlbuf.ic_dp = NULL;

	if ( ioctl( fd, I_STR, &ioctlbuf) < 0) {
		fprintf( stderr, "Ports: version error: %d - ioctl(pump_rst) call\n", errno);
		close( fd);
		return( -1);
	}

	ioctlbuf.ic_cmd = PPC_VERS;
	ioctlbuf.ic_timout = TIMEOUT;
	ioctlbuf.ic_len = 0;
	ioctlbuf.ic_dp = (char *)&version;

	if ( ioctl( fd, I_STR, &ioctlbuf) < 0) {
		fprintf( stderr, "Ports: version error: %d - ioctl(version) call\n", errno);
		close( fd);
		return( -1);
	}

	/*
	 * Overwrite the pump file name if the board is HIPORTS
	 */
	if ( version == HIPORTS)
		strcpy( pumpfile, "ports.hpp");
	close( fd);

	return( 1);
}
/*
 *	Find the number of port entries for a particular board.
 */

getnum( slotnum)
int slotnum;
{
	int n;


	switch ( slot[slotnum]) {

		case PORTS_CODE:	/* PORTS and HIPORTS */
			n = 5;
			break;

		case EPORTS_CODE:
			n = 8;
			break;

		case MILSTD_CODE:
			n = 4;
			break;

		default:
			n = 5;		/* default to the PORTS case */
			break;
	}
	return( n);
}
/*
 * Get the name of a tty device from a pmtab entry.
 * Note the /dev/term/ part is taken off.
 */
char *
get_tty_name( buffer)
char	*buffer;	/* Pointer to a typical pmtab entry */
{
	int	i;

	char	*ptr;

	static char	tty_name[3];


	/*
	 * Note the eighth field( ':' seperated) is the tty name field
	 */
	for ( i = 1, ptr = strchr( buffer, ':'); i < 8; i++, ptr = strchr( ptr, ':')) 
		ptr++;

	ptr += strlen( DEV_TERM) + 1;
	strncpy( tty_name, ptr, 2);

	if ( isdigit( tty_name[0]) && isdigit( tty_name[1]))
		return( tty_name);
	else
		return( (char *)NULL);
}

int
execute( s)
char	*s;
{
	int	status;
	int	fd;
	pid_t	pid;
	pid_t	w;

  
	if (( pid = fork()) == 0) {
		close( 0);
		close( 1);
		close( 2);
		fd = open( "/dev/null", O_RDWR);
		dup( fd);
		dup( fd);
		(void) execl( "/sbin/sh", "sh", "-c", s, 0);
		_exit(127);
	}
	while (( w = wait( &status)) != pid && w != (pid_t)-1)
		;

	return(( w == (pid_t)-1)? w: status);
}
