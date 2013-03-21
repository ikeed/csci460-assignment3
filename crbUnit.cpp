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

#define MAX_SECONDS 5

#define FIRST_COL_WIDTH 50

using namespace std;

static bool postMortem(int status, testStats &ts);
static void caughtException(string str);

#ifndef _ONE_THREAD_
static void killChild(pid_t pid);
#endif

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

bool childExited(pid_t pid, int &status, int maxSeconds) {
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


bool test(string testName, testStats &ts, bool (*test)(void*), void *args) {
	int status;
	pid_t pid = 0;

	ts.nTests++;
	cout << setw(FIRST_COL_WIDTH) << testName << "\t" << flush;
	#ifdef _ONE_THREAD_
	if (0) {
	#else
	if ((pid = fork())) {
	#endif
		// parent
		if (childExited(pid, status, MAX_SECONDS)) {
			return postMortem(status, ts);
		}else {
			// TODO: Have a seperate count for hung loops?
			ts.nCrashes++;
			ts.nFails++;
			cout << "[HUNG]" << endl;
			#ifndef _ONE_THREAD_
				killChild(pid);
			#endif
		}
		return false;
	}else {		
		// child
		try {
			if (!test(args)) {
				#ifndef _ONE_THREAD_ 
					exit(1); //failed
				#endif
			}else { 
				#ifndef _ONE_THREAD_
					exit(0); //passed
				#endif
			}
		}catch(std::exception ex) {
			caughtException(ex.what());
			exit(2);
		}catch (std::string ex) {
			caughtException(ex);
			exit(2);
		}
	}
	return true;
}

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
	if (WIFEXITED(status)) {
		switch(WEXITSTATUS(status)) {
			case 1: 
				cout << "[FAILED]" << endl;
				ts.nFails++;
				return false;
			case 0:
				cout << "[PASSED]" << endl;
				return true;
			case 2:
				//child process caught exception and did output
				ts.nFails++;
				return false;
			default:
				ts.nFails++;
				cout << "[UNKNOWN EXIT STATUS]" << endl;
				return false;
			}
	} else if (WIFSIGNALED(status)) {
		cout << "[CRASHED (" << strsignal(WTERMSIG(status)) << ")]" << endl;
		ts.nCrashes++;
		ts.nFails++;
		return false;
	} else if (WIFSTOPPED(status)) {
		cout << "[STOPPED (" << strsignal(WSTOPSIG(status)) << ")]" << endl;
		ts.nCrashes++;
		ts.nFails++;
		return false;
	}
	return true;
}

#ifndef _ONE_THREAD_
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
#endif

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

