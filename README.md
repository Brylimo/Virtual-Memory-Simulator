## Project #3: Virtual Memory Simulator

### *** Due on 24:00, June 11 (Friday) ***


### Goal
Implement a mini virtual memory system simulator.


### Problem Specification
- The framework provides an environment to simulate virtual memory and paging. Your task is to implement the key components of the simulator.

- In the simulator, we may have multiple processes. Likewise PA2, `struct process` abstracts processes on the system, and `struct process *current` points to the currently running process. `struct list_head processes` is a list of processes on the system.

- The framework accepts five main commands, which are `read`, `write`, `alloc`, `free`, and `switch`, and three supplementary commands, which are `show`, `pages`, and `exit`. Try to enter `help` or `?` for brief explanation for each command.

- `alloc` is to instruct the simulator to simulate a page allocation to the current process. Following shows the cases for the command. Note that this command *does not actually allocate a real page nor memory resource*, but *do simulate the situation for page allocation*.

  ```
  >> alloc 10 r	    # Allocate a page frame for VPN 10 for read
  >> alloc 20 rw    # Allocate a page frame for VPN 20 for read and write
  >> alloc 0x10 rw  # Allocate a page to VPN 0x10 for read and write
	```

- The pages allocated with `r` option are read-only pages. Writes to those pages should be rejected. The pages allocated with `rw` can be read and written. This implies that both read and write accesses to those VPNs should be allowed. However, you may presume that no page will be allocated with only `w` option in this assignment (i.e., no need to support write-only pages).

- Allocated pages should be mapped to the current process by manipulating the page table of the process. The system maintains 2-level hierarchical page table as defined in `vm.h`.

- `mapcounts[]`  is an array that is supposed to contain the numbers of PTE mappings to each page frame. For example, when a page frame `x` is mapped to three processes, `mapcounts[x]` should be 3. You may leverage this information to find a free page frame to allocate.

- When the system has many free page frames, allocate the page frame that has the smallest page frame number.

- `free` command is to deallocate the page that is mapped to the VPN. The page table should be set so that subsequent accesses to the freed VPN should be denied by MMU. You should consider the case when the target page frame is mapped more than or equal to 2 to properly handle `free` command with copy-on-write feature.

- `read` and `write` is to instruct the system to simulate the memory access. These commands are followed by VPN (virtual page number). For example;

	```
	>> read 10    /* Read VPN 10 */
	>> write 0x10 /* Write to VPN 0x10 */
  ```

- Each read and write request will be processed by the framework. Internally, it calls `__translate()` in `vm.c`, which simulates the address translation in MMU. It walk through the current page table, which is pointed by `ptbr`, to translate VPN to PFN.

- When the translation is successful, the framework will print out the translation result, and waits for next commands from the prompt.

- If the given VPN cannot be translated or accessed using the current page table, it will trigger the page fault mechanism in the framework by calling `handle_page_fault()`. In this page fault handler, your code should inspect the situation causing the page fault, and resolve the fault if it can handle with. To this end, you may modify/allocate/fix up the page table in this function.

- You may switch the currently running process with `switch` command. Enter the command followed by the process id to switch to. The framework will call `switch_process()` to handle the request. Find the target process from the `processes` list, and if it exists, do the context switching by replacing `current` and `ptbr` with the requested process.

- If the target process does not exist, you need to fork a child process from `current`. This implies you should allocate `struct process` for the child process and initialize it (including page table) accordingly.
To duplicate the parent's address space, set up the PTE in the child's page table to map to the same PFN of the parent. You need to set up PTE property bits to support copy-on-write.

- `show` prompt command shows the page table of the current process. `pages` command shows the summary for `mapcounts[]`.


### Tips and Restriction
- Implement features in an incremental way; implement the allocation/deallocation functions first to get used to the page table/PTE manipulation. And then move on to implement the fork by duplicating the page table contents. You need to manipulate both PTEs of parent and child to support copy-on-write properly.
- Be careful to handle `writable` bit in the page table when you attach a page or share it. Read-only pages should not be writable after the fork whereas writable pages should be writable after the fork through the copy-on-write mechanism. You can leverage the `private` variable in `struct pte` to implement this feature.
- Likewise previous PAs, printing out to stdout does not influence on the grading. So, feel free to print out debug message using `printf`.


### Submission / Grading
- Use [PAsubmit](https://sslab.ajou.ac.kr/pasubmit) for submission
	- 450 pts + 10 pts

- Code: ***pa3.c*** (350 pts)
	- Page allocation (50 pts)
	- Page deallocation (50 pts)
	- Fork (100 pts)
	- Copy-on-Write (150 pts)

- Document: One PDF document (100 pts) including;
	- Describe how you implement page allocation, deallocation, fork, and copy-on-write.
		- Describe your *entire journey to complete the assignment*. You must clarify why you choose the particular design you use over other alternatives, what is your key idea in implementing those features, explains your failed tries, the analysis on the causes of those failures, and how you resolved them.
		- Bad example: To implement page allocation, find xyz by increasing from 0 to 9999. Then increase x by 2. If xx > 0, do something.
		- Good example: To implement page allocation, firstly I used abc properties. Since it is xyz, it is kkk if xyz is 99. So, try to find something like plm by iterating the cde list and finds something with xxx. This value is increased by 2 to indicate something for xyz. I thought this should work, but it did not when xyz is 0xdeadbeef. It turned out that xyz is not true but abc is false. So, I modified it ....
	  - *Must include at least one case of your failure, what the problem was, and how you fixed it.*
	- Explain what happens to the page tables while running `testcases/cow-2`. *Show the final page tables for process 0, 1, and 2, and explain why they are like that*.
	- Lesson learned
	- No more than six pages

- Git repository (10 bonus pts)
	- Register http/https URL and with a deploy token and password.
	- Start the repository by cloning this repository.
	- Make sure your repository is private.
	- Also, make sure the token is valid through June 15 (due + 3 slip days + 1 day)

- *THE INSTRUCTOR WILL NOT ANSWER THE QUESTIONS ABOUT THOSE ALREADY SPECIFIED ON THE HANDOUT.*
- *QUESTIONS OVER EMAIL WILL BE IGNORED UNLESS IT CONCERNS YOUR PRIVACY.*
