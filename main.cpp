#include "message.h"
#include <iostream>
#include <sstream>
#include "crbUnit.h"
#include <queue>
#include <stdlib.h>

using namespace std;

static bool verifyMessageState(const message &m, const string &expected);
static string randomString();
static string randomString(int len);
static bool testConstructor1(void *ignoreMe);
static bool testConstructor2(void *ignoreMe);
static bool testStringConstructor(void *ignore);
static bool testAppend(void *arg);
static bool testPrepend(void *arg);
static bool testFrame(void *arg);
static bool testCopyConstructor(void *arg);
static bool testSplit(void *arg);
static bool testSplitInt(void *arg);
static void doEmptyTests(testStats &ts);
static string appendInt(string s, int i);
static void doNonEmptyTests(testStats &ts);
static void doAllocFails(testStats &ts);
static bool testOperatorPlus(void *args);
static bool testOperatorPlusEquals(void *args);
static string formatInt(int n);
bool testAllocConstructor(void *ignoreMe);
bool testAllocCopyConstructor(void *newMsg);
bool testAllocStringConstructor(void *str);

int main() {
	testStats ts = setup();
	doEmptyTests(ts);
	doNonEmptyTests(ts);

	doAllocFails(ts);
	summary(ts);	
	return 0;
}


static bool verifyMessageState(const message &m, const string &expected) {
	string actual = m.toString();

	if (actual != expected) {
		throw(formatError("verifyMessage", "wrong string", expected, actual));
	}else if (m.length() != expected.length()) {
		throw(formatError("verifyMessage", "message::ttlLen", formatInt(expected.length()), formatInt(m.length()))); 
	}
	return true;
}

static string randomString() {
	return randomString(rand() % 80 + 1);
}

static string randomString(int len) {
	string s;

	for (int i = 0; i < len; i++) {
		char c = rand() % 26 + 'a';
		s += c;
	}
	return s;
}

static bool testConstructor1(void *ignoreMe) {
	message m;
	return verifyMessageState(m, string(""));
}

static bool testConstructor2(void *ignoreMe) {
	message n, m;
	return !m.split(n) && !m.split(10, n) && !m.split(0, n);
}

static bool testStringConstructor(void *ignore) {
	string s = randomString();
	message m(s);
	
	return verifyMessageState(m, string(s));
}

static bool testAppend(void *arg) {
	message *m = (message *) arg;
	string appendMe = randomString();
	string s = m->toString();

	return m->append(appendMe) && verifyMessageState(*m, s+appendMe);
}	

static bool testPrepend(void *arg) {
	message *m = (message *) arg;
	string prependMe = randomString();
	string s = m->toString();

	return m->prepend(prependMe) && verifyMessageState(*m, prependMe + s);
}	

static bool testFrame(void *arg) {
	message *m = (message *) arg;
	string header = randomString();
	string footer = randomString();
	string s = m->toString();

	s = header + s + footer;
	return m->frame(header, footer) && verifyMessageState(*m, s);
}	

static bool testCopyConstructor(void *arg) {
	message *m = (message *) arg;
	string s = m->toString();

	message n(*m);
	return verifyMessageState(n, s);
}

static bool testSplit(void *arg) {
	message *m = (message *) arg;
	string s, s1, s2, pref, suf;
	message n;	

	//make sure these are the same length
	pref = randomString(10);
	suf = randomString(10);

	s = m->toString(s);
	s1 = s.substr(0, s.length()/2);
	s2 = s.substr(s.length()/2);
		
	if (!m->frame(pref, suf)) {
		throw(formatError("testSplit", "frame cowardly refused"));
	}else if (!m->split(n)) {
		throw(formatError("testSplit", "testSplit(int) cowardly refused"));
	}
	pref = pref + s1;
	suf = s2 + suf;
	try {
		return verifyMessageState(n, pref) && verifyMessageState(*m, suf);
	}catch (string s) {
		throw(formatError("testSplit", s));
	}
}

static bool testSplitInt(void *arg) {
	message *m = (message *) arg;
	string s;

	// force an odd length.
	if (m->length() % 2 == 0 && !m->append("a")) {
		throw(formatError("testSplitInt", "append cowardly refused"));
	}

	for (int i = 0; i < 5; i++) {
		if (!m->frame(randomString(1), randomString(1))) {
			throw(formatError("testSplitInt", "frame failed"));
		}
	}

	s = m->toString();
	while (s.length() >= 1) {
		message n;
		string t;
		if (!m->split(2, n)) {
			throw(formatError("testSplitInt", "split(2,n) failed"));
		}
		try {
			if (s.length() == 1) {
				return (verifyMessageState(n, s) && verifyMessageState(*m, string("")));
			}else if (!verifyMessageState(n, s.substr(0, 2)) || !verifyMessageState(*m, s.substr(2))) {
				return false;
			}
		}catch(string ex) {
			throw(formatError("testSplitInt", ex));
		}
		s = s.substr(2);
	}
	try {
		return verifyMessageState(*m, s);
	}catch(string ex) {
		throw(formatError("testSplitInt", ex));
	}
}

static void doEmptyTests(testStats &ts) {
	message *m;

	test("message(), length, toString", ts, testConstructor1, NULL);	
	test("message(), split(message), split(int, message)", ts, testConstructor2, NULL);
	m = new message;
	test("copy constructor (empty)", ts, testCopyConstructor, m);
	delete m;
	test("string constructor", ts, testStringConstructor, NULL);
	m = new message();
	test("append to empty message", ts, testAppend, m);
	delete m;
	m = new message();
	test("prepend to empty message", ts, testPrepend, m);
	delete m;
	m = new message();
	test("frame an empty message", ts, testFrame, m);
	delete m;
}	

static void doAllocFails(testStats &ts) {
	//was intending to test exception handling when out of memory but am having threading issues
	// TODO fix crbUnit to handle these properly
	return;
	// CRBKLUDGE

	message *m;
	m = new message;

	vector<void*> *p = hogHeap();
	test("message() when out of memory", ts, testAllocConstructor, NULL);	
	test("copy constructor when out of memory", ts, testAllocCopyConstructor, m);
	test("string constructor when out of memory", ts, testAllocStringConstructor, NULL);
	freeHeap(p);
}	

static string appendInt(string s, int i) {
	ostringstream os;
	os << s << ": " << i;
	return os.str();
}
	
static void doNonEmptyTests(testStats &ts) {
	message m[2];
	
	test("append 0", ts, testAppend, &m[1]);
	for (int i = 0; i < 3; i++) {
		test(appendInt("append", i), ts, testAppend, &m[0]);
		test(appendInt("prepend", i), ts, testPrepend, &m[0]);
		test(appendInt("frame", i), ts, testFrame, &m[0]);
		test(appendInt("split(half)", i), ts, testSplit, &m[0]);
		test("operator+", ts, testOperatorPlus, m);
		test("operator+=", ts, testOperatorPlusEquals, m);	
	}
	test("split(int)", ts, testSplitInt, &m[0]);
}

static bool testOperatorPlus(void *args) {
	message *arr = (message *) args;
	message *m1 = &arr[0], *m2 = &arr[1];
	string s1 = m1->toString(), s2 = m2->toString();
	message m = *m1 + *m2;

	if (m.toString() != s1 + s2) {
		throw(formatError("testOperatorPlus", "Wrong Result", s1+s2, m.toString()));
	}else if (m1->toString() != s1 || m2->toString() != s2) {
		throw(formatError("testOperatorPlus", "Operand modified!"));
	}
	return true;
}	

static bool testOperatorPlusEquals(void *args) {
	message *arr = (message *) args;
	message m1 = arr[0], m2 = arr[1];
	string s1 = m1.toString(), s2 = m2.toString();
	m1 += m2;
	if (m1.toString() != s1 + s2) {
		throw(formatError("testOperatorPlusEquals", "Wrong result", s1+s2, m1.toString()));
	}else if (m2.toString() != s2) {
		throw(formatError("testOperatorPlusEquals", "operand modified"));
	}
	return true;
}

static string formatInt(int n) {
	ostringstream oss;
	oss << n;
	return oss.str();
}	

bool testAllocConstructor(void *ignoreMe) {
	message *m;
	try {
		while ((m = new message())) {
			//WHEEE!
		}		
	}catch (bad_alloc ex) {
		return true;
	}
	return false;
}

bool testAllocCopyConstructor(void *newMsg) {
	message *m, *p = (message*)newMsg;
	try {
		while((m = new message(*p))) {
			// WHEE!
		}
	}catch (bad_alloc ex) {
		return true;
	}
	return false;
}

bool testAllocStringConstructor(void *str) {
	string *s = (string *)str;
	message *m;
	try {
		while ((m = new message(*s))) {
			//WHEEE!
		}
	}catch (bad_alloc ex) {
		return true;
	}
	return false;
}

