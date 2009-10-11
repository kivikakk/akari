#ifndef __TASKS_HPP__
#define __TASKS_HPP__

#include <Subsystem.hpp>
#include <Memory.hpp>
#include <interrupts.hpp>
#include <HashTable.hpp>
#include <Strings.hpp>
#include <List.hpp>
#include <Symbol.hpp>
#include <BlockingCall.hpp>

// user task kernel stack is used for state when it's
// pre-empted, and for system calls, etc.
#define USER_TASK_KERNEL_STACK_SIZE	0x2000
#define USER_TASK_STACK_SIZE		0x4000

#define USER_TASK_BASE				0x50000000

class Tasks : public Subsystem {
public:
	Tasks();

	u8 versionMajor() const;
	u8 versionMinor() const;
	const char *versionManufacturer() const;
	const char *versionProduct() const;

	static void SwitchRing(u8 cpl, u8 iopl);

	class Task {
	public:
		class Node {
		public:
			Node();
			class Listener;

			u32 registerWriter(bool exclusive);
			u32 registerListener();
			Listener &getListener(u32 id);
			void writeAllListeners(const char *buffer, u32 n);

			bool hasWriter(u32 id) const;
			bool hasListener(u32 id) const;

			class Listener {
			friend class Node;
			public:
				Listener(u32 id);

				void append(const char *data, u32 n);
				void reset();
				void cut(u32 n);
				void hook(Task *task);
				void unhook();

				const char *view() const;
				u32 length() const;

			protected:
				u32 _id;
				char *_buffer;
				u32 _buflen;
				Task *_hooked;
			};

		protected:
			bool _exclusive;
			LinkedList<u32> _writers;
			LinkedList<Listener> _listeners;

			u32 _wl_id;
			u32 _nextId();
		};

		static Task *BootstrapInitialTask(u8 cpl, Memory::PageDirectory *pageDirBase);
		static Task *CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, Memory::PageDirectory *pageDirBase);

		bool getIOMap(u8 port) const;
		void setIOMap(u8 port, bool enabled);

		// Task linked list.
		Task *next, *priorityNext;

		// IRQ listening controls.
		bool irqWaiting;
		u32 irqListen, irqListenHits;

		// User call blocking. (HACKy?)
		bool userWaiting;
		BlockingCall *userCall;

		// GUID and other identifying information.
		u32 id;
		Symbol registeredName;
		
		// Real task process data.
		u8 cpl;
		Memory::PageDirectory *pageDir;
		Memory::Heap *heap;
		u32 heapStart, heapEnd, heapMax;
		u32 ks;
		u8 iomap[32];

		HashTable<Symbol, Node *> *nodesByName;

	protected:
		Task(u8 cpl);

	};

	Task *getNextTask();
	void cycleTask();
	void saveRegisterToTask(Task *dest, void *regs);
	void *assignInternalTask(Task *task);

	Task *start, *current;
	Task *priorityStart;
	HashTable<Symbol, Task *> *registeredTasks;
};

#endif

