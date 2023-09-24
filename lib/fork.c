// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
extern volatile pte_t uvpt[];
extern volatile pde_t uvpd[];
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	int perm =  PTE_COW | PTE_U | PTE_P;
	if (!(err & FEC_WR && uvpd[PDX(addr)] & PTE_P &&
	    (uvpt[PGNUM(addr)] & perm) == perm)) {
		panic("pgfault: perm mismatch (err: %x, addr: %08x)\n", err, addr);
	}

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	addr = ROUNDDOWN(addr, PGSIZE);
	if (r = sys_page_alloc(0, PFTEMP, PTE_U | PTE_W | PTE_P), r < 0) {
		panic("pgfault: sys_page_alloc failed (%e)\n", r);
	}
	memcpy(PFTEMP, addr, PGSIZE);
	if (r = sys_page_map(0, PFTEMP, 0, addr, PTE_U | PTE_W | PTE_P), r < 0) {
		panic("pgfault: sys_page_map failed (%e)\n", r);
	}
	if (r = sys_page_unmap(0, PFTEMP), r < 0) {
		panic("pgfault: sys_page_unmap failed (%e)\n", r);
	}
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r = 0;

	// LAB 4: Your code here.
	void *va = (void *)(pn * PGSIZE);
	int perm = PTE_U | PTE_P;

	if (uvpt[pn] & (PTE_W | PTE_COW)) {
		perm |= PTE_COW;
		if (r = sys_page_map(0, va, envid, va, perm), r < 0) {
			panic("duppage: sys_page_map failed (new mapping %e)\n", r);
		}
		if (r = sys_page_map(0, va, 0, va, perm), r < 0) {
			panic("duppage: sys_page_map failed (our mapping %e)\n", r);
		}
	} else {
		if (r = sys_page_map(0, va, envid, va, perm), r < 0) {
			panic("duppage: sys_page_map failed (new mapping %e)\n", r);
		}
	}
	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	extern void _pgfault_upcall(void);

	int r = 0;
	envid_t parent_id = sys_getenvid();
	set_pgfault_handler(pgfault);
	if (r = sys_exofork(), r < 0) {
		panic("[parent env %d] "
		      "fork: sys_exofork (%e)\n", parent_id, r);
	}

	envid_t child_id = r;
	if (child_id == 0) {
		thisenv = envs + ENVX(sys_getenvid());
		return 0;
	}

	// copy address space
	for (size_t ptn = 0; ptn < UTOP / PTSIZE; ptn++) {
		if (!(uvpd[ptn] & PTE_P)) continue;
		size_t lim = MIN((ptn + 1) * NPTENTRIES, USTACKTOP / PGSIZE);
		for (size_t pn = ptn * NPTENTRIES; pn < lim; pn++) {
			if (!(uvpt[pn] & PTE_P && uvpt[pn] & PTE_U)) continue;
			duppage(child_id, pn);
		}
	}

	// alloc exception stack and set page fault upcall
	if (r = sys_page_alloc(child_id, (void *)(UXSTACKTOP - PGSIZE), PTE_W | PTE_U | PTE_P),
		r < 0) {
		panic("[parent env %d] "
		      "fork: sys_page_alloc (%e)\n", parent_id, r);
	}
	if (r = sys_env_set_pgfault_upcall(child_id, (void *)_pgfault_upcall), r < 0) {
		panic("[parent_id env %d] fork: sys_env_set_pgfault_upcall "
		      "for child (%e)\n",
				parent_id, r);
	}
	if (r = sys_env_set_status(child_id, ENV_RUNNABLE), r < 0) {
		panic("[parent env %d] "
		      "fork: sys_env_set_status (%e)\n", parent_id, r);
	}

	return child_id;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
