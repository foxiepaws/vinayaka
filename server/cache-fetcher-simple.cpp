#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "picojson.h"
#include "distsn.h"


using namespace std;


static string get_lightweight_api (string in)
{
	string out;

	picojson::value json_value;
	string json_parse_error = picojson::parse (json_value, in);
	if (! json_parse_error.empty ()) {
		cerr << json_parse_error << endl;
		exit (1);
	}

	auto users_array = json_value.get <picojson::array> ();
	
	out += string {"["};

	for (unsigned int cn = 0; cn < 100 && cn < users_array.size (); cn ++) {
		if (0 < cn) {
			out += string {","};
		}
	
		auto user_value = users_array.at (cn);
		auto user_object = user_value.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string user = user_object.at (string {"user"}).get <string> ();
		double similarity = user_object.at (string {"similarity"}).get <double> ();
		bool blacklisted = user_object.at (string {"blacklisted"}).get <bool> ();
		string screen_name = user_object.at (string {"screen_name"}).get <string> ();
		string bio = user_object.at (string {"bio"}).get <string> ();
		string avatar = user_object.at (string {"avatar"}).get <string> ();
		bool following = user_object.at (string {"following"}).get <bool> ();
		
		stringstream out_user;
		out_user
			<< "{"
			<< "\"host\":\"" << escape_json (host) << "\","
			<< "\"user\":\"" << escape_json (user) << "\","
			<< "\"similarity\":" << similarity << ","
			<< "\"blacklisted\":" << (blacklisted? "true": "false") << ","
			<< "\"screen_name\":\"" << escape_json (screen_name) << "\","
			<< "\"bio\":\"" << escape_json (bio) << "\","
			<< "\"avatar\":\"" << escape_json (avatar) << "\","
			<< "\"following\":" << (following? "true": "false")
			<< "}";
		out += out_user.str ();
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
		cout << get_lightweight_api (result);
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
			cout << get_lightweight_api (result_2);
		}
	}
}


