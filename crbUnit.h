#ifndef _USING_CRB_UNIT_H_
#define _USING_CRB_UNIT_H_
#include <string>
#include <vector>

using std::string;

// simple struct to store pass/fail info.
struct testStats {
	int nTests;
	int nFails;
	int nCrashes;
};

/* You must call this before calling test.  This will set up your
 * testStats object and also display a header on stdout,
 * under which all other test results will be shown.
 */ 
testStats setup();

/* You write a test function like this:
bool myTestFunction(void *args) {
	MyObjectType *x = (MyObjectType*) args;  
	// Your test function needs to know how to unpack the arguments.
	// Can be any type(s) you like.  You will pass these in when you call test(..)
	// later.  
	if (!x->SomeFunction()) {
		// You can throw a std::string exception in your test function to give an explicit error message.
		// Or simply return false which will be shown as [FAIL] and won't have a detailed error message.
		throw(formatError("myTestFunction", "MyObjectType::SomeFunction", "Should have Returned True"));
	}
	return true;  // test passed
}

//Then somewhere in your code when you want to run that test you go:
MyObjectType *p = new MyObjectType;
test("Testing SomeFunction", ts, myTestFunction, (void *)p);
// that will run your test function in a new thread.

// You don't have to supply arguments to your test function if you don't want.
*/	
bool test(string pre, testStats &ts, bool (*test)(void*), void *args);

// After you have finished calling test(..) with each of your test functions and various arguments, 
// you call this to display a summary of results.
void summary(testStats ts);

/* These are just to keep the formatting consistent.  It's not strictly necessary but if you're
 * going to throw std::string exceptions, this will make them pretty.
 * e.g. throw(formatError("TestFunction1", "Didn't Properly clear the array"));
 * or   throw(formatError("TestFunction2", "Returned Wrong Element!", 2, result));
 */ 
string formatError(string who, string what);
string formatError(string who, string what, string expected, string actual);

/* This is for causing out-of memory situations for testing your code which handles 
 * allocation failures.  you call this first:
 * std::vector<void *> bloaty = hogHeap();  // This will eat up all available memory.
 * // Then you do whatever tests you're going to do under those conditions:
   test("heap::insert when out of memory", ts, MyFunction_StarvingInsert, (void *) MyObject);
   test(..);
   ...
   // Then you free up all of that memory:
   freeHeap(bloaty);
   // Wheww!!
 */
std::vector<void*> *hogHeap();
void freeHeap(std::vector<void*> *hp);


#endif
