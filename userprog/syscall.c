#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"
#include "threads/init.h"
#include "threads/synch.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

// modified
struct lock filesys_lock;

void user_memory_access (const void * ptr);

void halt(void) NO_RETURN;
void exit(int status) NO_RETURN;
int exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, const void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void close(int fd) NO_RETURN;

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);

	// modified
	lock_init(&filesys_lock);            // initialize lock of file system
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f) {
	// TODO: Your implementation goes here.
	user_memory_access((const void *) f->rsp);

	uint64_t syscallnum = f->R.rax;
	uint64_t arg1 = f->R.rdi;
	uint64_t arg2 = f->R.rsi;
	uint64_t arg3 = f->R.rdx;
	uint64_t arg4 = f->R.r10;
	uint64_t arg5 = f->R.r8;
	uint64_t arg6 = f->R.r9;

	printf("System Call: %d!\n", (int)syscallnum);

		
	switch (syscallnum) {
		case SYS_HALT :
			halt();
			break;
		case SYS_EXIT :
			exit(arg1);          // arg1 = status
			break;
		case SYS_FORK :
			//pid_t pid = fork(arg1);
			thread_exit();
			break;
		case SYS_EXEC :
			f->R.rax = exec(arg1);
			break;
		case SYS_WAIT :
			f->R.rax = wait(arg1);
			break;
		case SYS_CREATE :
			//f->R.rax = create(arg1, arg2);      // arg1 = file name, arg2 = initial size
			thread_exit();
			break;
		case SYS_REMOVE :
			//f->R.rax = remove(arg1);   // arg1 = file name
			thread_exit();
			break;
		case SYS_OPEN :
			f->R.rax = open(arg1);    // arg1 = file name
			break;
		case SYS_FILESIZE :
			f->R.rax = filesize(arg1);         // arg1 = fd
			break;
		case SYS_READ :
			f->R.rax = read(arg1, arg2, arg3);      // arg1 = fd, arg2 = buffer, arg3 = size
			break;
		case SYS_WRITE :
			f->R.rax = write(arg1, arg2, arg3);     // arg1 = fd, arg2 = buffer, arg3 = size
			break;
		case SYS_SEEK :
			//seek(arg1, arg2);
			thread_exit();
			break;
		case SYS_TELL :
			//tell(arg1);
			thread_exit();
			break;
		case SYS_CLOSE :
			close(arg1);           // arg1 = fd
			break;
		default :
			thread_exit();
	}
	
	//printf ("system call!\n");
	//thread_exit ();
}

// check user memory access
void user_memory_access (const void *ptr) {
	/* terminate user process when user provied invalid pointer,
	 pointer into kernel memory,
	 or block paritially in one of those regions */
	if (ptr == NULL || is_kernel_vaddr(ptr))
		exit(-1);

}

// halt OS
void halt(void) {
	power_off();
}

// terminate process
void exit(int status) {
	struct thread *curr = thread_current();
	curr->exit_code = status;      // return status to kernel

	// close all opened file when exit the process
	for (int i = 3; i < 128; i++) {
		if (curr->fd_table[i] != NULL) {
			file_close(curr->fd_table[i]);
		}
	}

	printf("%s: exit(%d)\n", curr->name, status);  // Process Termination Message
	thread_exit();             // terminate current user program
}

// execute the process whose name is given in cmd_line
int exec(const char *cmd_line) {
	printf("exec debug\n");
	return process_exec(cmd_line);
}	

int wait(pid_t pid) {
	return process_wait(pid);
}

// create a new file called file initially initial_size bytes in size
bool create(const char *file, unsigned initial_size) {
	bool success = false;
	success = filesys_create(file, initial_size);   // return true if successful, false otherwise

	return success;
}

// remove the file called file
bool remove(const char *file) {
	bool success = false;
	success = filesys_remove(file);     // return true if successful, false otherwise
	
	return success;;
}

// open the file called file, return file descriptor
int open (const char *file) {
	struct thread *curr = thread_current();
	struct file * openfile = filesys_open(file);

	if (openfile == NULL)
		return -1;            // open fail
	else {
		for (int i = 3; i < 128; i++) {
			if (curr->fd_table[i] == NULL) {
				curr->fd_table[i] = openfile;
				return i;
			}
		}
	}
}

// return size of file in bytes
int filesize (int fd) {
	struct file * currfile = thread_current()->fd_table[fd];

	return file_length(currfile);
}

// read size bytes from file into buffer, return # of bytes actually read
int read (int fd, const void *buffer, unsigned size) {
	struct thread *curr = thread_current();

	//lock_acquire(&filesys_lock);          // can occur simultaneous access to file

	if (fd == 0) {               // fd = 0 means STDIN that reads from keyboard
		//lock_release(&filesys_lock);

		return (int)input_getc();
	}

	if (fd == 1) {        // fd = 1 means STDOUT that can't read
		//lock_release(&filesys_lock);
		return 0;
	}

	if (fd >= 3) {               // return # of bytes actually read
		//lock_release(&filesys_lock);
		int bytes = (int)file_read(curr->fd_table[fd], buffer, size);
		return bytes;
	}

	/*
	if (list_empty(curr->fd_table)) {           // If no file is open, then reading bytes become 0
		lock_release(&filesys_lock);
		return 0;
	}
	*/

	//lock_release(&filesys_lock);

	return -1;           // read fail
}

// write size bytes from buffer to open file fd, and return the size written
int write (int fd, const void *buffer, unsigned size) {
	struct thread *curr = thread_current();

	//lock_acquire(&filesys_lock);            // can occur simultaneous access to file
	if (fd == 0) {           // fd = 0 means STDIN
		//lock_release(&filesys_lock);
		return 0;
	}

	if (fd == 1) {           // fd = 1 means STDOUT that writes to the console
		//lock_release(&filesys_lock);
		putbuf(buffer, size);

		return size;
	}

	if (fd >= 3) {
		//lock_release(&filesys_lock);
		int written_size = (int)file_write(curr->fd_table[fd], buffer, size);
		return written_size;
	}	
	/*
	if (list_empty(curr->fd_table)) {             // No files are open
		lock_release(&filesys_lock);
		return 0;
	}
	*/

	//lock_release(&filesys_lock);

	return 0;           // write fail -> 0 byte is written
}

// close the file
void close (int fd) {
	struct thread *curr = thread_current();
	file_close(curr->fd_table[fd]);
	curr->fd_table[fd] = NULL;
}
