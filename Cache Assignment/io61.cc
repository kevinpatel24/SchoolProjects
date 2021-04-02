#include "io61.hh"
#include <sys/types.h>
#include <sys/stat.h>
#include <climits>
#include <cerrno>

// io61.c
//    YOUR CODE HERE!

// io61_file
//    Data structure for io61 file wrappers. Add your own stuff.


struct io61_file {
    int fd;
    static constexpr off_t bufsize = 4096;
    unsigned char cache[bufsize];
    off_t tag;      // file offset of first byte in cache (0 when file is opened)
    off_t end_tag;  // file offset one past last valid byte in cache
    off_t pos_tag;  // file offset of next char to read in cache	
    int mode;
};

// io61_fdopen(fd, mode)
//    Return a new io61_file for file descriptor `fd`. `mode` is
//    either O_RDONLY for a read-only file or O_WRONLY for a
//    write-only file. You need not support read/write files.

io61_file* io61_fdopen(int fd, int mode) {
    assert(fd >= 0);
    io61_file* f = new io61_file;
    f->fd = fd;
    f->tag = 0;
    f->end_tag = 0;
    f->pos_tag = 0;
    f->mode = mode;
    return f;
}

void io61_fill(io61_file* f) {
    // Fill the read cache with new data, starting from file offset `end_tag`.
    // Only called for read caches.

    // Reset the cache to empty.
    f->tag = f->pos_tag = f->end_tag;

    // Read data.
    ssize_t n = read(f->fd, f->cache, f->bufsize);
    if (n >= 0) {
        f->end_tag = f->tag + n;
    }

}


// io61_close(f)
//    Close the io61_file `f` and release all its resources.

int io61_close(io61_file* f) {
    io61_flush(f);
    int r = close(f->fd);
    delete f;
    return r;
}


// io61_readc(f)
//    Read a single (unsigned) character from `f` and return it. Returns EOF
//    (which is -1) on error or end-of-file.

int io61_readc(io61_file* f) {
    unsigned char buf[1];
    if (f->pos_tag == f->end_tag) {
    	io61_fill(f);
    	if (f->pos_tag == f->end_tag) {
    		return EOF;
    	}    
    }
    buf[0] = f->cache[f->pos_tag - f->tag];
    ++f->pos_tag;
    return buf[0];
}


// io61_read(f, buf, sz)
//    Read up to `sz` characters from `f` into `buf`. Returns the number of
//    characters read on success; normally this is `sz`. Returns a short
//    count, which might be zero, if the file ended before `sz` characters
//    could be read. Returns -1 if an error occurred before any characters
//    were read.

ssize_t io61_read(io61_file* f, char* buf, size_t sz) {
    // Check invariants.    
    size_t pos = 0; //bytes read
    
    while (pos < sz){
    	//If we're at the end of the cache, we refil
    	if (f->end_tag == f->pos_tag){
    		io61_fill(f);

    		if (f->end_tag == f->pos_tag){
    			break;
    		}
    	}
    	//If the number of bytes left to read is lower then cache space, we memcpy the remaining bytes
    	if ((size_t) (f->end_tag-f->pos_tag) >= sz-pos){
    		memcpy(&buf[pos], &f->cache[f->pos_tag-f->tag], sz-pos);
    		f->pos_tag = f->pos_tag + sz - pos;
    		pos = sz;
    	}
    	//If the number of bytes left to read is more then what is in cache, we copy only what we have in the cache for now.
    	else {
    		off_t bytes_to_copy = f->end_tag -f->pos_tag;
    		memcpy(&buf[pos], &f->cache[f->pos_tag-f->tag], bytes_to_copy);
    		pos = pos + bytes_to_copy;
    		f->pos_tag = f->end_tag;
    	}
    	    
    }
    return pos;
}
    
    // Note: This function never returns -1 because `io61_readc`
    // does not distinguish between error and end-of-file.
    // Your final version should return -1 if a system call indicates
    // an error.



// io61_writec(f)
//    Write a single character `ch` to `f`. Returns 0 on success or
//    -1 on error.

int io61_writec(io61_file* f, int ch) {
    unsigned char buf[1];
    buf[0] = ch;
    if (f->end_tag == f->tag + f->bufsize){
    	io61_flush(f);
    }
    
    f->cache[f->pos_tag - f->tag] = buf[0];
    ++f->pos_tag;
    ++f->end_tag;
    return 0;
}


// io61_write(f, buf, sz)
//    Write `sz` characters from `buf` to `f`. Returns the number of
//    characters written on success; normally this is `sz`. Returns -1 if
//    an error occurred before any characters were written.

ssize_t io61_write(io61_file* f, const char* buf, size_t sz) {

    size_t pos = 0;
    size_t temp = sz;
    while (pos < sz) {
    	//Flush the cache, if we're at the end
        if (f->end_tag == f->tag + f->bufsize) {
            io61_flush(f);
        }
        temp = sz - pos;
        // If bytes left to write is lower then what is in cache, write that amount of bytes, flush one last time.
        if ((off_t)temp <= f->tag + f->bufsize - f->end_tag){
        	memcpy(&f->cache[f->pos_tag-f->tag], &buf[pos], temp);
        	f->pos_tag = f->pos_tag + temp;
        	f->end_tag = f->end_tag + temp;
        	pos = pos + temp;
        	io61_flush(f);

        
        }
        //If bytes left to write is more then what is in cache, write what we have space for now to cache
        else {
        	off_t bytes_to_copy = f->tag + f->bufsize - f->end_tag;
        	memcpy(&f->cache[f->pos_tag-f->tag], &buf[pos], bytes_to_copy);
        	f->pos_tag = f->pos_tag + bytes_to_copy;
        	f->end_tag = f->end_tag + bytes_to_copy;
        	pos = pos + bytes_to_copy;
		
        }

    }
    return pos;
}

// io61_flush(f)
//    Forces a write of all buffered data written to `f`.
//    If `f` was opened read-only, io61_flush(f) may either drop all
//    data buffered for reading, or do nothing.

int io61_flush(io61_file* f) {

    if (f->mode == O_RDONLY) {
    	return 0;
    }
    
    ssize_t n = write(f->fd, f->cache, f->pos_tag - f->tag);
    f->tag = f->pos_tag;
    return n;
}


// io61_seek(f, pos)
//    Change the file pointer for file `f` to `pos` bytes into the file.
//    Returns 0 on success and -1 on failure.

int io61_seek(io61_file* f, off_t pos) {
    if (pos >= f->tag && pos <=f->end_tag){
    	io61_flush(f);
    	f->pos_tag = pos;
	return 0;    
    }
    
    //If file is read only, want to flush then sys call to set the new position
    if (f->mode == O_WRONLY){
    	io61_flush(f);
    	off_t new_pos = lseek(f->fd, pos, SEEK_SET);
    	
    	if (new_pos == pos){
    		f->pos_tag = f->tag = f->end_tag = pos;
    	}
    	else {
    		return -1;
    	}   
    }
    
    //For write only files, we set to fill the cache starting slightly earlier then requested position to a position 1-4096 bytes earlier which is better for reverse reads.
    else {
    	off_t aligned_pos = pos -(pos % f->bufsize);
    	f->pos_tag = f->tag = f->end_tag = aligned_pos;
    	off_t try_pos = lseek(f->fd, aligned_pos, SEEK_SET);
    	
    	if (try_pos == aligned_pos) {
    		io61_fill(f);
    		f->pos_tag = pos;
    	}
    	
    	else {
    		return -1;
    	} 
    }
    return 0;
      
}


// You shouldn't need to change these functions.

// io61_open_check(filename, mode)
//    Open the file corresponding to `filename` and return its io61_file.
//    If `!filename`, returns either the standard input or the
//    standard output, depending on `mode`. Exits with an error message if
//    `filename != nullptr` and the named file cannot be opened.

io61_file* io61_open_check(const char* filename, int mode) {
    int fd;
    if (filename) {
        fd = open(filename, mode, 0666);
    } else if ((mode & O_ACCMODE) == O_RDONLY) {
        fd = STDIN_FILENO;
    } else {
        fd = STDOUT_FILENO;
    }
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    return io61_fdopen(fd, mode & O_ACCMODE);
}


// io61_filesize(f)
//    Return the size of `f` in bytes. Returns -1 if `f` does not have a
//    well-defined size (for instance, if it is a pipe).

off_t io61_filesize(io61_file* f) {
    struct stat s;
    int r = fstat(f->fd, &s);
    if (r >= 0 && S_ISREG(s.st_mode)) {
        return s.st_size;
    } else {
        return -1;
    }
}
