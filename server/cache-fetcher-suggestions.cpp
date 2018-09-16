#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "picojson.h"
#include "distsn.h"


using namespace std;


static string get_filtered_api (string in, set <string> friends, string listener_host, string listener_user)
{

	picojson::value json_value;
	string json_parse_error = picojson::parse (json_value, in);
	if (! json_parse_error.empty ()) {
		cerr << json_parse_error << endl;
		exit (1);
	}

	auto users_array = json_value.get <picojson::array> ();

	vector <string> filtered_formats;

	for (unsigned int cn = 0; cn < users_array.size (); cn ++) {
		auto user_value = users_array.at (cn);
		auto user_object = user_value.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string user = user_object.at (string {"user"}).get <string> ();
		bool blacklisted = user_object.at (string {"blacklisted"}).get <bool> ();
		string screen_name = user_object.at (string {"screen_name"}).get <string> ();
		string bio = user_object.at (string {"bio"}).get <string> ();
		string avatar = user_object.at (string {"avatar"}).get <string> ();
		bool following_bool = following (host, user, friends);
		bool local = (host == listener_host);
		string type = user_object.at (string {"type"}).get <string> ();
		bool bot = (type == string {"Service"});
		string url = user_object.at (string {"url"}).get <string> ();

		if ((! local) && (! following_bool) && (! blacklisted) && (! bot)) {
			stringstream out_user;
			out_user
				<< "{"
				<< "\"id\":\"" << escape_json (string {"0"}) << "\","
				<< "\"username\":\"" << escape_json (user) << "\","
				<< "\"acct\":\"" << escape_json (user + string {"@"} + host) << "\","
				<< "\"display_name\":\"" << escape_json (screen_name) << "\","
				<< "\"bot\":false,"
				<< "\"note\":\"" << escape_json (bio) << "\","
				<< "\"url\":\"" << escape_json (url) << "\","
				<< "\"avatar\":\"" << escape_json (avatar) << "\","
				<< "\"avatar_static\":\"" << escape_json (avatar) << "\","
				<< "\"followers_count\":0,"
				<< "\"following_count\":0,"
				<< "\"statuses_count\":0"
				<< "}";
			filtered_formats.push_back (out_user.str ());
		}
	}

	string out;
	out += string {"["};
	for (unsigned int cn = 0; cn < filtered_formats.size (); cn ++) {
		if (0 < cn) {
			out += string {","};
		}
		out += filtered_formats.at (cn);
	}
	out += string {"]"};
	return out;
}


int main (int argc, char *argv [])
{
	string host {argv [1]};
	string user {argv [2]};

	bool hit;
	string result = fetch_cache (host, user, hit);
	if (hit) {
		cout << "Access-Control-Allow-Origin: *" << endl;
		cout << "Content-Type: application/json" << endl << endl;
		cout << get_filtered_api (result, get_friends (host, user), host, user);
	} else {
		pid_t pid = fork ();
		if (pid == 0) {
			execv ("/usr/local/bin/vinayaka-user-match-impl", argv);
		} else {
			set <string> friends = get_friends (host, user);
			int status;
			waitpid (pid, &status, 0);
			string result_2 = fetch_cache (host, user, hit);
			cout << "Access-Control-Allow-Origin: *" << endl;
			cout << "Content-Type: application/json" << endl << endl;
			cout << get_filtered_api (result_2, friends, host, user);
		}
	}
}


