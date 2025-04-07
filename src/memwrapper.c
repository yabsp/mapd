#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

//compile to shared library with "gcc -fPIC -shared -o libmemwrap.so memwrap.c -ldl"
//Afterwards use "LD_PRELOAD=./libmemwrap.so ./my_program" on a normally compiled binary "my_program"

void* malloc(size_t size) 
{
	void* (*real_malloc)(size_t) = dlsym(RTLD_NEXT, "malloc");
	void* ptr = real_malloc(size);
	fprintf(stderr, "[WRAP] malloc(%zu) = %p\n", size, ptr);

	return ptr;
}

void free(void *ptr) 
{
	void (*real_free)(void*) = dlsym(RTLD_NEXT, "free");
	fprintf(stderr, "[WRAP] free(%p)\n", ptr);
	real_free(ptr);
}
