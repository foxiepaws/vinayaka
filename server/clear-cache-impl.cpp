#include <iostream>
#include <ctime>
#include <sstream>
#include <fstream>
#include <unistd.h>
#include "picojson.h"
#include "distsn.h"


using namespace std;


int main (int argc, char *argv [])
{
	vector <vector <string>> table_in;
	FILE * in = fopen ("/var/lib/vinayaka/match-cache.csv", "rb");
	if (in != nullptr) {
		table_in = parse_csv (in);
		fclose (in);
	}

	time_t now = time (nullptr);
	vector <vector <string>> table_out;

	for (auto row: table_in) {
		if (3 < row.size ()) {
			stringstream timestamp_sstream {row.at (3)};
			time_t timestamp_time_t;
			timestamp_sstream >> timestamp_time_t;
			if (difftime (now, timestamp_time_t) < 60 * 60) {
				table_out.push_back (row);
			}
		}
	}
	
	ofstream out {"/var/lib/vinayaka/match-cache.csv"};
	for (auto row: table_out) {
		for (auto cell: row) {
			out << "\"" << escape_csv (cell) << "\",";
		}
		out << endl;
	}
}


