# NetBSD system call names.
MIASM	= \
	syscall.o \
	exit.o \
	fork.o \
	read.o \
	write.o \
	open.o \
	close.o \
	wait4.o \
	link.o \
	unlink.o \
	chdir.o \
	fchdir.o \
	mknod.o \
	chmod.o \
	chown.o \
	break.o \
	getfsstat.o \
	getpid.o \
	mount.o \
	unmount.o \
	setuid.o \
	getuid.o \
	geteuid.o \
	ptrace.o \
	recvmsg.o \
	sendmsg.o \
	recvfrom.o \
	accept.o \
	getpeername.o \
	getsockname.o \
	access.o \
	chflags.o \
	fchflags.o \
	sync.o \
	kill.o \
	getppid.o \
	dup.o \
	pipe.o \
	getegid.o \
	profil.o \
	ktrace.o \
	sigaction.o \
	getgid.o \
	sigprocmask.o \
	__getlogin.o \
	setlogin.o \
	acct.o \
	sigpending.o \
	ioctl.o \
	revoke.o \
	symlink.o \
	readlink.o \
	execve.o \
	umask.o \
	chroot.o \
	vfork.o \
	sbrk.o \
	sstk.o \
	vadvise.o \
	munmap.o \
	mprotect.o \
	madvise.o \
	mincore.o \
	getgroups.o \
	setgroups.o \
	getpgrp.o \
	setpgid.o \
	setitimer.o \
	getitimer.o \
	dup2.o \
	fcntl.o \
	select.o \
	fsync.o \
	setpriority.o \
	socket.o \
	connect.o \
	getpriority.o \
	sigreturn.o \
	bind.o \
	setsockopt.o \
	listen.o \
	sigsuspend.o \
	vtrace.o \
	gettimeofday.o \
	getrusage.o \
	getsockopt.o \
	readv.o \
	writev.o \
	settimeofday.o \
	fchown.o \
	fchmod.o \
	setreuid.o \
	setregid.o \
	rename.o \
	flock.o \
	mkfifo.o \
	sendto.o \
	shutdown.o \
	socketpair.o \
	mkdir.o \
	rmdir.o \
	utimes.o \
	adjtime.o \
	setsid.o \
	quotactl.o \
	nfssvc.o \
	statfs.o \
	fstatfs.o \
	getfh.o \
	sysarch.o \
	ntp_gettime.o \
	ntp_adjtime.o \
	setgid.o \
	setegid.o \
	seteuid.o \
	lfs_bmapv.o \
	lfs_markv.o \
	lfs_segclean.o \
	lfs_segwait.o \
	pathconf.o \
	fpathconf.o \
	getrlimit.o \
	setrlimit.o \
	mmap.o \
	__syscall.o \
	lseek.o \
	truncate.o \
	ftruncate.o \
	__sysctl.o \
	mlock.o \
	munlock.o \
	undelete.o \
	futimes.o \
	getpgid.o \
	reboot.o \
	poll.o \
	__semctl.o \
	semget.o \
	semop.o \
	semconfig.o \
	msgctl.o \
	msgget.o \
	msgsnd.o \
	msgrcv.o \
	shmat.o \
	shmctl.o \
	shmdt.o \
	shmget.o \
	clock_gettime.o \
	clock_settime.o \
	clock_getres.o \
	nanosleep.o \
	posix_rename.o \
	swapctl.o \
	getdents.o \
	minherit.o \
	lchmod.o \
	lchown.o \
	lutimes.o \
	__msync13.o \
	__stat13.o \
	__fstat13.o \
	__lstat13.o \
	__sigaltstack14.o \
	__vfork14.o \
