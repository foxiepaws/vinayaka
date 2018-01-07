#include <cstdlib>
#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "picojson.h"
#include "distsn.h"


using namespace std;


int main (int argc, char *argv [])
{
	string host {argv [1]};
	string user {argv [2]};

	bool hit;
	string result = fetch_cache (host, user, hit);
	if (hit) {
		cout << "Access-Control-Allow-Origin: *" << endl;
		cout << "Content-Type: application/json" << endl << endl;
		cout << result;
	} else {
		pid_t pid = fork ();
		if (pid == 0) {
			execv ("/usr/local/bin/vinayaka-user-match-impl", argv);
		} else {
			int status;
			waitpid (pid, &status, 0);
			string result_2 = fetch_cache (host, user, hit);
			cout << "Access-Control-Allow-Origin: *" << endl;
			cout << "Content-Type: application/json" << endl << endl;
			cout << result_2;
		}
	}
}


