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

#include <Memory.hpp>
#include <interrupts.hpp>
#include <map>
#include <string>
#include <list>
#include <Symbol.hpp>
#include <BlockingCall.hpp>
#include <UserIPCQueue.hpp>
#include <UserPrivs.hpp>

// user task kernel stack is used for state when it's
// pre-empted, and for system calls, etc.
#define USER_TASK_KERNEL_STACK_SIZE	0x2000
#define USER_TASK_STACK_SIZE		0x4000

// These can't be too low - otherwise the kernel may have
// expanded into this area by the time a process is created.
#define PROCESS_HEAP_START			0x20000000
#define PROCESS_HEAP_SIZE			0x300000		// 3MiB
#define PROC_HEAP_SIZE 				0x10000

#define USER_TASK_BASE				0x50000000

class Tasks {
public:
	Tasks();

	explicit Tasks(const Tasks &);
	Tasks &operator =(const Tasks &);

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
			std::list<u32> _writers;
			std::list<Listener> _listeners;

			u32 _wl_id;
			u32 _nextId();
		};

		class Queue {
		public:
			class Item {
			public:
				Item(u32, u32, pid_t, u32, const void *, u32);
				~Item();

				explicit Item(const Item &r);
				Item &operator =(const Item &r);

				struct queue_item_info info;
				char *data;
			};

			Queue();

			u32 push_back(pid_t from, u32 reply_to, const void *data, u32 data_len);
			void shift();
			void remove(Item *item);
			Item *first();
			Item *itemByReplyTo(u32 reply_to);
			Item *itemById(u32 id);

		protected:
			std::list<Item *> list;
		};

		explicit Task(const Task &);
		Task &operator =(const Task &);

		static Task *BootstrapInitialTask(u8 cpl, Memory::PageDirectory *pageDirBase);
		static Task *CreateTask(u32 entry, u8 cpl, bool interruptFlag, u8 iopl, Memory::PageDirectory *pageDirBase, const char *name, const std::list<std::string> &args);

		bool getIOMap(u16 port) const;
		void setIOMap(u16 port, bool enabled);

		void unblockType(const Symbol &type);
		void unblockTypeWith(const Symbol &type, u32 data);

		void grantPrivilege(priv_t priv);
		bool hasPrivilege(priv_t priv) const;

		u8 *dumpELFCore(u32 *size) const;

		// Task linked list.
		Task *next, *priorityNext;

		// IRQ listening controls.
		u32 irqListen, irqListenHits;

		// User call blocking. (HACKy?)
		bool userWaiting;
		BlockingCall *userCall;

		// GUID and other identifying information.
		pid_t id;
		std::string name;
		Symbol registeredName;
		
		// Real task process data.
		u8 cpl;
		Memory::PageDirectory *pageDir;
		Heap *heap;
		u32 heapStart, heapEnd, heapMax;
		u32 ks;
		u8 iomap[8192];
		u8 *privMap;

		std::map<Symbol, Stream *> *streamsByName;
		Queue *replyQueue;

	protected:
		Task(u8 cpl, const std::string &name);
		~Task();
	};

	Task *getTaskById(pid_t id) const;

	Task *prepareFetchNextTask();
	void cycleTask();
	void saveRegisterToTask(Task *dest, void *regs);
	void *assignInternalTask(Task *task);

	Task *start, *current;
	Task *priorityStart;
	std::map<Symbol, Task *> registeredTasks;
	std::map<pid_t, Task *> tasksByPid;
};

#endif

