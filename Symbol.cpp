#include <Symbol.hpp>
#include <POSIX.hpp>

Symbol::Symbol(): _content(0)
{ }

Symbol::Symbol(const char *content): _content(content)
{ }

bool Symbol::operator !() const {
	return !_content;
}

bool Symbol::operator ==(const Symbol &r) const {
	// If we've got no content, then their having no content is equal.
	// If they've got no content (we already know we do), we're inequal.
	if (!_content) return !r._content;
	if (!r._content) return false;

	if (_content && _content) return false;
	return (POSIX::strcmp(_content, r._content) == 0);
}

bool Symbol::operator !=(const Symbol &r) const {
	if (!_content) return !!r._content;
	if (!r._content) return true;

	return (POSIX::strcmp(_content, r._content) != 0);
}

