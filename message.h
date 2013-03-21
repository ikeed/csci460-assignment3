#ifndef _USING_CRB_MESSAGE_H_
#define _USING_CRB_MESSAGE_H_
#include <string>
#include <deque>

using namespace std;

struct segment {
	char *buf;
	// Decided on a length member because we might want 
	// multiple (not-null-terminated) pointers into a 
	// single buffer.  This way the user is unrestricted.
	int len;
	int index;
};

class message {
	private:

	// When a string is added to this message, we'll store it in this deque
	deque<string> buffer;

	//This deque stores segment objects so I can point to the middle of strings.
	deque<segment> segments;

	// The total length of all segments contained in this object 
	// if concatenated together.
	size_t ttlLen;

	// private methods to help with adds
	segment addToBuffer(string s, segment seg);
	bool addSegment(string s, bool toFront);
	message &concatenate(message &mTo, const message &mFrom);

	public:

	// these constructors throw bad_alloc exceptions if we run out of memory
	// note that test coverage is only 99% because I'm currently not testing 
	// that exception handling.  It has been tested but my crbUnit lib has a bug
	// when out of memory so I have disabled that test at at the moment.
	message();
	message(string start);
	message(const message &m);

	// You only need to call this if you want to blank a non-empty message
	void clear();

	bool split(message &m); 
	// split(m) splits the message exactly in half and 
	// initializes m with the FIRST half of the message.  
	// The remainder still resides in this instance.
	// returns: The same as split(int, message) below.

	bool split(size_t chunkSize, message &m);
	// split(int) will split off the first 'chunkSize' bytes of the message
	// and initialize m with it.  The remainder of the message will still
	// reside in 'this' instance.  If there are fewer than chunkSize chars in 'this' message
	// m will contain the entire contents and the current message will become empty.
	// returns true on success, false on failure (no data)
	// I did it this way so that message objects could behave like a queue of chunks. e.g.:
	// message m;
	// while (myMessage.length()) {
	//	if (myMessage.split(10, m)) {
	//		cout << m.toString();
	//	}
	// }
	//	
		
	
	// these will push s to the front or back of the message.  Returns true if-f successful.
	bool prepend(string s);
	bool append(string s);
	
	// shortcut for prepending and appending at the same time.
	bool frame(string prefix, string suffix) {return prepend(prefix) && append(suffix);}

	// convert the current message to a string.  Returns the string by reference and 
	// by return value so it can be used in operator chaining.
	string &toString(string &s) const;
	string toString() const;
	size_t length() const;

	message &operator=(const message &m);
	const message operator+(const message &m1) const;
	message &operator+=(const message &m1);
};

#endif



