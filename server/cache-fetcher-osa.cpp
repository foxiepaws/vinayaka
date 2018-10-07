#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <socialnet-1.h>

#include "picojson.h"
#include "distsn.h"


using namespace std;


static string get_lightweight_api (string in, set <socialnet::HostNameAndUserName> friends, string listener_host, string listener_user)
{
	picojson::value json_value;
	string json_parse_error = picojson::parse (json_value, in);
	if (! json_parse_error.empty ()) {
		cerr << json_parse_error << endl;
		exit (1);
	}

	auto users_array = json_value.get <picojson::array> ();
	
	vector <string> osa_format_for_users;
	
	for (auto user_value: users_array) {
		auto user_object = user_value.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string user = user_object.at (string {"user"}).get <string> ();
		double similarity = user_object.at (string {"similarity"}).get <double> ();
		bool blacklisted = user_object.at (string {"blacklisted"}).get <bool> ();
		string screen_name = user_object.at (string {"screen_name"}).get <string> ();
		string bio = user_object.at (string {"bio"}).get <string> ();
		string avatar = user_object.at (string {"avatar"}).get <string> ();
		bool following_bool = socialnet::following (host, user, friends);
		bool local = (host == listener_host);
		string type = user_object.at (string {"type"}).get <string> ();
		bool bot = (type == string {"Service"});
		string url = user_object.at (string {"url"}).get <string> ();

		if ((! local) && (! following_bool) && (! blacklisted) && (! bot)) {
			stringstream out;
			out
				<< "{"
				<< "\"to_id\":\"" << escape_json (user + string {"@"} + host) << "\","
				<< "\"cnt\":\"" << similarity << "\","
				<< "\"from_id\":[],"
				<< "\"from_cnt\":" << similarity << ","
				<< "\"name\":\"" << escape_json (screen_name) << "\","
				<< "\"icon\":\"" << escape_json (avatar) << "\","
				<< "\"url\":\"" << escape_json (url) << "\""
				<< "}";
			osa_format_for_users.push_back (out.str ());
		}
	}

	unsigned int size = min (static_cast <unsigned int> (400), static_cast <unsigned int> (osa_format_for_users.size ()));
	
	stringstream out;

	out << "{";

	out << "\"status\":200,";
	out << "\"cnt\":" << size << ",";
	out << "\"ids\":";
	
	out << string {"["};

	for (unsigned int cn = 0; cn < size; cn ++) {
		if (0 < cn) {
			out << string {","};
		}
		out << osa_format_for_users.at (cn);
	}

	out << string {"]"};
	out << string {"}"};

	return out.str ();
}


int main (int argc, char *argv [])
{
	string host {argv [1]};
	string user {argv [2]};

	bool hit;
	string result = fetch_cache (host, user, hit);
	if (hit) {
		auto socialnet_user = socialnet::make_user (host, user, make_shared <socialnet::Http> ());
		auto friends = socialnet_user->get_friends_no_exception ();
		cout << "Access-Control-Allow-Origin: *" << endl;
		cout << "Content-Type: application/json" << endl << endl;
		cout << get_lightweight_api (result, friends, host, user);
	} else {
		pid_t pid = fork ();
		if (pid == 0) {
			execv ("/usr/local/bin/vinayaka-user-match-impl", argv);
		} else {
			auto socialnet_user = socialnet::make_user (host, user, make_shared <socialnet::Http> ());
			auto friends = socialnet_user->get_friends_no_exception ();
			int status;
			waitpid (pid, &status, 0);
			string result_2 = fetch_cache (host, user, hit);
			cout << "Access-Control-Allow-Origin: *" << endl;
			cout << "Content-Type: application/json" << endl << endl;
			cout << get_lightweight_api (result_2, friends, host, user);
		}
	}
}


