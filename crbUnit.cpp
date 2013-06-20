#include "crbUnit.h"
#include <iostream>
#include <string>
#include <sstream>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>
#include <iomanip>
#include <stdarg.h>

#define MAX_SECONDS 5

#define FIRST_COL_WIDTH 50

using namespace std;


// The parent thread forks child threads to actually carry out the tests.  
// If the child doesn't crash, it will exit with one of these statuses.  Used by postMortem.
typedef enum {esTEST_FAILED, esTEST_PASSED, esEXCEPTION_CAUGHT} childExitStatus;

// This gets called by the parent thread to see what happened to the child thread.
// The testStats object will be updated to reflect the result of the last test. 
static bool postMortem(int status, testStats &ts);

// pretty trivial but I wanted it to look the same in all places.
static void caughtException(string str);

// This will encourage the child thread to no longer run in decreasingly
// polite ways as necessary.
static void killChild(pid_t pid);

/* if function is called to see IF the child thread has exited (we return true) and 
 * will wait maxSeconds seconds for it to do so.  If not, it simply returns false.
 */
static bool hasChildExited(pid_t pid, int &status, int maxSeconds);


/* This *must* be called first by your test code to create and initialize an object 
 * to hold your results.  That object is then passed in to the test(..) function 
 * with each test.  This also prints a header under which test results will be displayed.
 */
testStats setup() {
	testStats ts;
	ts.nTests = ts.nFails = ts.nCrashes = 0;
	system("clear");
	cout << setw(FIRST_COL_WIDTH) << "Test Name:";
	cout << "\tResult:" << endl;
	for (int i = 0; i < FIRST_COL_WIDTH+15; i++) {
		cout << '-';
	}
	cout << endl;
	return ts;
}

/* Your test module calls this to carry out one unit test.  (*test) is the function you wrote.
 * args is whatever arguments your test function needs (this allows you to do stateful testing).
 * Your test function needs to undertand how to unpack args.
 */
bool test(string testName, testStats &ts, bool (*test)(void*), void *args) {
	int status;
	bool oneThread = false;
	pid_t pid = 0;

	#ifdef _ONE_THREAD_
		oneThread = true;
	#endif

	ts.nTests++;
	cout << setw(FIRST_COL_WIDTH) << testName << "\t" << flush;
	if (!oneThread && (pid = fork())) {
		// parent.  Give the child a chance to work.
		if (hasChildExited(pid, status, MAX_SECONDS)) {
			// Child exited.  Find out why.
			return postMortem(status, ts);
		}else {
			// TODO: Have a seperate count for hung loops?
			ts.nCrashes++;
			ts.nFails++;
			cout << "[HUNG]" << endl;
			if (!oneThread) {
				killChild(pid);
			}
		}
		return false;
	}else {		
		// child
		try {
			// We want to run the test whether we are in single-threaded mode or not.
			if (!test(args)) {
				// but don't exit if we're in single-threaded mode!
				if (!oneThread) {
					exit(esTEST_FAILED);
				}
			}else { 
				// but don't exit if we're in single-threaded mode!
				if (!oneThread) {
					exit(esTEST_PASSED);
				}
			}
		}catch(std::exception ex) {
			caughtException(ex.what());
			exit(esEXCEPTION_CAUGHT);
		}catch (std::string ex) {
			caughtException(ex);
			exit(esEXCEPTION_CAUGHT);
		}
	}
	return true;
}


// This should be called at the end of your test module do display overall results.
void summary(testStats ts) {
	string line;
	ostringstream oss;
	oss << "*    Tests Passed: "
		<< ts.nTests - ts.nFails
		<< "/" << ts.nTests 
		<< " (" << setprecision(2) << 100 * (ts.nTests - ts.nFails)/ts.nTests << "%)"
		<< "    [" << ts.nCrashes 
		<< " crashes]    *";
	for (size_t i = 0; i < oss.str().length(); i++) {
		line += string("*");
	}
	cout << line << endl << oss.str() << endl << line << endl;
}

static bool postMortem(int status, testStats &ts) {
	if (WIFEXITED(status)) {  // exited.  what was its status?
		switch(WEXITSTATUS(status)) {
			case esTEST_FAILED: 
				cout << "[FAILED]" << endl;
				ts.nFails++;
				return false;
			case esTEST_PASSED:
				cout << "[PASSED]" << endl;
				return true;
			case esEXCEPTION_CAUGHT:
				//child process caught exception and did output already
				ts.nFails++;
				return false;
			default:  //buh?
				ts.nFails++;
				cout << "[UNKNOWN EXIT STATUS]" << endl;
				return false;
			}
	} else if (WIFSIGNALED(status)) {  // Child thread was killed (unhandled exception.  uh-oh!)
		cout << "[CRASHED (" << strsignal(WTERMSIG(status)) << ")]" << endl;
		ts.nCrashes++;
		ts.nFails++;
		return false;
	} else if (WIFSTOPPED(status)) {  // it was stopped for some other reason.
		cout << "[STOPPED (" << strsignal(WSTOPSIG(status)) << ")]" << endl;
		ts.nCrashes++;
		ts.nFails++;
		return false;
	}
	// survived all that?  Nothing to report, nothing to do.  Just return our joy.
	return true;
}

// stronger medicine until the child process is gone
static void killChild(pid_t pid) {
	int sigs[] = {SIGHUP, SIGINT, SIGTERM, SIGKILL};
	for (int i = 0; i < 4; i++) {
		kill(pid, sigs[i]);
		sleep(1);
		if (waitpid(pid, NULL, WNOHANG)) {
			return;
		}
	}	
}

static bool hasChildExited(pid_t pid, int &status, int maxSeconds) {
	for (int i = 0; i < maxSeconds; i++) {
		if (waitpid(pid, &status, WNOHANG | WUNTRACED)) {
			//child terminated
			return true;
		}else {
			sleep(1); //give it another second
		}
	}
	return false;
}	

static void caughtException(string str) {
	cout << "[FAILED (" << str << ")]" << endl;
}

string formatError(string who, string what) {
	ostringstream os;
	return who + string("::") + what;
}

string formatError(string who, string what, string expected, string actual) {
	return formatError(who, what) + "[expected: <" + expected + ">, actual: <" + actual + ">]";
}

vector<void*> *hogHeap() {
	vector<void*> *v = new vector<void*>;
	size_t sz = (size_t)(-1);
	void *p;

	while(1) {
		while((p = malloc(sz)) == NULL) {
			if ((sz/=2) == 0) {
				return v;
			}
		}
		try {
			v->push_back(v);
		}catch (bad_alloc ex) {
			if (p) {
				free(p);
			}
			return v;
		}
	}
	return v;		
}

void freeHeap(vector<void*> *hp) {
	vector<void*>::const_iterator i;

	for(i = hp->begin(); i != hp->end(); i++) {
		if (*i != NULL) {
			free(*i);
		}
	}
	delete hp;
}

