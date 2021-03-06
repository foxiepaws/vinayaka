#include <iostream>
#include <ctime>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <socialnet-1.h>

#include "distsn.h"
#include "picojson.h"


using namespace std;


static string get_filtered_api (string in, set <socialnet::HostNameAndUserName> friends, string listener_host, string listener_user)
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
		bool following_bool = socialnet::following (host, user, friends);
		bool local = (host == listener_host);
		string type = user_object.at (string {"type"}).get <string> ();
		bool bot = (type == string {"Service"});
		bool described_bool = described (screen_name, bio, avatar);

		if ((! local) && (! following_bool) && (! blacklisted) && (! bot) && described_bool) {
			stringstream out_user;
			out_user
				<< "{"
				<< "\"id\":\"" << escape_json (string {"0"}) << "\","
				<< "\"username\":\"" << escape_json (user) << "\","
				<< "\"acct\":\"" << escape_json (user + string {"@"} + host) << "\","
				<< "\"display_name\":\"" << escape_json (screen_name) << "\","
				<< "\"bot\":false,"
				<< "\"note\":\"" << escape_json (bio) << "\","
				<< "\"url\":\"" << escape_json (string {"https://"} + host + string {"/users/"} + user) << "\","
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

	string s;
	{
		string file_name {"/var/lib/vinayaka/users-new-cache.json"};
		FileLock lock {file_name, LOCK_SH};
		FILE *in = fopen (file_name.c_str (), "r");
		if (in == nullptr) {
			cerr << file_name << " can not open." << endl;
			exit (1);
		}
		for (; ; ) {
		char b [1024];
			auto fgets_return = fgets (b, 1024, in);
			if (fgets_return == nullptr) {
				break;
			}
			s += string {b};
		}
	}

	auto socialnet_user = socialnet::make_user (host, user, make_shared <socialnet::Http> ());
	auto friends = socialnet_user->get_friends_no_exception ();

	cout << "Access-Control-Allow-Origin: *" << endl;
	cout << "Content-Type: application/json" << endl << endl;
	cout << get_filtered_api (s, friends, host, user);
}

