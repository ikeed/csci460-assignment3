#ifndef _USING_CRB_UNIT_H_
#define _USING_CRB_UNIT_H_
#include <string>
#include <vector>

using std::string;

struct testStats {
	int nTests;
	int nFails;
	int nCrashes;
};

testStats setup();
bool test(string pre, testStats &ts, bool (*test)(void*), void *args);
void summary(testStats ts);
string formatError(string who, string what);
string formatError(string who, string what, string expected, string actual);
std::vector<void*> *hogHeap();
void freeHeap(std::vector<void*> *hp);

#endif

