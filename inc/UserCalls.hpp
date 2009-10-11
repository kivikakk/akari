#ifndef __USER_CALLS_HPP__
#define __USER_CALLS_HPP__

#include <arch.hpp>
#include <Symbol.hpp>
#include <Tasks.hpp>

namespace User {
	void putc(char c);
	void puts(const char *s);
	void putl(u32 n, u8 base);
	u32 getProcessId();
	void irqWait();
	void irqListen(u32 irq);
	void panic(const char *s);
	bool registerName(const char *name);
	bool registerNode(const char *name);
	void exit();
	u32 obtainNodeWriter(const char *name, const char *node, bool exclusive);
	u32 obtainNodeListener(const char *name, const char *node);
	u32 readNode(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 readNodeUnblock(const char *name, const char *node, u32 listener, char *buffer, u32 n);
	u32 writeNode(const char *name, const char *node, u32 writer, const char *buffer, u32 n);
	void defer();

	class BlockingCall {
	public:
		BlockingCall();
		virtual ~BlockingCall();

		bool shallBlock() const;

		virtual u32 operator ()() = 0;
	
	protected:
		void _wontBlock();
		void _willBlock();
	
	private:
		bool _shallBlock;
	};

	class ReadBlockingCall : public BlockingCall {
	public:
		ReadBlockingCall(const char *name, const char *node, u32 listener, char *buffer, u32 n);

		Tasks::Task::Node::Listener *getListener() const;

		u32 operator ()();

	protected:
		Tasks::Task::Node::Listener *_listener;
		char *_buffer;
		u32 _n;
	};
}

#endif

