// This file is part of Akari.
// Copyright 2010 Arlen Cuss
//
// Akari is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Akari is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Akari.  If not, see <http://www.gnu.org/licenses/>.

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
#include <UserIPC.hpp>

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

		class Stream {
		public:
			Stream();
			class Listener;

			u32 registerWriter(bool exclusive);
			u32 registerListener();
			Listener &getListener(u32 id);
			void writeAllListeners(const char *buffer, u32 n);

			bool hasWriter(u32 id) const;
			bool hasListener(u32 id) const;

			class Listener {
			friend class Stream;
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

		class Queue {
		public:
			class Item {
			public:
				Item(u32, u32, u32, const void *, u32);
				~Item();

				struct queue_item_info info;
				char *data;
			};

			Queue();

			u32 push_back(u32 reply_to, const void *data, u32 data_len);
			void shift();
			Item *first();

		protected:
			LinkedList<Item *> list;
		};

		static Task *BootstrapInitialTask(u8 cpl, Memory::PageDirectory *pageDirBase);
		static Task *CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, Memory::PageDirectory *pageDirBase);

		bool getIOMap(u16 port) const;
		void setIOMap(u16 port, bool enabled);

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
		u8 iomap[8192];

		HashTable<Symbol, Stream *> *streamsByName;
		Queue *replyQueue;

	protected:
		Task(u8 cpl);
		~Task();
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

