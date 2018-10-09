#include <iostream>
#include <limits>
#include <unistd.h>
#include <sys/wait.h>

#include <languagemodel-1.h>

#include "distsn.h"


using namespace std;


static vector <User> get_users (unsigned int size)
{
	vector <UserAndSpeed> users_and_speed = get_users_and_speed ();
	vector <User> users;
	for (unsigned int cn = 0; cn < users_and_speed.size () && cn < size; cn ++) {
		UserAndSpeed user_and_speed = users_and_speed.at (cn);
		User user {user_and_speed.host, user_and_speed.username};
		users.push_back (user);
	}
	return users;
}


int main (int argc, char **argv)
{
	auto all_users = get_users (numeric_limits <unsigned int>::max ());

	socialnet::Hosts hosts;
	hosts.initialize ();

	vector <User> misskey_users;

	for (auto user: all_users) {
		string host_name = user.host;
		auto host = hosts.get (host_name);
		if (host && host->implementation () == socialnet::eImplementation::MISSKEY) {
			misskey_users.push_back (user);
		}
	}

	for (unsigned int cn = 0; cn < 720 && cn < misskey_users.size (); cn ++) {
		auto user = misskey_users.at (cn);
		cerr << cn << " " << user.host << " " << user.user << endl;

		auto pid = fork ();
		if (pid < 0) {
			/* Error */
			/* Do nothing */
		} else if (pid == 0) {
			/* Child */
			execl ("/usr/lib/cgi-bin/inayaka-user-match-api.cgi", user.host.c_str (), user.user.c_str ());
		} else {
			/* Parent */
			int status = 0;
			wait (& status);
		}
	}
}


