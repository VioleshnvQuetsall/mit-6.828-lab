// Concurrent version of prime sieve of Eratosthenes.
// Invented by Doug McIlroy, inventor of Unix pipes.
// See http://swtch.com/~rsc/thread/.
// The picture halfway down the page and the text surrounding it
// explain what's going on here.
//
// Since NENV is 1024, we can print 1022 primes before running out.
// The remaining two environments are the integer generator at the bottom
// of main and user/idle.

#include <inc/lib.h>

unsigned
primeproc(void)
{
	int i, id, p;
	envid_t envid;

	// fetch a prime from our left neighbor
	do {
		p = ipc_recv(&envid, 0, 0);
		cprintf("CPU %d: %d ", thisenv->env_cpunum, p);

		// fork a right neighbor to continue the chain
		if ((id = fork()) < 0) panic("fork: %e", id);
	} while (id == 0);

	// filter out multiples of our prime
	while (1) {
		i = ipc_recv(&envid, 0, 0);
		if (i % p) ipc_send(id, i, 0, 0);
	}
}

void
umain(int argc, char **argv)
{
	int i = 2, id = fork();

	// fork the first prime process in the chain
	if (id < 0) panic("fork: %e", id);
	if (id == 0) primeproc();

	// feed all the integers through
	while (1) ipc_send(id, i++, 0, 0);
}

