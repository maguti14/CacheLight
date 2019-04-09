/*
 * \brief   Kernel backend for virtual machines
 * \author  Martin Stein
 * \author  Stefan Kalkowski
 * \date    2013-10-30
 */

/*
 * Copyright (C) 2013-2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* core includes */
#include <kernel/cpu.h>
#include <kernel/vm.h>
#include <drivers/trustzone.h>
#include <platform.h>
#include <map_local.h>
#include <platform.h>
#include <hw/memory_map.h>

using namespace Kernel;


Kernel::Vm::Vm(void                   * const state,
               Kernel::Signal_context * const context,
               void                   * const /* table */)
:  Cpu_job(Cpu_priority::MIN, 0),
  _state((Genode::Vm_state * const)state),
  _context(context), _table(0) {
	affinity(cpu_pool()->primary_cpu()); }


Kernel::Vm::~Vm() {}


void Vm::exception(Cpu & cpu)
{
	switch(_state->cpu_exception) {
	case Genode::Cpu_state::INTERRUPT_REQUEST:
	case Genode::Cpu_state::FAST_INTERRUPT_REQUEST:
		_interrupt(cpu.id());
		return;
	case Genode::Cpu_state::DATA_ABORT:
		_state->dfar = Cpu::Dfar::read();
	default:
		pause();
		if(_state->r0 == 4) // smc for cacheLock
		{
			int test[4] = {0, 1, 2, 3};
			Genode::log("Test: ", test[0]);
			Genode::log("CPU Exception: ", _state->cpu_exception);
			/* Peform VA -> PA address translation and check for consistency */
			unsigned int paT;
			unsigned int temp;
			unsigned int threshold = 0x7FFFFFFF; //TODO: Would be better to Use the RAM range given to VM instead?
			unsigned int lockReg = _state->r2;
			unsigned int unlockReg = (~(lockReg))&0xFF;
			//unsigned int mmu = 0; 
			//int volatile * const addr = (int *) _state->r1;
			//for(int i = 0; i < 10; i++)
			//	addr[i] = 0x01010101;
			//int vaT = _state->r1; // VA
			Genode::log("VA: ", _state->r1);
			asm volatile(
			"mcr p15, 0, %0, c7, c8, 4\n\t" // Translate VA in context of NW
			"isb					  \n\t"
			:   // output
			: "r" (_state->r1) // input
			); 
			Genode::log("Translated addr");
			asm volatile(
			"mrc p15, 0, %0, c7, c4, 0\n\t" //read PA from PAR
			: "=r"(paT)  // output
			:  // input
			); 
			Genode::log("PAR is: ", paT);
			temp = (unsigned int) _state->r1 & 0xFFF; // bottom twelve bits of VA 
			paT = paT & 0xFFFFF000; // PA in top bits only
			
			paT = paT | temp; //whole PA comes from top bits of PAR and last 12 bits of VA
			
			Genode::log("Threshold is: ", threshold);
			Genode::log("PA is: ", paT);
			
			if(paT < Trustzone::NONSECURE_RAM_BASE || (paT > Trustzone::NONSECURE_RAM_BASE + Trustzone::NONSECURE_RAM_SIZE))
			{
				Genode::warning("Invalid Physical address mapping! Dumping memory and running introspection tools!");
				// TODO: Modify virtual address mapping, dump memory and run instrospection tools
				Genode::warning("Caught with your hand in the proverbial cookie jar!");
			}
			else
			{
				Genode::log("Valid address, Genode performing cache locking for you!");
				/* Enter Monitor Mode
				int mode;
				asm volatile("mrs %0, cpsr\n" : "=r" (mode));
				mode = mode & 0x1f;
				Genode::log("Mode: ", mode);
				Genode::log("Entering MON");
				asm volatile("cps #22	\n\t"); 
				Genode::log("Entered MON");
				asm volatile("mrs %0, cpsr\n" : "=r" (mode));
				mode = mode & 0x1f;
				Genode::log("Mode: ", mode);
				asm volatile("cps #19	\n\t");
				
				unsigned int secPA;
				for(unsigned int i = 0x00000fff; i <= 0xffffffff; i+=0x00001000)
				{
					asm volatile(
					"mcr p15, 0, %0, c7, c8, 3\n\t" // Translate VA in context of NW
					"isb					  \n\t"
					:   // output
					: "r" (i) // input
					); 
					asm volatile(
					"mrc p15, 0, %0, c7, c4, 0\n\t" //read PA from PAR
					: "=r"(secPA)  // output
					:  // input
					); 
					temp = i & 0xFFF; // bottom twelve bits of VA
					secPA = secPA & 0xFFFFF000; // PA in top bits only
			
					secPA = secPA | temp; //whole PA comes from top bits of PAR and last 12 bits of VA
			
					Genode::log("Secure PA is: ", i);
					if(secPA == paT)
						break; 
				}
				*/
				// disables interrupts
				Genode::log("Disable Interrupts");
				//Genode::map_local(paT, Hw::Mm::core_NW().base, 1, Hw::PAGE_FLAGS_KERN_DATA);
				addr_t swVA = Genode::Platform::nw_to_virt(paT); // building offset?  0xe0000000 + (paT - 0x80000000);
				Genode::log("SW VA: ", swVA);
				//asm volatile("cpsid if	\n\t"); // Mask IRQ and FIQ
				
				/*
				Genode::log("Entering Monitor Mode");
				asm volatile("cps #22	\n\t");
			
				// Set NS bit to map to Normal world - Monitor Mode always operates in secure regardless of NS bit 
				int scr; 
			
				asm volatile(
				"mrc p15, 0, %0, c1, c1, 0\n\t" //read SCR bit0 = 0 = secure world
				"isb					  \n\t"
				: "=r"(scr)  // output
				:  // input
				); 
				Genode::log("Read SCR: ", scr);
				scr = scr | 0x00000001; // set NS bit
				//"orr r4, r4, #0x0001 \n" : "=r" (scr) : "r" (scr) : 
				asm volatile(
				"mcr p15, 0, %0, c1, c1, 0\n\t" //write SCR bit0 = 1 = NS
				"isb					  \n\t"
				:   // output
				: "r" (scr) // input
				); 
			
				Genode::log("Set NS bit: ", scr);
				
				Genode::log("Disable MMU: ");

				// Disable MMU TODO: Do this without disabling MMU
				asm volatile(
				"mrc p15, 0, %0, c1, c0, 0\n\t" // read CP15 register 1
				"bic %0, %0, #0x1		  \n\t" // disabled
				"mcr p15, 0, %0, c1, c0, 0\n\t" // write CP15 register 1
				"isb					  \n\t"
				"dsb					  \n\t"
				: "=r"(mmu)  // output
				: "r" (mmu) // input
				); 
				*/
				//printing c9, L2 cache lock down register
				asm volatile(
			//			// clear the lock reg
						"MCR p15, 1, %0, c9, c0, 0		\n\t"


						// clear all the cache
						//clear the L1 first
						"MOV R2, #4							\n\t"
						"lockMem_loop_l1outer:						\n\t"
						"SUBS R2,#1							\n\t"
						"MOV R1, #128						\n\t"
							"lockMem_loop_l1inner:					\n\t"
							"SUBS R1,#1						\n\t"
							// now figure out the masking for level 0, that is
							// way 31:30, set 12:6, level 3:1
							"MOV R3, R2, lsl #30			\n\t"
							"ADD R3, R3, R1, lsl #6			\n\t"
			//				"MCR p15, 0, R3, c7, c6, 2  	\n\t"   // invalidate by set way
							"MCR p15, 0, R3, c7, c14, 2		\n\t"	// clean and invalidate by set way
			//				"MCR p15, 0, R3, c7, c10, 2		\n\t"	// clean by set/way
							"cmp R1, #0						\n\t"
							"BNE lockMem_loop_l1inner				\n\t"
						"cmp R2, #0							\n\t"
						"BNE lockMem_loop_l1outer					\n\t"

						"MOV R2, #8							\n\t"
						"lockMem_loop_l2outer:						\n\t"
						"SUBS R2,#1							\n\t"
						"MOV R1, #512					\n\t"
							"lockMem_loop_l2inner:					\n\t"
							"SUBS R1,#1						\n\t"
							// now figure out the masking for level 0, that is
							// way 31:29, set 14:6, level 3:1
							"MOV  R3, #2					\n\t"
							"ADD  R3, R3, R2, lsl #29		\n\t"
							"ADD  R3, R3, R1, lsl #6		\n\t"
			//				"MCR p15, 0, R3, c7, c6, 2  	\n\t"   // invalidate by set way
							"MCR p15, 0, R3, c7, c14, 2		\n\t"	// clean and invalidate by set way
			//				"MCR p15, 0, R3, c7, c10, 2		\n\t"	// clean by set/way
							"cmp R1, #0						\n\t"
							"BNE lockMem_loop_l2inner		\n\t"
						"cmp R2, #0							\n\t"
						"BNE lockMem_loop_l2outer			\n\t"
						"	DSB								\n\t"

						//load memory area into the region
						//"LDR R1, %3			\n\t"
						//"LDR R2, %2			\n\t"
						"MOV R4, #0xA0		\n\t"
						"loop:				\n\t"
						"SUBS %3,#1			\n\t"
			// using just STRB force a write alloc, which is only done for L2
			//			"LDRB R3,[R2,R1]	\n\t"
						"STRB R4,[%2,%3]    \n\t"
						"BNE loop			\n\t"


						// set lock reg
						"MCR p15, 1, %1, c9, c0, 0\n\t"

						:  // output
						:  "r"(unlockReg), "r"(lockReg), "r"(swVA), "r"(_state->r3)// input
						: "r1","r2","r3","r4"
				);
				/*
				//enable MMU
				asm volatile(
				"mrc p15, 0, %0, c1, c0, 0\n\t" // read CP15 register 1
				"orr %0, %0, #0x1		  \n\t" // enabled
				"mcr p15, 0, %0, c1, c0, 0\n\t" // write CP15 register 1
				"isb					  \n\t"
				"dsb					  \n\t"
				"mrc p15, 0, %0, c1, c0, 0\n\t" // read CP15 register 1
				: "=r"(mmu)  // output
				: "r" (mmu) // input
				); 

				Genode::log("Enable MMU: ", mmu);
				
				// Clear NS bit 
				asm volatile(
				"mrc p15, 0, %0, c1, c1, 0\n\t" //read SCR bit0 = 0 = secure world
				"isb					  \n\t"
				: "=r"(scr)  // output
				:  // input
				); 
				scr = scr & ~(0x00000001); // set NS bit
				//"orr r4, r4, #0x0001 \n" : "=r" (scr) : "r" (scr) : 
				asm volatile(
				"mcr p15, 0, %0, c1, c1, 0\n\t" //write SCR bit0 = 1 = NS
				"isb					  \n\t"
				:   // output
				: "r" (scr) // input
				); 
			
				Genode::log("Clear NS bit: ", scr);
			
				// Exit Monitor Mode 
				asm volatile("cps #19	\n\t"); 
			
				Genode::log("Exit Monitor Mode");
				*/
				Genode::log("Enable Interrupts");
				//asm volatile("cpsie if	\n\t"); // Unmask IRQ and FIQ
				
				Genode::log("Test: ", test[0]);
			}
		}
		_context->submit(1);
	}
}


bool secure_irq(unsigned const i);


extern "C" void monitor_mode_enter_normal_world(Cpu::Context*, void*);
extern void * kernel_stack;


void Vm::proceed(Cpu & cpu)
{
	unsigned const irq = _state->irq_injection;
	if (irq) {
		if (pic()->secure(irq)) {
			Genode::warning("Refuse to inject secure IRQ into VM");
		} else {
			pic()->trigger(irq);
			_state->irq_injection = 0;
		}
	}

	monitor_mode_enter_normal_world(reinterpret_cast<Cpu::Context*>(_state),
	                                (void*) cpu.stack_start());
}
