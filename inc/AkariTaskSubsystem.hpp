#ifndef __AKARI_TASK_SUBSYSTEM_HPP__
#define __AKARI_TASK_SUBSYSTEM_HPP__

#include <AkariSubsystem.hpp>
#include <AkariMemorySubsystem.hpp>
#include <interrupts.hpp>
#include <HashTable.hpp>
#include <Strings.hpp>
#include <List.hpp>

// user task kernel stack is used for state when it's
// pre-empted, and for system calls, etc.
#define USER_TASK_KERNEL_STACK_SIZE	0x2000
#define USER_TASK_STACK_SIZE	0x4000

class AkariTaskSubsystem : public AkariSubsystem {
	public:
		AkariTaskSubsystem();

		u8 versionMajor() const;
		u8 versionMinor() const;
		const char *versionManufacturer() const;
		const char *versionProduct() const;

		static void SwitchRing(u8 cpl, u8 iopl);

		class Task {
			public:
				static Task *BootstrapInitialTask(u8 cpl, AkariMemorySubsystem::PageDirectory *pageDirBase);
				static Task *CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, AkariMemorySubsystem::PageDirectory *pageDirBase);

				bool getIOMap(u8 port) const;
				void setIOMap(u8 port, bool enabled);

				// Task linked list.
				Task *next, *priorityNext;

				// IRQ listening controls.
				bool irqWaiting;
				u32 irqListen, irqListenHits;

				// GUID and other identifying information.
				u32 id;
				ASCIIString registeredName;
				
				// Real task process data.
				u8 cpl;
				AkariMemorySubsystem::PageDirectory *pageDir;
				u32 ks; u32 utks;
				u8 iomap[32];

				class Node {
					public:
						Node();

						void registerListener();

					protected:
						LinkedList<u32> _listeners;
				};

				HashTable<ASCIIString, Node *> *nodesByName;

			protected:
				Task(u8 cpl);

		};

		Task *getNextTask();
		void cycleTask();
		void saveRegisterToTask(Task *dest, void *regs);
		void *assignInternalTask(Task *task);

		Task *start, *current;
		Task *priorityStart;
		HashTable<ASCIIString, Task *> *registeredTasks;
};

#endif

