#include <Strings.hpp>
#include <POSIX.hpp>
#include <debug.hpp>

ASCIIString::ASCIIString(): _data(0), _dataLength(0) { }

ASCIIString::ASCIIString(const ASCIIString &src) {
	operator =(src);
}

ASCIIString::ASCIIString(const char *&src) {
	operator =(src);
}

ASCIIString &ASCIIString::operator =(const ASCIIString &src) {
	if (_data)
		delete [] _data;

	_dataLength = src._dataLength;

	if (_dataLength == 0) {
		_data = 0;
		return *this;
	}

	_data = new char[_dataLength + 1];
	POSIX::memcpy(_data, src._data, _dataLength + 1);

	return *this;
}

ASCIIString &ASCIIString::operator =(const char *&src) {
	if (_data)
		delete [] _data;

	if (!src) {
		_dataLength = 0;
		_data = 0;
		return *this;
	}

	_dataLength = POSIX::strlen(src);

	if (_dataLength == 0) {
		_data = 0;
		return *this;
	}

	_data = new char[_dataLength + 1];
	POSIX::memcpy(_data, src, _dataLength + 1);

	return *this;
}

bool ASCIIString::operator ==(const ASCIIString &r) const {
	if (_dataLength != r._dataLength)
		return false;
	
	return operator ==(r._data);
}

bool ASCIIString::operator ==(const char *r) const {
	const char *l = _data;

	while (*l && *r) {
		if (*l != *r)
			return false;
		++l, ++r;
	}

	if (*l != *r)
		return false;
	
	return true;
}

char ASCIIString::operator[](u32 n) const {
	ASSERT(n < _dataLength);
	return _data[n];
}

char &ASCIIString::operator[](u32 n) {
	ASSERT(n < _dataLength);
	return _data[n];
}

bool ASCIIString::empty() const {
	return (_dataLength == 0);
}

u32 ASCIIString::length() const {
	return _dataLength;
}

const char *ASCIIString::GetCString() const {
	static char *buffer = 0;
	static u32 bufferLength = 0;	// includes NUL allocated

	if (!buffer || bufferLength < (_dataLength + 1)) {
		if (buffer)
			delete [] buffer;

		buffer = new char[_dataLength + 1];
		bufferLength = _dataLength + 1;
	}

	if (_dataLength == 0) {
		*buffer = 0;
	} else {
		POSIX::memcpy(buffer, _data, _dataLength + 1);
	}

	return (const char *)buffer;
}

