#ifndef __SYMBOL_HPP__
#define __SYMBOL_HPP__

class Symbol {
	public:
		Symbol();
		Symbol(const char *content);

		bool operator !() const;
		bool operator ==(const Symbol &) const;
		bool operator !=(const Symbol &) const;

	protected:
		const char *_content;
};

#endif

