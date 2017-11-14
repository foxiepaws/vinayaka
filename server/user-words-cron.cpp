#include <cstdio>
#include <cstdlib>
#include <curl/curl.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <set>
#include "picojson.h"
#include "tinyxml2.h"
#include "distsn.h"


using namespace tinyxml2;
using namespace std;


static string escape_json (string in)
{
	string out;
	for (auto c: in) {
		if (c == '\n') {
			out += string {"\\n"};
		} else if (c == '"'){
			out += string {"\\\""};
		} else if (c == '\\'){
			out += string {"\\\\"};
		} else {
			out.push_back (c);
		}
	}
	return out;
}


class User {
public:
	string host;
	string user;
	User () { /* Do nothing. */ };
	User (string a_host, string a_user) {
		host = a_host;
		user = a_user;
	};
};


static void get_profile (string host, string user, string &a_screen_name, string &a_bio, vector <string> &a_toots)
{
	try {
		/* FIXME! Mastodon Only! Not for Pleroma!!! */
		string atom_query = string {"https://"} + host + string {"/users/"} + user + string {".atom"};
		string atom_reply = http_get (atom_query);
		XMLDocument atom_document;
		XMLError atom_parse_error = atom_document.Parse (atom_reply.c_str ());
		if (atom_parse_error != XML_SUCCESS) {
			throw (UserException {__LINE__});
		}
		XMLElement * root_element = atom_document.RootElement ();
		if (root_element == nullptr) {
			throw (UserException {__LINE__});
		}
		XMLElement * feed_element = root_element;
		XMLElement * author_element = feed_element->FirstChildElement ("author");
		if (author_element == nullptr) {
			throw (UserException {__LINE__});
		}
		XMLElement * displayname_element = author_element->FirstChildElement ("poco:displayName");
		if (displayname_element == nullptr) {
			throw (UserException {__LINE__});
		}
		const char * displayname_text = displayname_element->GetText ();
		if (displayname_text == nullptr) {
			throw (UserException {__LINE__});
		}
		a_screen_name = string {displayname_text};
		XMLElement * note_element = author_element->FirstChildElement ("poco:note");
		if (note_element == nullptr) {
			throw (UserException {__LINE__});
		}
		const char * note_text = note_element->GetText ();
		if (note_text == nullptr) {
			throw (UserException {__LINE__});
		}
		a_bio = string {note_text};
	} catch (HttpException e) {
		throw (UserException {__LINE__});
	}
}


static vector <string> get_words_from_toots (vector <string>)
{
	return vector <string> {};
}


static vector <string> get_words (string host, string user)
{
	string screen_name;
	string bio;
	vector <string> toots;
	get_profile (host, user, screen_name, bio, toots);
	cout << user << "@" << host << endl;
	cout << screen_name << endl;
	cout << bio << endl << endl;
	vector <string> words = get_words_from_toots (toots);
	return words;
}


static vector <User> get_users ()
{
	vector <User> users;
	string reply = http_get ("http://distsn.org/cgi-bin/distsn-user-recommendation-api.cgi?10");
	picojson::value json_value;
	string error = picojson::parse (json_value, reply);
	if (! error.empty ()) {
		cerr << error << endl;
		exit (1);
	}
	auto user_jsons = json_value.get <picojson::array> ();
	for (auto user_json: user_jsons) {
		auto user_object = user_json.get <picojson::object> ();
		string host = user_object.at (string {"host"}).get <string> ();
		string username = user_object.at (string {"username"}).get <string> ();
		users.push_back (User {host, username});
	}
	return users;
}


int main (int argc, char **argv)
{
	auto users = get_users ();
	vector <pair <User, vector <string>>> users_and_words;
	for (auto user: users) {
		try {
			auto words = get_words (user.host, user.user);
			users_and_words.push_back (pair <User, vector <string>> {user, words});
		} catch (UserException e) {
			cerr << "Error " << user.user << "@" << user.host << " " << e.line << endl;
		}
	}
	ofstream out {"/var/lib/vinayaka/user-words.xml"};
	out << "[";
	for (unsigned int a = 0; a < users_and_words.size (); a ++) {
		if (0 < a) {
			out << ",";
		}
		auto user_and_words = users_and_words.at (a);
		auto user = user_and_words.first;
		auto words = user_and_words.second;
		out
			<< "{"
			<< "\"host\":\"" << escape_json (user.host) << "\","
			<< "\"user\":\"" << escape_json (user.user) << "\","
			<< "\"words\":"
			<< "[";
			for (unsigned int b = 0; b < words.size (); b ++) {
				if (0 < b) {
					out << ",";
				}
				auto word = words.at (b);
				out << "\"" << escape_json (word) << "\"";
			}
		out << "]" << "}";
	}
	out << "]";
}



