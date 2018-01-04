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
	map <User, Profile> users_to_profile = read_profiles ();
	cout << "Access-Control-Allow-Origin: *" << endl;
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
			<< "\"blacklisted\":" << (user.blacklisted? "true": "false") << ",";
			if (400 <= cn || users_to_profile.find (User {user.host, user.username}) == users_to_profile.end ()) {
				cout
					<< "\"screen_name\":\"\","
					<< "\"avatar\":\"\"";
			} else {
				Profile profile = users_to_profile.at (User {user.host, user.username});
				cout << "\"screen_name\":\"" << escape_json (profile.screen_name) << "\",";
				if (safe_url (profile.avatar)) {
					cout << "\"avatar\":\"" << escape_json (profile.avatar) << "\"";
				} else {
					cout << "\"avatar\":\"\"";
				}
			}
		cout
			<< "}";
	}
	cout << "]";
	return 0;
}


