#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include "picojson.h"
#include "distsn.h"


using namespace std;


int main (int argc, char *argv [])
{
	string host {argv [1]};
	string user {argv [2]};

	vector <vector <string>> table;
	FILE * in = fopen ("/var/lib/vinayaka/match-cache.csv", "rb");
	if (in != nullptr) {
		table = parse_csv (in);
		fclose (in);
	}

	time_t now = time (nullptr);

	for (auto row: table) {
		if (3 < row.size ()
			&& row.at (0) == host
			&& row.at (1) == user)
		{
			stringstream timestamp_sstream {row.at (3)};
			time_t timestamp_time_t;
			timestamp_sstream >> timestamp_time_t;
			if (difftime (now, timestamp_time_t) < 60 * 60) {
				string result {row.at (2)};
				cout << "Access-Control-Allow-Origin: *" << endl;
				cout << "Content-Type: application/json" << endl << endl;
				cout << result;
				return 0;
			}
		}
	}
	execv ("/usr/local/bin/vinayaka-user-match-resource-guard", argv);
}


