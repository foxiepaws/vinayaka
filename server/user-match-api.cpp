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


class UserAndWords {
public:
	string user;
	string host;
	vector <string> words;
};


class UserAndSimilarity {
public:
	string user;
	string host;
	double similarity;
};


static bool by_similarity_desc (const UserAndSimilarity &a, const UserAndSimilarity &b)
{
	return b.similarity < a.similarity;
}


static vector <string> get_words (string user, string host)
{
	return vector <string> {};
}


static vector <UserAndWords> read_storage ()
{
	FILE * in = fopen ("/var/lib/vinayaka/user-words.xml", "rb");
	if (in == nullptr) {
		cerr << "File not found." << endl;
		exit (1);
	}
        string s;
        for (; ; ) {
                if (feof (in)) {
                        break;
                }
                char b [1024];
                fgets (b, 1024, in);
                s += string {b};
        }
        picojson::value json_value;
        picojson::parse (json_value, s);
        auto array = json_value.get <picojson::array> ();
        
        vector <UserAndWords> memo;
        
        for (auto user_value: array) {
        	auto user_object = user_value.get <picojson::object> ();
        	string user = user_object.at (string {"user"}).get <string> ();
        	string host = user_object.at (string {"host"}).get <string> ();
        	auto words_array = user_object.at (string {"words"}).get <picojson::array> ();
        	vector <string> words;
        	for (auto word_value: words_array) {
        		string word = word_value.get <string> ();
        		words.push_back (word);
        	}
        	UserAndWords user_and_words;
        	user_and_words.user = user;
        	user_and_words.host = host;
        	user_and_words.words = words;
        	memo.push_back (user_and_words);
        }
	return memo;
}


static double get_similarity (vector <string> listener, vector <string> speaker)
{
	set <string> listener_set {listener.begin (), listener.end ()};
	set <string> speaker_set {speaker.begin (), speaker.end ()};
	set <string> intersection;
	set_intersection (listener_set.begin (), listener_set.end (),
		speaker_set.begin (), speaker_set.end (),
		inserter (intersection, intersection.begin ()));
	double similarity = static_cast <double> (speaker_set.size ()) / static_cast <double> (intersection.size ());
	return similarity;
}


int main (int argc, char **argv)
{
	if (argc < 3) {
		exit (1);
	}
	string user {argv [1]};
	string host {argv [2]};
	vector <string> words = get_words (user, host);
	
	vector <UserAndWords> users_and_words = read_storage ();
	
	vector <UserAndSimilarity> users_and_similarity;
	for (auto user_and_words: users_and_words) {
		double similarity = get_similarity (words, user_and_words.words);
		UserAndSimilarity user_and_similarity;
		user_and_similarity.user = user_and_words.user;
		user_and_similarity.host = user_and_words.host;
		user_and_similarity.similarity = similarity;
		users_and_similarity.push_back (user_and_similarity);
	}
	
	sort (users_and_similarity.begin (), users_and_similarity.end (), by_similarity_desc);
	
	cout << "Content-Type: application/json" << endl << endl;
	cout << "[";
	for (unsigned int cn = 0; cn < users_and_similarity.size () && cn < 100; cn ++) {
		if (0 < cn) {
			cout << ",";
		}
		auto user = users_and_similarity.at (cn);
		cout
			<< "{"
			<< "\"host\":\"" << user.host << "\","
			<< "\"user\":\"" << user.host << "\","
			<< "\"similarity\":" << user.similarity
			<< "}";
	}
	cout << "]";
}



