#include <stdint.h>


void lock_acquire(int *lock)
{
	int temp0 = 1;

	__asm__(
		"loop%=: "
		"amoswap.w.aq %1, %1, (%0);"
		"bnez %1,loop%="
		://no output register
		:"r" (lock), "r" (temp0)
		:/*no clobbered registers*/
	);
}

void lock_release (int *lock)
{
	__asm__(
		"amoswap.w.rl x0, x0, (%0);"// Release lock by storing 0.
		://no output
		:"r" (lock)
		://no clobbered register
	);
}

static inline int and_gate (int *arr, int size) {
  asm volatile("fence rw, rw;");
	int rslt = 1;
	for (int i = 0; i < size; i++){
		rslt = rslt & arr[i];
	}
	return rslt;
}

static inline int or_gate (int *arr, int size) {
  asm volatile("fence rw, rw;");
	int rslt = 0;
	for (int i = 0; i < size; i++){
		rslt = rslt | arr[i];
	}
	return rslt;
}

static inline int or_gate_checkercores (int *arr, int size) {
  	asm volatile("fence rw, rw;");
	
  	int rslt = 0;
  	for (int i = 0; i < size - 1; i++){
		if (i != 0){
			// 0 is the big core.
			rslt = rslt | arr[i];
		} else {
			rslt = rslt;
		}
	}
	return rslt;
}