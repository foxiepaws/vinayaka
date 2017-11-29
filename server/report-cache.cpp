#include <iostream>
#include "unistd.h"
#include "picojson.h"
#include "distsn.h"


using namespace std;


int main (int argc, char *argv [])
{
	vector <vector <string>> table;
	FILE * in = fopen ("/var/lib/vinayaka/match-cache.csv", "rb");
	if (in != nullptr) {
		table = parse_csv (in);
		fclose (in);
	}
	cout << "Content-Type: application/json" << endl << endl;
	cout << "[";
	for (unsigned int cn = 0; cn < table.size (); cn ++) {
		auto row = table.at (cn);
		if (2 < row.size ()) {
			if (0 < cn) {
				cout << ",";
			}
			string host {row.at (0)};
			string user {row.at (1)};
			cout << "{"
				<< "\"host\":\"" << escape_json (host) << "\","
				<< "\"user\":\"" << escape_json (user) << "\""
				<< "}";
		}
	}
	cout << "]";
}


