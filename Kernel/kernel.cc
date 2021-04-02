#include "kernel.hh"
#include "k-apic.hh"
#include "k-vmiter.hh"
#include <atomic>

// kernel.cc
//
//    This is the kernel.


// INITIAL PHYSICAL MEMORY LAYOUT
//
//  +-------------- Base Memory --------------+
//  v                                         v
// +-----+--------------------+----------------+--------------------+---------/
// |     | Kernel      Kernel |       :    I/O | App 1        App 1 | App 2
// |     | Code + Data  Stack |  ...  : Memory | Code + Data  Stack | Code ...
// +-----+--------------------+----------------+--------------------+---------/
// 0  0x40000              0x80000 0xA0000 0x100000             0x140000
//                                             ^
//                                             | \___ PROC_SIZE ___/
//                                      PROC_START_ADDR

#define PROC_SIZE 0x40000       // initial state only

proc ptable[NPROC];             // array of process descriptors
                                // Note that `ptable[0]` is never used.
proc* current;                  // pointer to currently executing proc

#define HZ 100                  // timer interrupt frequency (interrupts/sec)
static std::atomic<unsigned long> ticks; // # timer interrupts so far


// Memory state
//    Information about physical page with address `pa` is stored in
//    `pages[pa / PAGESIZE]`. In the handout code, each `pages` entry
//    holds an `refcount` member, which is 0 for free pages.
//    You can change this as you see fit.

pageinfo pages[NPAGES];


[[noreturn]] void schedule();
[[noreturn]] void run(proc* p);
void exception(regstate* regs);
uintptr_t syscall(regstate* regs);
void memshow();

// kernel_start(command)
//    Initialize the hardware and processes and start running. The `command`
//    string is an optional string passed from the boot loader.

static void process_setup(pid_t pid, const char* program_name);

void kernel_start(const char* command) {
    // initialize hardware
    init_hardware();
    log_printf("Starting WeensyOS\n");

    ticks = 1;
    init_timer(HZ);

    // clear screen
    console_clear();

    // (re-)initialize kernel page table
    for (vmiter it(kernel_pagetable);
         it.va() < MEMSIZE_PHYSICAL;
         it += PAGESIZE) {
        if (it.va() != 0) {
            int flags;
            if (it.va() < PROC_START_ADDR && it.va() != CONSOLE_ADDR){
                flags = PTE_P | PTE_W;
            }
            else {
                flags = PTE_P | PTE_W | PTE_U;
            }
            it.map(it.va(), flags);
        } else {
            // nullptr is inaccessible even to the kernel
            it.map(it.va(), 0);
        }
    }

    // set up process descriptors
    for (pid_t i = 0; i < NPROC; i++) {
        ptable[i].pid = i;
        ptable[i].state = P_FREE;
    }
    if (command && !program_image(command).empty()) {
        process_setup(1, command);
    } else {
        process_setup(1, "allocator");
        process_setup(2, "allocator2");
        process_setup(3, "allocator3");
        process_setup(4, "allocator4");
    }

    // Switch to the first process using run()
    run(&ptable[1]);
}


// kalloc(sz)
//    Kernel memory allocator. Allocates `sz` contiguous bytes and
//    returns a pointer to the allocated memory, or `nullptr` on failure.
//
//    The returned memory is initialized to 0xCC, which corresponds to
//    the x86 instruction `int3` (this may help you debug). You'll
//    probably want to reset it to something more useful.
//
//    On WeensyOS, `kalloc` is a page-based allocator: if `sz > PAGESIZE`
//    the allocation fails; if `sz < PAGESIZE` it allocates a whole page
//    anyway.
//
//    The handout code returns the next allocatable free page it can find.
//    It checks all pages. (You could maybe make this faster!)
void sys_exit(pid_t this_pid);

void* kalloc(size_t sz) {
    if (sz > PAGESIZE) {
        return nullptr;
    }

    for (uintptr_t pa = 0; pa != MEMSIZE_PHYSICAL; pa += PAGESIZE) {
        if (allocatable_physical_address(pa)
            && !pages[pa / PAGESIZE].used()) {
            ++pages[pa / PAGESIZE].refcount;
            memset((void*) pa, 0xCC, PAGESIZE);
            return (void*) pa;
        }
    }
    return nullptr;
}


// kfree(kptr)
//    Free `kptr`, which must have been previously returned by `kalloc`.
//    If `kptr == nullptr` does nothing.

void kfree(void* kptr) {
    (void) kptr;
    if (kptr == nullptr || !allocatable_physical_address((uintptr_t) kptr)){       
        //do nothing 
    }
    else{
        if (pages[(uintptr_t) kptr / PAGESIZE].refcount > 0) {
            --pages[(uintptr_t) kptr / PAGESIZE].refcount;
        }
        assert(pages[(uintptr_t) kptr / PAGESIZE].refcount >= 0);
       
        }    
}


// process_setup(pid, program_name)
//    Load application program `program_name` as process number `pid`.
//    This loads the application's code and data into memory, sets its
//    %rip and %rsp, gives it a stack page, and marks it as runnable.

void process_setup(pid_t pid, const char* program_name) {
    init_process(&ptable[pid], 0);

    // initialize process page table
    ptable[pid].pagetable = kernel_pagetable;

    x86_64_pagetable *new_pagetable = kalloc_pagetable();
    
   for (vmiter it(new_pagetable), kit(kernel_pagetable);
        it.va() < PROC_START_ADDR;
        it += PAGESIZE, kit += PAGESIZE){
        it.map(kit.pa(), kit.perm());
        };
    

   ptable[pid].pagetable = new_pagetable;

    // obtain reference to the program image
    program_image pgm(program_name);

    // allocate and map all memory
    for (auto seg = pgm.begin(); seg != pgm.end(); ++seg) {
        for (uintptr_t a = round_down(seg.va(), PAGESIZE);
             a < seg.va() + seg.size();
             a += PAGESIZE) {

            if (seg.writable()){
                vmiter(ptable[pid].pagetable, a).map(kalloc(PAGESIZE),PTE_P | PTE_W | PTE_U);
            }
            else {
                vmiter(ptable[pid].pagetable, a).map(kalloc(PAGESIZE),PTE_P | PTE_U);
            }
        }
    }

    // copy instructions and data into place
    for (auto seg = pgm.begin(); seg != pgm.end(); ++seg) {
        memset((void*) vmiter(new_pagetable,seg.va()).pa(), 0, seg.size());
        memcpy((void*) vmiter(new_pagetable,seg.va()).pa(), seg.data(), seg.data_size());
    }

    // mark entry point
    ptable[pid].regs.reg_rip = pgm.entry();

    // allocate stack
    uintptr_t stack_addr = MEMSIZE_VIRTUAL - PAGESIZE;
    assert(!pages[stack_addr / PAGESIZE].used());
    vmiter(ptable[pid].pagetable, stack_addr).map(kalloc(PAGESIZE), PTE_P | PTE_W | PTE_U);
    ptable[pid].regs.reg_rsp = stack_addr + PAGESIZE;

    // mark process as runnable
    ptable[pid].state = P_RUNNABLE;
}



// exception(regs)
//    Exception handler (for interrupts, traps, and faults).
//
//    The register values from exception time are stored in `regs`.
//    The processor responds to an exception by saving application state on
//    the kernel's stack, then jumping to kernel assembly code (in
//    k-exception.S). That code saves more registers on the kernel's stack,
//    then calls exception().
//
//    Note that hardware interrupts are disabled when the kernel is running.

void exception(regstate* regs) {
    // Copy the saved registers into the `current` process descriptor.
    current->regs = *regs;
    regs = &current->regs;

    // It can be useful to log events using `log_printf`.
    // Events logged this way are stored in the host's `log.txt` file.
    /* log_printf("proc %d: exception %d at rip %p\n",
                current->pid, regs->reg_intno, regs->reg_rip); */

    // Show the current cursor location and memory state
    // (unless this is a kernel fault).
    console_show_cursor(cursorpos);
    if (regs->reg_intno != INT_PF || (regs->reg_errcode & PFERR_USER)) {
        memshow();
    }

    // If Control-C was typed, exit the virtual machine.
    check_keyboard();


    // Actually handle the exception.
    switch (regs->reg_intno) {

    case INT_IRQ + IRQ_TIMER:
        ++ticks;
        lapicstate::get().ack();
        schedule();
        break;                  /* will not be reached */

    case INT_PF: {
        // Analyze faulting address and access type.
        uintptr_t addr = rdcr2();
        const char* operation = regs->reg_errcode & PFERR_WRITE
                ? "write" : "read";
        const char* problem = regs->reg_errcode & PFERR_PRESENT
                ? "protection problem" : "missing page";

        if (!(regs->reg_errcode & PFERR_USER)) {
            panic("Kernel page fault on %p (%s %s)!\n",
                  addr, operation, problem);
        }
        console_printf(CPOS(24, 0), 0x0C00,
                       "Process %d page fault on %p (%s %s, rip=%p)!\n",
                       current->pid, addr, operation, problem, regs->reg_rip);
        current->state = P_BROKEN;
        break;
    }

    default:
        panic("Unexpected exception %d!\n", regs->reg_intno);

    }


    // Return to the current process (or run something else).
    if (current->state == P_RUNNABLE) {
        run(current);
    } else {
        schedule();
    }
}


// syscall(regs)
//    System call handler.
//
//    The register values from system call time are stored in `regs`.
//    The return value, if any, is returned to the user process in `%rax`.
//
//    Note that hardware interrupts are disabled when the kernel is running.

int syscall_page_alloc(uintptr_t addr);
int fork();

uintptr_t syscall(regstate* regs) {
    // Copy the saved registers into the `current` process descriptor.
    current->regs = *regs;
    regs = &current->regs;

    // It can be useful to log events using `log_printf`.
    // Events logged this way are stored in the host's `log.txt` file.
    /* log_printf("proc %d: syscall %d at rip %p\n",
                  current->pid, regs->reg_rax, regs->reg_rip); */

    // Show the current cursor location and memory state.
    console_show_cursor(cursorpos);
    memshow();

    // If Control-C was typed, exit the virtual machine.
    check_keyboard();


    // Actually handle the exception.
    switch (regs->reg_rax) {

    case SYSCALL_PANIC:
        panic(nullptr);         // does not return

    case SYSCALL_GETPID:
        return current->pid;

    case SYSCALL_YIELD:
        current->regs.reg_rax = 0;
        schedule();             // does not return

    case SYSCALL_PAGE_ALLOC:
        return syscall_page_alloc(current->regs.reg_rdi);

    case SYSCALL_FORK:
        return fork();

    case SYSCALL_EXIT:
        sys_exit(current->pid);
        schedule();

    default:
        panic("Unexpected system call %ld!\n", regs->reg_rax);

    }

    panic("Should not get here!\n");
}


// syscall_page_alloc(addr)
//    Handles the SYSCALL_PAGE_ALLOC system call. This function
//    should implement the specification for `sys_page_alloc`
//    in `u-lib.hh` (but in the handout code, it does not).

int syscall_page_alloc(uintptr_t addr) {
    if (addr < PROC_START_ADDR || addr > MEMSIZE_VIRTUAL || addr % PAGESIZE != 0) {
        return -1;
    }
    //assert(!pages[addr / PAGESIZE].used());
    //pages[addr / PAGESIZE].refcount = 1;
    auto temp = kalloc(PAGESIZE);

    if (temp == nullptr) {
        return -1;
    }

    int c = vmiter(current->pagetable,addr).try_map(temp, PTE_P | PTE_W | PTE_U);
    if (c == -1) {
        kfree(temp);
        return -1;
    }
   
    
    memset((void*) vmiter(current->pagetable, addr).pa(), 0, PAGESIZE);
    return 0;
    
}


// schedule
//    Pick the next process to run and then run it.
//    If there are no runnable processes, spins forever.

void schedule() {
    pid_t pid = current->pid;
    for (unsigned spins = 1; true; ++spins) {
        pid = (pid + 1) % NPROC;
        if (ptable[pid].state == P_RUNNABLE) {
            run(&ptable[pid]);
        }

        // If Control-C was typed, exit the virtual machine.
        check_keyboard();

        // If spinning forever, show the memviewer.
        if (spins % (1 << 12) == 0) {
            memshow();
        }
    }
}


// run(p)
//    Run process `p`. This involves setting `current = p` and calling
//    `exception_return` to restore its page table and registers.

void run(proc* p) {
    assert(p->state == P_RUNNABLE);
    current = p;

    // Check the process's current pagetable.
    check_pagetable(p->pagetable);

    // This function is defined in k-exception.S. It restores the process's
    // registers then jumps back to user mode.
    exception_return(p);

    // should never get here
    while (true) {
    }
}


// memshow()
//    Draw a picture of memory (physical and virtual) on the CGA console.
//    Switches to a new process's virtual memory map every 0.25 sec.
//    Uses `console_memviewer()`, a function defined in `k-memviewer.cc`.

void memshow() {
    static unsigned last_ticks = 0;
    static int showing = 0;

    // switch to a new process every 0.25 sec
    if (last_ticks == 0 || ticks - last_ticks >= HZ / 2) {
        last_ticks = ticks;
        showing = (showing + 1) % NPROC;
    }

    proc* p = nullptr;
    for (int search = 0; !p && search < NPROC; ++search) {
        if (ptable[showing].state != P_FREE
            && ptable[showing].pagetable) {
            p = &ptable[showing];
        } else {
            showing = (showing + 1) % NPROC;
        }
    }

    extern void console_memviewer(proc* vmp);
    console_memviewer(p);
}
    
int fork(){
    //find first open process if available
    pid_t find_free = 1;
    while (find_free <= NPROC){
        if (ptable[find_free].state == P_FREE){
            break;
        }
        find_free += 1;
    }

    if (find_free > NPROC){
        return -1;
    }

    //create new pagetable for child
    ptable[find_free].pagetable = kalloc_pagetable();
    if (ptable[find_free].pagetable ==nullptr){
        kfree(ptable[find_free].pagetable);
        return -1;
    }
    ptable[find_free].pid = find_free;

    //Iterate through new and old pagetables and map accordingly
    for (vmiter it(ptable[find_free].pagetable), cit(current->pagetable);
        it.va() < MEMSIZE_VIRTUAL;
        it += PAGESIZE, cit += PAGESIZE){
            //If writable, we kalloc a new space for the child
            if (cit.present() && cit.user() && cit.writable() && cit.pa() != CONSOLE_ADDR){
                void* addr =  kalloc(PAGESIZE);                  
                if (addr == nullptr){
                    sys_exit(find_free);
                    return -1;
                }
                int b = it.try_map(addr, PTE_P | PTE_W | PTE_U);  
                if (b == -1) {
                    kfree(addr);
                    sys_exit(find_free);
                    return -1;
                } 
                memcpy(addr, (void*) vmiter(current->pagetable,cit.va()).pa(), PAGESIZE);       
            }
            
            //If not writable, we don't need to create a new space for the child and can just map directly 
            else {
                int success = it.try_map(cit.pa(), cit.perm()); 
                if (success == -1){
                    sys_exit(find_free);
                    return -1;
                }

            }
            //Writable pages were added to refcount when we did kalloc but user pages that arent writable also need to be which we do here. 
            if (cit.user() && !cit.writable()){
                ++pages[cit.pa() / PAGESIZE].refcount;   
            }
        }
    
    //Copy registers except for rax which is 0 for the child. 
    ptable[find_free].regs = current -> regs;
    ptable[find_free].regs.reg_rax = 0;
    ptable[find_free].state = P_RUNNABLE;

    //rax for the parent will become pid of the child when we return. 
    return find_free;
}

void sys_exit(pid_t this_pid){
    //free unpriveledged pages
    for (vmiter it(ptable[this_pid].pagetable); it.va() < MEMSIZE_VIRTUAL; it += PAGESIZE) {
        if (it.pa() != CONSOLE_ADDR && it.user()) {
            kfree((void*) it.pa());
        }

    //free pages that make up pagetables
        for (ptiter pit(ptable[this_pid].pagetable); pit.va() < MEMSIZE_VIRTUAL; pit.next()){            
            kfree((void*) pit.pa());
            
        }

        ptable[this_pid].state = P_FREE;
        kfree(ptable[this_pid].pagetable);
    }

}