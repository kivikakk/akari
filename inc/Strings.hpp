#ifndef __STRINGS_HPP__
#define __STRINGS_HPP__

#include <arch.hpp>

class ASCIIString {
	public:
		ASCIIString();
		ASCIIString(const ASCIIString &);
		ASCIIString(const char *&);
		
		ASCIIString &operator =(const ASCIIString &);
		ASCIIString &operator =(const char *&);

		bool operator ==(const ASCIIString &) const;
		bool operator ==(const char *) const;

		bool operator !() const;

		char operator[](u32 n) const;
		char &operator[](u32 n);

		bool empty() const;
		u32 length() const;
		const char *getCString() const;

	protected:
		char *_data;
		u32 _dataLength;
};

#endif

