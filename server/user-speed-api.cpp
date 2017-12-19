#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include "distsn.h"


using namespace std;


int main (int argc, char **argv)
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	cout << "Content-type: application/json" << endl << endl;
	cout << "[";
	for (unsigned int cn = 0; cn < users_and_speed.size (); cn ++) {
		if (0 < cn) {
			cout << ",";
		}
		auto user = users_and_speed.at (cn);
		cout
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"username\":\"" << escape_json (user.username) << "\","
			<< "\"speed\":" << scientific << user.speed << ","
			<< "\"blacklisted\":" << (user.blacklisted? "true": "false")
			<< "}";
	}
	cout << "]";
	return 0;
}


