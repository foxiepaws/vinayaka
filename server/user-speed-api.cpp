#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <algorithm>
#include <limits>
#include <sstream>
#include "distsn.h"


using namespace std;


int main (int argc, char **argv)
{
	unsigned int limit = numeric_limits <unsigned int>::max ();
	if (1 < argc) {
		stringstream {argv [1]} >> limit;
	}

	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	map <User, Profile> users_to_profile = read_profiles ();
	cout << "Access-Control-Allow-Origin: *" << endl;
	cout << "Content-type: application/json" << endl << endl;
	cout << "[";
	for (unsigned int cn = 0; cn < limit && cn < users_and_speed.size (); cn ++) {
		if (0 < cn) {
			cout << ",";
		}
		auto user = users_and_speed.at (cn);
		cout
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"username\":\"" << escape_json (user.username) << "\","
			<< "\"speed\":" << scientific << user.speed << ","
			<< "\"blacklisted\":" << (user.blacklisted? "true": "false") << ",";
			if (users_to_profile.find (User {user.host, user.username}) == users_to_profile.end ()) {
				cout
					<< "\"screen_name\":\"\","
					<< "\"avatar\":\"\","
					<< "\"type\":\"\","
					<< "\"url\":\"\","
					<< "\"implementation\":\"unknown\"";
			} else {
				Profile profile = users_to_profile.at (User {user.host, user.username});
				cout << "\"screen_name\":\"" << escape_json (profile.screen_name) << "\",";
				if (safe_url (profile.avatar)) {
					cout << "\"avatar\":\"" << escape_json (profile.avatar) << "\",";
				} else {
					cout << "\"avatar\":\"\",";
				}
				cout << "\"type\":\"" << escape_json (profile.type) << "\",";
				cout << "\"url\":\"" << escape_json (profile.url) << "\",";
				cout << "\"implementation\":\"" << socialnet::format (profile.implementation) << "\"";
			}
		cout
			<< "}";
	}
	cout << "]";
	return 0;
}


