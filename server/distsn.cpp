#include <iostream>
#include <sstream>
#include <curl/curl.h>
#include <cassert>
#include "distsn.h"


using namespace tinyxml2;
using namespace std;


string escape_json (string in)
{
	string out;
	for (auto c: in) {
		if (c == '\n') {
			out += string {"\\n"};
		} else if (0x00 <= c && c < 0x20) {
			out += string {"�"};
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


static string remove_character_reference (string in)
{
	string out;
	string code;
	unsigned int state = 0;
	for (auto c: in) {
		switch (state) {
		case 0:
			if (c == '&') {
				code.push_back (c);
				state = 1;
			} else {
				out.push_back (c);
			}
			break;
		case 1:
			if (c == ';') {
				code.push_back (c);
				if (code == string {"&amp;"}) {
					out += string {"&"};
				} else if (code == string {"&lt;"}) {
					out += string {"<"};
				} else if (code == string {"&gt;"}) {
					out += string {">"};
				} else if (code == string {"&quot;"}) {
					out += string {"\""};
				} else if (code == string {"&apos;"}) {
					out += string {"'"};
				} else if (code == string {"&#39;"}) {
					out += string {"'"};
				} else {
					out += code;
				}
				code.clear ();
				state = 0;
			} else {
				code.push_back (c);
			}
			break;
		default:
			assert (false);
		}
	}
	out += code;
	return out;
}


static bool character_for_username (char c)
{
	return ('a' <= c && c <= 'z')
	|| ('A' <= c && c <= 'Z')
	|| ('0' <= c && c <= '9')
	|| c == '@'
	|| c == '_'
	|| c == '-'
	|| c == '.';
}


static string remove_mention (string in)
{
	string out;
	unsigned int state = 0;
	for (auto c: in) {
		switch (state) {
		case 0:
			if (c == '@') {
				state = 1;
			} else {
				out.push_back (c);
			}
			break;
		case 1:
			if (! character_for_username (c)) {
				out.push_back (c);
				state = 0;
			}
			break;
		default:
			assert (false);
		}
	}
	return out;
}


static string small_letterize (string in)
{
	string out;
	for (auto c: in) {
		if ('A' <= c && c <= 'Z') {
			out.push_back (c - 'A' + 'a');
		} else {
			out.push_back (c);
		}
	}
	return out;
}


static string katakana_to_latin (string in)
{
	map <string, string> rules {
		{string {"゠"}, string {"="}},
		{string {"ァ"}, string {"a"}},
		{string {"ア"}, string {"a"}},
		{string {"ィ"}, string {"i"}},
		{string {"イ"}, string {"i"}},
		{string {"ゥ"}, string {"u"}},
		{string {"ウ"}, string {"u"}},
		{string {"ェ"}, string {"e"}},
		{string {"エ"}, string {"e"}},
		{string {"ォ"}, string {"o"}},
		{string {"オ"}, string {"o"}},
		{string {"カ"}, string {"ka"}},
		{string {"ガ"}, string {"ga"}},
		{string {"キ"}, string {"ki"}},
		{string {"ギ"}, string {"gi"}},
		{string {"ク"}, string {"ku"}},
		{string {"グ"}, string {"gu"}},
		{string {"ケ"}, string {"ke"}},
		{string {"ゲ"}, string {"ge"}},
		{string {"コ"}, string {"ko"}},
		{string {"ゴ"}, string {"go"}},
		{string {"サ"}, string {"sa"}},
		{string {"ザ"}, string {"za"}},
		{string {"シ"}, string {"si"}},
		{string {"ジ"}, string {"zi"}},
		{string {"ス"}, string {"su"}},
		{string {"ズ"}, string {"zu"}},
		{string {"セ"}, string {"se"}},
		{string {"ゼ"}, string {"ze"}},
		{string {"ソ"}, string {"so"}},
		{string {"ゾ"}, string {"zo"}},
		{string {"タ"}, string {"ta"}},
		{string {"ダ"}, string {"da"}},
		{string {"チ"}, string {"ti"}},
		{string {"ヂ"}, string {"zi"}},
		{string {"ッ"}, string {"x"}},
		{string {"ツ"}, string {"tu"}},
		{string {"ヅ"}, string {"zu"}},
		{string {"テ"}, string {"te"}},
		{string {"デ"}, string {"de"}},
		{string {"ト"}, string {"to"}},
		{string {"ド"}, string {"do"}},
		{string {"ナ"}, string {"na"}},
		{string {"ニ"}, string {"ni"}},
		{string {"ヌ"}, string {"nu"}},
		{string {"ネ"}, string {"ne"}},
		{string {"ノ"}, string {"no"}},
		{string {"ハ"}, string {"ha"}},
		{string {"バ"}, string {"ba"}},
		{string {"パ"}, string {"pa"}},
		{string {"ヒ"}, string {"hi"}},
		{string {"ビ"}, string {"bi"}},
		{string {"ピ"}, string {"pi"}},
		{string {"フ"}, string {"hu"}},
		{string {"ブ"}, string {"bu"}},
		{string {"プ"}, string {"pu"}},
		{string {"ヘ"}, string {"he"}},
		{string {"ベ"}, string {"be"}},
		{string {"ペ"}, string {"pe"}},
		{string {"ホ"}, string {"ho"}},
		{string {"ボ"}, string {"bo"}},
		{string {"ポ"}, string {"po"}},
		{string {"マ"}, string {"ma"}},
		{string {"ミ"}, string {"mi"}},
		{string {"ム"}, string {"mu"}},
		{string {"メ"}, string {"me"}},
		{string {"モ"}, string {"mo"}},
		{string {"ャ"}, string {"a"}},
		{string {"ヤ"}, string {"ya"}},
		{string {"ュ"}, string {"u"}},
		{string {"ユ"}, string {"ya"}},
		{string {"ョ"}, string {"o"}},
		{string {"ヨ"}, string {"yo"}},
		{string {"ラ"}, string {"ra"}},
		{string {"リ"}, string {"ri"}},
		{string {"ル"}, string {"ru"}},
		{string {"レ"}, string {"re"}},
		{string {"ロ"}, string {"ro"}},
		{string {"ヮ"}, string {"a"}},
		{string {"ワ"}, string {"wa"}},
		{string {"ヰ"}, string {"i"}},
		{string {"ヱ"}, string {"e"}},
		{string {"ヲ"}, string {"o"}},
		{string {"ン"}, string {"n"}},
		{string {"ヴ"}, string {"b"}},
		{string {"ヵ"}, string {"箇"}},
		{string {"ヶ"}, string {"箇"}},
		{string {"ヷ"}, string {"b"}},
		{string {"ヸ"}, string {"be"}},
		{string {"ヹ"}, string {"bi"}},
		{string {"ヺ"}, string {"bo"}},
		{string {"・"}, string {"-"}},
		{string {"ー"}, string {""}},
		{string {"ヽ"}, string {""}},
		{string {"ヾ"}, string {""}},
		{string {"ヿ"}, string {""}},
	};

	string out;

	for (unsigned int cn = 0; cn < in.size (); ) {
		bool hit = false;
		for (auto rule: rules) {
			string katakana = rule.first;
			string latin = rule.second;
			if (katakana.size () + cn <= in.size () && in.substr (cn, katakana.size ()) == katakana) {
				out += latin;
				hit = true;
				cn += katakana.size ();
				break;
			}
		}
		if (! hit) {
			auto c = in.at (cn);
			out.push_back (c);
			cn ++;
		}
	}

	return out;
}


class OccupancyCount {
public:
	string word;
	double count;
public:
	OccupancyCount (): count (0) { /* Do nothing. */};
	OccupancyCount (string a_word, double a_count) {
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
	return string {"ぁ"} <= codepoint && codepoint <= string {"ゟ"};
}


static bool contains_hiragana (string word)
{
	for (unsigned int cn = 0; cn + 3 <= word.size (); cn ++) {
		if (is_hiragana (word.substr (cn, 3))) {
			return true;
		}
	}
	return false;
}


static bool effective_character (char c)
{
	return ('a' <= c && c <= 'z')
		||  ('A' <= c && c <= 'Z')
		||  ('0' <= c && c <= '9')
		||  (! (0x00 <= c && c <= 0x7f));
}


static bool effective_word (string s)
{
	for (auto c: s) {
		if (! effective_character (c)) {
			return false;
		}
	}
	return true;
}


static unsigned int number_of_numbers (string word)
{
	const set <string> numbers {
		string {"0"},
		string {"1"},
		string {"2"},
		string {"3"},
		string {"4"},
		string {"5"},
		string {"6"},
		string {"7"},
		string {"8"},
		string {"9"},
		string {"."},
		string {","},
		string {":"},
		string {"#"},
		string {"$"},
		string {"\""},
		string {"€"},
		string {"年"},
		string {"月"},
		string {"日"},
		string {"時"},
		string {"分"},
	};

	unsigned int number_of_numbers = 0;

	for (unsigned int cn = 0; cn < word.size (); cn ++) {
		for (string number: numbers) {
			if (number.size () + cn <= word.size ()
				&& word.substr (cn, number.size ()) == number
			) {
				number_of_numbers += number.size ();
			}
		}
	}
	
	return number_of_numbers;
}


static bool too_much_numbers (string word)
{
	return word.size () < number_of_numbers (word) * 2;
}


static bool contains_japanese_punctuations (string word)
{
	vector <string> japanese_punctuations {
		string {"、"},
		string {"。"},
		string {"，"},
		string {"．"},
		string {"？"},
		string {"！"},
		string {"「"},
		string {"」"},
		string {"（"},
		string {"）"},
		string {"『"},
		string {"』"},
		string {"【"},
		string {"】"},
	};
	for (auto punctuation: japanese_punctuations) {
		if (word.find (punctuation) != string::npos) {
			return true;
		}
	}
	return false;
}


static bool valid_word (string word)
{
	return
		starts_with_utf8_codepoint_boundary (word)
		&& effective_word (word)
		&& (! too_much_numbers (word))
		&& (! contains_hiragana (word))
		&& (! contains_japanese_punctuations (word));
}


static bool enquete_toot (string toot)
{
	return toot.find (string {"friends.nico"}) != string::npos
		&& toot.find (string {"アンケート"}) != string::npos;
}


static bool posted_by_third_party (string toot)
{
	return toot.find ("https://shindanmaker.com/") != string::npos
		|| toot.find ("http://mystery.distsn.org/") != string::npos
		|| toot.find ("https://mystery.distsn.org/") != string::npos
		|| toot.find ("https://vote.thedesk.top/") != string::npos;
}


static bool valid_toot (string toot)
{
	return (! enquete_toot (toot)) && (! posted_by_third_party (toot));
}


static bool is_alphabet (char c)
{
	return ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z');
}


static bool separate_at_alphabet (string toot, unsigned int offset)
{
	return
		0 < offset
		&& offset < toot.size ()
		&& is_alphabet (toot.at (offset - 1))
		&& is_alphabet (toot.at (offset));
}


static bool valid_position (string toot, unsigned int offset)
{
	return ! separate_at_alphabet (toot, offset);
}


vector <string> get_words_from_toots (vector <string> toots, unsigned int word_length, unsigned int vocabulary_size)
{
	return get_words_from_toots (toots, word_length, vocabulary_size, map <string, unsigned int> {}, 1);
}


vector <string> get_words_from_toots
	(vector <string> toots,
	unsigned int word_length,
	unsigned int vocabulary_size,
	map <string, unsigned int> words_to_popularity,
	unsigned int minimum_popularity)
{
	map <string, unsigned int> occupancy_count_map;
	for (auto raw_toot: toots) {
		string toot = remove_html (raw_toot);
		toot = remove_character_reference (toot);
		toot = remove_url (toot);
		toot = remove_mention (toot);
		toot = small_letterize (toot);
		if (valid_toot (toot)) {
			toot = katakana_to_latin (toot);
			if (word_length <= toot.size ()) {
				for (unsigned int offset = 0; offset <= toot.size () - word_length; offset ++) {
					string word = toot.substr (offset, word_length);
					if (valid_position (toot, offset) && valid_word (word)) {
						if (occupancy_count_map.find (word) == occupancy_count_map.end ()) {
							occupancy_count_map.insert (pair <string, unsigned int> {word, 1});
						} else {
							occupancy_count_map.at (word) ++;
						}
					}
				}
			}
		}
	}

	vector <OccupancyCount> occupancy_count_vector;
	for (auto i: occupancy_count_map) {
		string word {i.first};
		unsigned int occupancy_in_this_user {i.second};
		unsigned int popularity = minimum_popularity;
		if (words_to_popularity.find (word) != words_to_popularity.end ()) {
			popularity = words_to_popularity.at (word);
		}
		double rarity = get_rarity (popularity);
		double score = static_cast <double> (occupancy_in_this_user) * rarity;
		occupancy_count_vector.push_back (OccupancyCount {word, score});
	}
	sort (occupancy_count_vector.begin (), occupancy_count_vector.end (), by_count_desc);
	
	vector <string> words;
	for (unsigned int cn = 0; cn < occupancy_count_vector.size () && cn < vocabulary_size; cn ++) {
		words.push_back (occupancy_count_vector.at (cn).word);
	}
	
	return words;
}


double get_rarity (unsigned int popularity)
{
	return static_cast <double> (1.0) / static_cast <double> (popularity);
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


vector <vector <string>> parse_csv (FILE *in)
{
	string s;
	for (; ; ) {
	char b [1024];
		auto fgets_return = fgets (b, 1024, in);
		if (fgets_return == nullptr) {
			break;
		}
		s += string {b};
	}

	unsigned int state = 0;
	string cell_cache;
	vector <string> row_cache;
	vector <vector <string>> table_cache;

	unsigned int line = 1;

	for (auto c: s) {
		if (c == '\n') {
			line ++;
		}
		switch (state) {
		case 0:
			if (c == '"') {
				state = 1;
			} else if (c == '\r') {
				/* Do nothing, */
			} else if (c == '\n') {
				table_cache.push_back (row_cache);
				row_cache.clear ();
			} else {
				cerr << "Line: " << line << endl;
				cerr << "Character: " << c << endl;
				throw ParseException {__LINE__};
			}
			break;
		case 1:
			if (c == '"') {
				state = 2;
			} else {
				cell_cache.push_back (c);
			}
			break;
		case 2:
			if (c == '"') {
				cell_cache.push_back ('"');
				state = 1;
			} else if (c == ',') {
				row_cache.push_back (cell_cache);
				cell_cache.clear ();
				state = 0;
			} else if (c == '\r') {
				/* Do nothing, */
			} else if (c == '\n') {
				row_cache.push_back (cell_cache);
				cell_cache.clear ();
				table_cache.push_back (row_cache);
				row_cache.clear ();
				state = 0;
			} else {
				cerr << "Line: " << line << endl;
				cerr << "Character: " << c << endl;
				throw ParseException {__LINE__};
			}
			break;
		default:
			throw ParseException {__LINE__};
	}
}

return table_cache;
}


string escape_csv (string in)
{
	string out;
	for (auto c: in) {
		if (c == '"') {
			out.push_back ('"');
			out.push_back ('"');
		} else {
			out.push_back (c);
		}
	}
	return out;
}


string escape_utf8_fragment (string in) {
	string out;
	for (unsigned int cn = 0; cn < in.size (); cn ++) {
		unsigned int c = static_cast <unsigned char> (in.at (cn));
		if ((0x00 <= c && c < 0x20) || c == ' ') {
			out += "�";
		} else if ((0xc2 <= c && c <= 0xdf && in.size () - cn < 2)
			|| (0xe0 <= c && c <= 0xef && in.size () - cn < 3)
			|| (0xf0 <= c && c <= 0xf7 && in.size () - cn < 4)
			|| (0xf8 <= c && c <= 0xfb && in.size () - cn < 5)
			|| (0xfc <= c && c <= 0xfd && in.size () - cn < 6))
		{
			for (unsigned int x = 0; x < in.size () - cn; x ++) {
				out += "�";
			}
			break;
		} else {
			out.push_back (c);
		}
	}
	return out;
}


map <User, Profile> read_profiles ()
{
	FILE * in = fopen ("/var/lib/vinayaka/user-profiles.json", "rb");
	if (in == nullptr) {
		return map <User, Profile> {};
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

	map <User, Profile> users_to_profile;

	for (auto user_value: array) {
		auto user_object = user_value.get <picojson::object> ();
		string host;
		if (user_object.find (string {"host"}) != user_object.end ()) {
			host = user_object.at (string {"host"}).get <string> ();
		}
		string user;
		if (user_object.find (string {"user"}) != user_object.end ()) {
			user = user_object.at (string {"user"}).get <string> ();
		}
		string screen_name;
		if (user_object.find (string {"screen_name"}) != user_object.end ()) {
			screen_name = user_object.at (string {"screen_name"}).get <string> ();
		}
		string bio;
		if (user_object.find (string {"bio"}) != user_object.end ()) {
			bio = user_object.at (string {"bio"}).get <string> ();
		}
		string avatar;
		if (user_object.find (string {"avatar"}) != user_object.end ()) {
			avatar = user_object.at (string {"avatar"}).get <string> ();
		}
		string type;
		if (user_object.find (string {"type"}) != user_object.end ()) {
			type = user_object.at (string {"type"}).get <string> ();
		}
		string url = string {"https://"} + host + string {"/users/"} + user;
		if (user_object.find (string {"url"}) != user_object.end ()) {
			url = user_object.at (string {"url"}).get <string> ();
		}
		auto implementation = socialnet::eImplementation::UNKNOWN;
		if (user_object.find (string {"implementation"}) != user_object.end ()) {
			socialnet::decode (user_object.at (string {"implementation"}).get <string> (), implementation);
		}
		
		Profile profile;
		profile.screen_name = screen_name;
		profile.bio = bio;
		profile.avatar = avatar;
		profile.type = type;
		profile.url = url;
		profile.implementation = implementation;
		users_to_profile.insert (pair <User, Profile> {User {host, user}, profile});
	}
	return users_to_profile;
}


static bool charactor_for_url (char c)
{
	return
		('a' <= c && c <= 'z')
		|| ('A' <= c && c <= 'Z')
		|| ('0' <= c && c <= '9')
		|| c == ':'
		|| c == '/'
		|| c == '.'
		|| c == '-'
		|| c == '_'
		|| c == '%'
		|| c == '@';
}


bool safe_url (string url)
{
	for (auto c: url) {
		if (! charactor_for_url (c)) {
			return false;
		}
	}
	return true;
}


string fetch_cache (string a_host, string a_user, bool & a_hit)
{
	a_hit = false;

	string file_name {"/var/lib/vinayaka/match-cache.csv"};
	FileLock lock {file_name, LOCK_SH};

	vector <vector <string>> table;
	FILE * in = fopen (file_name.c_str (), "rb");
	if (in != nullptr) {
		table = parse_csv (in);
		fclose (in);
	}

	time_t now = time (nullptr);

	for (auto row: table) {
		if (3 < row.size ()
			&& row.at (0) == a_host
			&& row.at (1) == a_user)
		{
			stringstream timestamp_sstream {row.at (3)};
			time_t timestamp_time_t;
			timestamp_sstream >> timestamp_time_t;
			if (difftime (now, timestamp_time_t) < 60 * 60) {
				string result {row.at (2)};
				a_hit = true;
				return result;
			}
		}
	}

	return string {};
}


set <User> get_optouted_users ()
{
	map <User, Profile> users_to_profile = read_profiles ();
	
	set <User> optouted_users;
	for (auto user_to_profile: users_to_profile) {
		User user = user_to_profile.first;
		Profile profile = user_to_profile.second;
		string bio = profile.bio;
		bool optouted = (bio.find ("㊙️") != bio.npos);
		if (optouted) {
			optouted_users.insert (user);
		}
	}
	
	return optouted_users;
}


FileLock::FileLock (string a_path, int operation)
{
	path = a_path;
	fd = open (path.c_str (), O_RDWR);
	if (fd < 0) {
		abort ();
	}
	int reply = flock (fd, operation);
	if (reply < 0) {
		abort ();
	}
}


FileLock::~FileLock ()
{
	flock (fd, LOCK_UN);
	close (fd);
}


