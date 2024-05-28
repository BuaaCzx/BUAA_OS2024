#ifndef _USER_FD_H_
#define _USER_FD_H_ 1

#include <fs.h>

#define debug 0

#define MAXFD 32
#define FILEBASE 0x60000000
#define FDTABLE (FILEBASE - PDMAP)

#define INDEX2FD(i) (FDTABLE + (i)*PTMAP)
#define INDEX2DATA(i) (FILEBASE + (i)*PDMAP)

// pre-declare for forward references
struct Fd;
struct Stat;
struct Dev;

// Device struct:
// It is used to read and write data from corresponding device.
// We can use the five functions to handle data.
// There are three devices in this OS: file, console and pipe.
struct Dev {
	int dev_id;
	char *dev_name;
	int (*dev_read)(struct Fd *, void *, u_int, u_int);
	int (*dev_write)(struct Fd *, const void *, u_int, u_int);
	int (*dev_close)(struct Fd *);
	int (*dev_stat)(struct Fd *, struct Stat *);
	int (*dev_seek)(struct Fd *, u_int);
};
/*
int cons_read(struct Fd *fd, void *vbuf, u_int n, u_int offset) {
	int c;

	if (n == 0) {
		return 0;
	}

	while ((c = syscall_cgetc()) == 0) {
		syscall_yield();
	}

	if (c != '\r') {
		debugf("%c", c);
	} else {
		debugf("\n");
	}
	if (c < 0) {
		return c;
	}
	if (c == 0x04) { // ctl-d is eof
		return 0;
	}
	*(char *)vbuf = c;
	return 1;
}

int cons_write(struct Fd *fd, const void *buf, u_int n, u_int offset) {
	int r = syscall_print_cons(buf, n);
	if (r < 0) {
		return r;
	}
	return n;
}

int cons_close(struct Fd *fd) {
	return 0;
}

int cons_stat(struct Fd *fd, struct Stat *stat) {
	strcpy(stat->st_name, "<cons>");
	return 0;
}

*/

// file descriptor 文件描述符
struct Fd {
	u_int fd_dev_id;
	u_int fd_offset;
	u_int fd_omode;
};

// State
struct Stat {
	char st_name[MAXNAMELEN];
	u_int st_size;
	u_int st_isdir;
	struct Dev *st_dev;
};

// file descriptor + file
struct Filefd {
	struct Fd f_fd;
	u_int f_fileid;
	struct File f_file;
};

int fd_alloc(struct Fd **fd);
int fd_lookup(int fdnum, struct Fd **fd);
void *fd2data(struct Fd *);
int fd2num(struct Fd *);
int dev_lookup(int dev_id, struct Dev **dev);
int num2fd(int fd);
extern struct Dev devcons;
extern struct Dev devfile;
extern struct Dev devpipe;

#endif
