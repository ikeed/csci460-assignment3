#include "message.h"
#include <string>
#include <string.h>
#include <deque>
#include <iostream>
#include <stdlib.h>
using namespace std;

//#define _SHENANIGANS_

//helper function.  splitMe is the existing segment which we need to split.
// seg is a new object the caller has just created.  
// splitPoint is the number of characters seg should own when we're done.
// This will associate seg with the same string (index) in the buffer as splitMe
// but seg.buf will point to the start of the string whereas splitMe.buf will point to
// the 'splitPoint'th character of the string.  The .len fields of both segment objects
// will be updated so both objects know how much of the string is theirs.
static segment &splitBuf(segment &seg, segment &splitMe, int splitPoint);

static void shenanigans(string &s) {
	#ifdef _SHENANIGANS_
	int x = 1, y = 1, z;
	char *p = NULL;
	srand(time(NULL));
	switch(rand() % 17) {
		case 0: z = 1/(x-y); z++; break;//floating point exception
		case 1: while(1) {}  //infinite loop
		case 2: case 3: s = "foo_" + s + "_garply"; break;// wrong behaviour
		case 4: cout << p + 1000 << endl; break;  //segmentation fault
		default: break;
	}
	#endif
}



segment message::addToBuffer(string s, segment seg) {
	shenanigans(s);
	seg.len = s.length();
	seg.buf = (char*) s.c_str();
	if (buffer.empty()) {
		seg.index = 0;
	}else {
		seg.index = buffer.size();
	}
	buffer.push_back(s);

	return seg;
}

void message::clear() {
	ttlLen = 0;
	segments.clear();
	buffer.clear();
}

message::message() {
	this->clear();
}

message::message(string start)  {
	this->clear();
	if (!append(start)) {
		throw bad_alloc();
	}
}

message::message(const message &m) {
	*this = m;
}

bool message::split(message &m) {
// split() splits the message exactly in half and 
// initializes m with the FIRST half of the message.  
// The remainder still resides in this instance.
// returns: The same as split(int, message) below.
	return split(ttlLen/2, m);
}
		


bool message::split(size_t chunkSize, message &m) {
// split(int) will split off the first 'chunkSize' bytes of the message
// and initialize m with it.  The remainder of the message will still reside in 'this' instance.
// returns true on success, false on failure (no data)
	m.buffer = buffer;  // copy all of them over for now. //TODO: Um, don't?
	// note:  This will duplicate the buffer object which is a deque<string> 
	// and will duplicate all of the string objects in it BUT the string::c_str() (char *) 
	// pointers will all still be pointing to the same place.  (I tested this) So no duplication 
	// of the underlying buffer is performed.  Just tossing pointers around.
	if (ttlLen <= 0 || buffer.empty()) {
		// no data?
		return false;
	} else if (ttlLen <= chunkSize) {
		// they want the whole thing!

		m = *this; // copy ourself.
		// now empty ourself out
		ttlLen = 0;
		buffer.clear();
		segments.clear();
		return true;
	}
		
	while (!segments.empty() && (m.ttlLen + segments.front().len) < chunkSize) {
		segment seg = segments.front();
		segments.pop_front();
		m.segments.push_back(seg);
		ttlLen -= seg.len;
		m.ttlLen += seg.len;
	}

	// if we're here, then either this->segments is empty or the first segment needs to be split.
	if (!segments.empty() && m.ttlLen < chunkSize) {
		// first segment needs to be split.
		int shortfall = chunkSize - m.ttlLen;
		segment seg;
		splitBuf(seg, (segment&) segments.front(), shortfall);
		m.ttlLen = chunkSize;
		ttlLen -= shortfall;
		m.segments.push_back(seg);		
	}
	// looks like we survived all that!
	return true;

}

bool message::addSegment(string s, bool toFront) {
	segment seg;

	shenanigans(s);
	// calling the string(char *) constructor to make sure we COPY s.
	// strings shallow-copy by default.	
	seg = addToBuffer(string(s.c_str()), seg);
	if (toFront) {
		segments.push_front(seg);
	}else {
		segments.push_back(seg);
	}
	ttlLen += seg.len;
	return true;
}

bool message::prepend(string s) {
	return addSegment(s, true);
}

bool message::append(string s) {
	return addSegment(s, false);
}

string &message::toString(string &s) const {
	s = toString();
	return s;
}

string message::toString() const {
	string s;

	if (segments.empty()) {
		return s;
	}
	deque<segment>::const_iterator i;
	for(i = segments.begin(); i != segments.end(); i++) {
		s += string(i->buf).substr(0, i->len);
	}
	shenanigans(s);
	return s;
}


size_t message::length() const {
	return ttlLen;
}



// Static helper functions lurk here.

static segment &splitBuf(segment &seg, segment &splitMe, int splitPoint) {
	seg.buf = splitMe.buf;
	seg.index = splitMe.index;
	seg.len = splitPoint;
	splitMe.buf += splitPoint;
	splitMe.len -= splitPoint;
	return seg;
}
	
message &message::operator=(const message &m) {

	this->clear();
	// m may have been built up out of a million chunks.
	// get them all concatenated an add them to 'this' as
	// a single string for simplicity.
	if (!this->append(m.toString())) {
		// TODO: Seriously.  Exceptions.  yeah.
	}
	return *this;
}

const message message::operator+(const message &m1) const {
	message m(*this);
	return m+=m1;
}

message &message::concatenate(message &mTo, const message &mFrom) {
	deque<segment>::const_iterator i;
	segment s;
	for(i = mFrom.segments.begin(); i != mFrom.segments.end(); i++) {
		s = *i; //copy
		mTo.buffer.push_back(mFrom.buffer[s.index]);
		s.index = mTo.buffer.size();
		mTo.segments.push_back(s);
		mTo.ttlLen+=s.len;
	}
	return mTo;
}


message &message::operator+=(const message &m1) {
	return concatenate(*this, m1);
}

