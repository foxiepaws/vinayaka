#include <sstream>
#include <curl/curl.h>
#include "distsn.h"


using namespace tinyxml2;
using namespace std;


string get_id (const picojson::value &toot)
{
	if (! toot.is <picojson::object> ()) {
		throw (TootException {});
	}
	auto properties = toot.get <picojson::object> ();
	if (properties.find (string {"id"}) == properties.end ()) {
		throw (TootException {});
	}
	auto id_object = properties.at (string {"id"});
	string id_string;
	if (id_object.is <double> ()) {
		double id_double = id_object.get <double> ();
		stringstream s;
		s << static_cast <unsigned int> (id_double);
		id_string = s.str ();
	} else if (id_object.is <string> ()) {
		id_string = id_object.get <string> ();
	} else {
		throw (TootException {});
	}
	return id_string;
}


vector <picojson::value> get_timeline (string host)
{
	vector <picojson::value> timeline;

	{
		string reply = http_get (string {"https://"} + host + string {"/api/v1/timelines/public?local=true&limit=40"});

		picojson::value json_value;
		string error = picojson::parse (json_value, reply);
		if (! error.empty ()) {
			throw (HostException {});
		}
		if (! json_value.is <picojson::array> ()) {
			throw (HostException {});
		}
	
		vector <picojson::value> toots = json_value.get <picojson::array> ();
		timeline.insert (timeline.end (), toots.begin (), toots.end ());
	}
	
	if (timeline.size () < 1) {
		throw (HostException {});
	}
	
	for (unsigned int cn = 0; ; cn ++) {
		if (1000 <= cn) {
			throw (HostException {__LINE__});
		}
		
		time_t top_time;
		time_t bottom_time;
		try {
			top_time = get_time (timeline.front ());
			bottom_time = get_time (timeline.back ());
		} catch (TootException e) {
			throw (HostException {});
		}
		if (60 * 60 * 3 <= top_time - bottom_time && 40 <= timeline.size ()) {
			break;
		}

		string bottom_id;
		try {
			bottom_id = get_id (timeline.back ());
		} catch (TootException e) {
			throw (HostException {});
		}
		string query
			= string {"https://"}
			+ host
			+ string {"/api/v1/timelines/public?local=true&limit=40&max_id="}
			+ bottom_id;
		string reply = http_get (query);

		picojson::value json_value;
		string error = picojson::parse (json_value, reply);
		if (! error.empty ()) {
			throw (HostException {});
		}
		if (! json_value.is <picojson::array> ()) {
			throw (HostException {});
		}
	
		vector <picojson::value> toots = json_value.get <picojson::array> ();
		if (toots.empty ()) {
			break;
		}
		timeline.insert (timeline.end (), toots.begin (), toots.end ());
	}

	return timeline;
}


static int writer (char * data, size_t size, size_t nmemb, std::string * writerData)
{
	if (writerData == nullptr) {
		return 0;
	}
	writerData->append (data, size * nmemb);
	return size * nmemb;
}


string http_get (string url)
{
	CURL *curl;
	CURLcode res;
	curl_global_init (CURL_GLOBAL_ALL);

	curl = curl_easy_init ();
	if (! curl) {
		throw (HttpException {});
	}
	curl_easy_setopt (curl, CURLOPT_URL, url.c_str ());
	string reply_1;
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, writer);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, & reply_1);
	res = curl_easy_perform (curl);
	curl_easy_cleanup (curl);
	if (res != CURLE_OK) {
		throw (HttpException {});
	}
	return reply_1;
}


time_t get_time (const picojson::value &toot)
{
	if (! toot.is <picojson::object> ()) {
		throw (TootException {});
	}
	auto properties = toot.get <picojson::object> ();
	if (properties.find (string {"created_at"}) == properties.end ()) {
		throw (TootException {});
	}
	auto time_object = properties.at (string {"created_at"});
	if (! time_object.is <string> ()) {
		throw (TootException {});
	}
	auto time_s = time_object.get <string> ();
	return str2time (time_s);
}


time_t str2time (string s)
{
	struct tm tm;
	strptime (s.c_str (), "%Y-%m-%dT%H:%M:%S", & tm);
	return timegm (& tm);
}


string escape_json (string in)
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


static string remove_html (string in)
{
	string out;
	unsigned int state = 0;
	for (auto c: in) {
		switch (state) {
		case 0:
			if (c == '<') {
				state = 1;
			} else {
				out.push_back (c);
			}
			break;
		case 1:
			if (c == '>') {
				state = 0;
			}
			break;
		default:
			abort ();
		}
	}
	return out;
}


static string remove_url (string in)
{
	string out;
	unsigned int state = 0;
	string cache;
	for (auto c: in) {
		switch (state) {
		case 0:
			if (c == 'h') {
				cache.push_back (c);
				state = 1;
			} else {
				out.push_back (c);
			}
			break;
		case 1:
			if (c == 't') {
				cache.push_back (c);
				state = 2;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 2:
			if (c == 't') {
				cache.push_back (c);
				state = 3;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 3:
			if (c == 'p') {
				cache.push_back (c);
				state = 4;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 4:
			if (c == 's') {
				cache.push_back (c);
				state = 5;
			} else if (c == ':') {
				cache.push_back (c);
				state = 6;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 5:
			if (c == ':') {
				cache.push_back (c);
				state = 6;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 6:
			if (c == '/') {
				cache.push_back (c);
				state = 7;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 7:
			if (c == '/') {
				cache.clear ();
				state = 8;
			} else {
				out += cache;
				out.push_back (c);
				cache.clear ();
				state = 0;
			}
			break;
		case 8:
			if (0x20 < c && c < 0x7f) {
				/* Do notthing. */
			} else {
				out.push_back (c);
				state = 0;
			}
			break;
		default:
			abort ();
		}
	}
	out += cache;
	return out;
}


class OccupancyCount {
public:
	string word;
	unsigned int count;
public:
	OccupancyCount (): count (0) { /* Do nothing. */};
	OccupancyCount (string a_word, unsigned int a_count) {
		word = a_word;
		count = a_count;
	};
};


static bool by_count_desc (const OccupancyCount &a, const OccupancyCount &b)
{
	return b.count < a.count || (a.count == b.count && b.word < a.word);
}


static bool starts_with_utf8_codepoint_boundary (string word)
{
	unsigned int first_octet = static_cast <unsigned char> (word.at (0));
	return (! (0x80 <= first_octet && first_octet < 0xC0));
}


static bool is_hiragana (string codepoint)
{
	return (string {"ぁ"} <= codepoint && codepoint <= string {"ゟ"})
		|| codepoint == string {"、"}
		|| codepoint == string {"。"}
		|| codepoint == string {"？"}
		|| codepoint == string {"！"}
		|| codepoint == string {"思"};
}


static bool is_katakana (string codepoint)
{
	/* 長音「ー」を含む。 */
	return (string {"ァ"} <= codepoint && codepoint <= string {"ヿ"})
		|| codepoint == string {"「"}
		|| codepoint == string {"」"};
}


static bool all_kana (string word)
{
	for (unsigned int cn = 0; cn * 3 + 2 < word.size (); cn ++) {
		string codepoint = word.substr (cn * 3, 3);
		if (! (is_hiragana (codepoint) || is_katakana (codepoint))) {
			return false;
		}
	}
	return true;
}


static bool all_hiragana (string word)
{
	for (unsigned int cn = 0; cn * 3 + 2 < word.size (); cn ++) {
		string codepoint = word.substr (cn * 3, 3);
		if (! is_hiragana (codepoint)) {
			return false;
		}
	}
	return true;
}


static bool valid_word (string word)
{
	bool invalid
		= (! starts_with_utf8_codepoint_boundary (word))
		|| (word.size () < 9 && all_kana (word))
		|| (word.size () < 12 && all_hiragana (word));
	return ! invalid;
}


vector <string> get_words_from_toots (vector <string> toots, unsigned int word_length, unsigned int vocabulary_size)
{
	map <string, unsigned int> occupancy_count_map;
	for (auto raw_toot: toots) {
		string toot = remove_html (raw_toot);
		toot = remove_url (toot);
		if (word_length <= toot.size ()) {
			for (unsigned int offset = 0; offset <= toot.size () - word_length; offset ++) {
				string word = toot.substr (offset, word_length);
				if (valid_word (word)) {
					if (occupancy_count_map.find (word) == occupancy_count_map.end ()) {
						occupancy_count_map.insert (pair <string, unsigned int> {word, 1});
					} else {
						occupancy_count_map.at (word) ++;
					}
				}
			}
		}
	}

	vector <OccupancyCount> occupancy_count_vector;
	for (auto i: occupancy_count_map) {
		occupancy_count_vector.push_back (OccupancyCount {i.first, i.second});
	}
	sort (occupancy_count_vector.begin (), occupancy_count_vector.end (), by_count_desc);
	
	vector <string> words;
	for (unsigned int cn = 0; cn < occupancy_count_vector.size () && cn < vocabulary_size; cn ++) {
		words.push_back (occupancy_count_vector.at (cn).word);
	}
	
	return words;
}


static void get_profile_impl (string atom_query, string &a_screen_name, string &a_bio, vector <string> &a_toots, string &a_avatar)
{
	try {
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

		string avatar;
		XMLElement * logo_element = feed_element->FirstChildElement ("logo");
		if (logo_element != nullptr) {
			const char * logo_text = logo_element->GetText ();
			if (logo_text != nullptr) {
				avatar = string {logo_text};
			}
		}
		a_avatar = avatar;

		XMLElement * author_element = feed_element->FirstChildElement ("author");
		if (author_element == nullptr) {
			throw (UserException {__LINE__});
		}
		XMLElement * displayname_element = author_element->FirstChildElement ("poco:displayName");
		a_screen_name = string {};
		if (displayname_element != nullptr) {
			const char * displayname_text = displayname_element->GetText ();
			if (displayname_text != nullptr) {
				a_screen_name = string {displayname_text};
			}
		}
		a_bio = string {};
		XMLElement * note_element = author_element->FirstChildElement ("poco:note");
		if (note_element != nullptr) {
			const char * note_text = note_element->GetText ();
			if (note_text != nullptr) {
				a_bio = string {note_text};
			}
		}
		vector <string> toots;
		for (XMLElement * entry_element = feed_element->FirstChildElement ("entry");
			entry_element != nullptr;
			entry_element = entry_element->NextSiblingElement ("entry"))
		{
			XMLElement * verb_element = entry_element->FirstChildElement ("activity:verb");
			if (verb_element == nullptr) {
				throw (UserException {__LINE__});
			}
			const char * verb_text = verb_element->GetText ();
			if (verb_text == nullptr) {
				throw (UserException {__LINE__});
			}
			if (string {verb_text} == string {"http://activitystrea.ms/schema/1.0/post"}) {
				XMLElement * content_element = entry_element->FirstChildElement ("content");
				if (content_element != nullptr) {
					const char * content_text = content_element->GetText ();
					if (content_text != nullptr) {
						toots.push_back (string {content_text});
						toots.push_back (string {content_text});
					}
				}
			} else if (string {verb_text} == string {"http://activitystrea.ms/schema/1.0/share"}) {
				XMLElement * activity_object_element = entry_element->FirstChildElement ("activity:object");
				if (activity_object_element != nullptr) {
					XMLElement * content_element = activity_object_element->FirstChildElement ("content");
					if (content_element != nullptr) {
						const char * content_text = content_element->GetText ();
						if (content_text != nullptr) {
							toots.push_back (string {content_text});
						}
					}
				}
			}
		}
		a_toots = toots;
	} catch (HttpException e) {
		throw (UserException {__LINE__});
	}
}


void get_profile (string host, string user, string &a_screen_name, string &a_bio, vector <string> &a_toots, string & a_avatar)
{
	try {
		string query = string {"https://"} + host + string {"/users/"} + user + string {".atom"};
		get_profile_impl (query, a_screen_name, a_bio, a_toots, a_avatar);
	} catch (UserException e) {
		string query = string {"https://"} + host + string {"/users/"} + user + string {"/feed.atom"};
		get_profile_impl (query, a_screen_name, a_bio, a_toots, a_avatar);
	}
}


void get_profile (string host, string user, string &a_screen_name, string &a_bio, vector <string> &a_toots)
{
	string avatar;
	get_profile (host, user, a_screen_name, a_bio, a_toots, avatar);
}


vector <string> get_words (string host, string user, unsigned int word_length, unsigned int vocabulary_size)
{
	cerr << user << "@" << host << endl;
	string screen_name;
	string bio;
	vector <string> toots;
	get_profile (host, user, screen_name, bio, toots);
	toots.push_back (screen_name);
	toots.push_back (screen_name);
	toots.push_back (bio);
	toots.push_back (bio);
	vector <string> words = get_words_from_toots (toots, word_length, vocabulary_size);
	return words;
}


vector <UserAndWords> read_storage (string filename)
{
	FILE * in = fopen (filename.c_str (), "rb");
	if (in == nullptr) {
		throw (ModelException {__LINE__});
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



