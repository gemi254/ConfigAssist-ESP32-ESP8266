#include "dYaml.h"
#define INT_MAX (int) ((unsigned) -1 / 2)

namespace dyml
{
	template <typename T>
	const T& dyml_min(const T& a, const T& b) {	return (a < b) ? a : b; }

	template <typename T>
	inline bool startwith(const char* str, T& pre)
	{
		const auto n = sizeof(pre) / sizeof(char) - 1;
		return (0 == strncmp(pre, str, n));
	}

	size_t split_ss(vector<kvSize>& ss, char* str)
	{
		ss.resize(0);

		char* sp1 = str;
		char* sp2 = sp1;

		while (*sp2)
		{
			const char ch = *sp2;
			if ('\r' == ch || '\n' == ch)
			{
				*sp2 = 0;

				if (sp2 > sp1) ss.emplace_back(sp1 - str, sp2 - sp1);

				++sp2;
				sp1 = sp2;

				continue;
			}

			++sp2;
		}

		if (sp2 > sp1)
			ss.emplace_back(sp1 - str, sp2 - sp1);

		return ss.size();

	}

	int Directyaml::first_not(const char* sp, char ch)
	{
		auto p = sp;
		while (*p && *p == ch) ++p;
		return (0 == *p) ? -1 : int(p - sp);
	}

	kvSize Directyaml::trim(const char* s)
	{
		auto b = s;
		auto e = s + strlen(s);
		while (b < e && ' ' == (*b)) ++b;
		while (e > b && ' ' == *(e - 1)) --e;
		return kvSize(b - s, e - b);
	}

#ifdef __DYML_ALLOW_LINE_END_COMMENT__
	int Directyaml::char_count(const char* sp, char ch)
	{
		int cnt = 0;
		while (*sp) { if (*sp == ch) ++cnt; ++sp; }
		return cnt;
	}

	void Directyaml::remove_line_end_comment(char* s)
	{
		while (*s && *s == ' ') ++s;

		if (0 == *s) return;
		if ('#' == *s) { *s = 0; return; }

		char* cp = nullptr;

		// find last " #"
		auto p = s + (strlen(s) - 1);
		while (p > s)
		{
			if (*p == '#' && *(p - 1) == ' ')
			{
				cp = p - 1;
				break;
			}

			--p;
		}

		if (cp)
		{
			*cp = 0;

			const auto front_quotes = char_count(s, '\"');

			if (*s == '\"' && 0 != (front_quotes % 2))
			{
				*cp = ' '; // restore
			}
		}
	}

#endif

	int Directyaml::child(const Node* parent, const char* key) const
	{
		if (parent->_level < 0)
		{
			for (auto i : _topLevels)
			{
				const auto& row = _rows[i];
				if (0 == strcmp(row.key, key)) return i;
			}
		}

		else
		{
			const auto now = (int)_rows.size();
			for (int i = parent->_row + 1; i < now; ++i)
			{
				const auto& row = _rows[i];
				if (row.level == parent->_level) break;
				if (row.level == (parent->_level + 1))
				{
					if (0 == strcmp(row.key, key)) return i;
				}
			}
		}

		return -1;
	}

	int Directyaml::child(const Node* parent, int childIndex) const
	{
		const auto now = (int)_rows.size();

		int ci = 0;
		for (int i = parent->_row + 1; i < now; ++i)
		{
			const auto& row = _rows[i];
			if (row.level == parent->_level) break;
			if (row.level == (parent->_level + 1))
			{
				if (ci == childIndex)
					return i;

				++ci;
			}
		}

		return -1;
	}

	int Directyaml::children(const Node* parent) const
	{
		if (parent->_level < 0)
			return (int)_topLevels.size();

		int noc = 0;

		const auto now = (int)_rows.size();
		for (int i = parent->_row + 1; i < now; ++i)
		{
			const auto& row = _rows[i];
			if (row.level == parent->_level) break;
			if (row.level == (parent->_level + 1)) ++noc;
		}

		return noc;
	}

	void Directyaml::checkIndents()
	{
		const auto now = (int)_rows.size();

		vector<int> todo(now);

		for (;;)
		{
			int cnt = 0;

			auto parent = _rows[0].level;
			for (int i = 1; i < now; ++i)
			{
				auto& row = _rows[i];

				// ill indented?
				if (row.level > (parent + 1))
				{
					todo[cnt++] = i;

					const auto cl = row.level;
					for (int j = i + 1; j < now; ++j)
					{
						const auto& cr = _rows[j];
						if (cr.level >= cl) todo[cnt++] = j;
						else
							break;
					}

					break;
				}

				else parent = row.level;
			}

			if (cnt <= 0) break;
			else
			{
				for (int i = 0; i < cnt; ++i) _rows[todo[i]].level -= 1;
			}
		}
	}

	void Directyaml::parse(char *ymlstr, bool managed)
	{
		auto yml_s = ymlstr;
		auto yml_st = ymlstr;
		if(*yml_st == ' ') yml_st++;
		if(*yml_st == '\n') yml_st++;

		if( startwith(yml_st, "[") || startwith(yml_st, "{") ){
			_lastError = "Invalid yaml starting with '[ or {' at line: " + std::to_string(0);
			return;
		}
		if (managed)
		{
			_data = ymlstr;
			yml_s = &_data[0];
		}

		vector<kvSize> ss;
		ss.reserve(512);

		const auto l_count = split_ss(ss, yml_s);

		_rows.reserve(l_count);

		int lvMin = INT_MAX;
		int ln = 0;
		for (const auto& kv : ss)
		{
			ln++;
			if (kv.v <= 0) continue;

			auto sp = yml_s + kv.k;

			auto level = first_not(sp, ' ');
			if (level < 0) continue; // empty line

			sp += level; // skip leading spaces

			if (sp[0] == '#') continue;
			if (sp[0] == '%') continue; // %YAML, %TAG
			if (startwith(sp, "//")) continue;
			if (startwith(sp, "...")) continue;

			const char *key = sp;
			const char *val = nullptr;

			const auto startWithSingleDash = startwith(sp, "- ");

			// eat '-' (if it is array)
			if (startWithSingleDash)
			{
				sp += 2;
				key = sp;
				level += 2;
			}

			auto sep = strchr(sp, ':');
			// Seperation found and no next char or space
			if (sep && (!sep[1] || sep[1] == ' ')) {
				*((char*)sep) = 0;
				sp = sep + 1; // skip ':'
			}else if (!sep && startWithSingleDash && level==4){	// No ending :
				_lastError = "Missing ending ':' at line: " + std::to_string(ln-1) + ", node: " + std::string(sp);
				return;
			}else if (startWithSingleDash){	// Multi-elements array?
				key = nullptr;
			}

#ifdef __DYML_ALLOW_LINE_END_COMMENT__
			remove_line_end_comment(sp);
#endif

			// update val
			const auto be = trim(sp);
			if (be.v > 0)
			{
				((char*)sp)[be.start + be.count] = 0;
				val = sp + be.start;
			}

			level /= 2;

			lvMin = dyml_min(lvMin, level);

			_rows.emplace_back(level, key, val);
		}

		// stick to page left if not
		if (0 != lvMin)
		{
			for (auto& row : _rows) row.level -= lvMin;
		}

		// cache top levels
		const auto now = (int)_rows.size();

		_topLevels.reserve(now / 2);
		for (int i = 0; i < now; ++i)
		{
			if (0 == _rows[i].level) _topLevels.push_back(i);
		}

		if (!_rows.empty()) checkIndents();
	}
#if DEBUG_DYAML
	void print_yaml_rows(Directyaml& my, int width){
		char sn[0x20];
		const auto& rows = my.rows();
		const int cnt = (int)rows.size();
		for (int i = 0; i < cnt; ++i){
			const auto& row = rows[i];

			sprintf(sn, "#%d", i);
			printf("%5s lv=%2d %*s | %s\n"
				, sn
				, row.level
				, width
				, row.key ? row.key : " "
				, row.val ? row.val : " ")
				;
		}
	}

	void print_yaml_tree(Directyaml::Node node, int level){
		int noc = node.children();
		for (int i = 0; i < noc; ++i)	{
			// leading spaces
			printf("\n%s",  string((level + 1) * 2, ' ').c_str());

			// key-value
			auto ch = node.child(i);

			const auto k = (ch.key() == NULL) ? ""  :ch.key();
			const auto v = (ch.val() == NULL) ? ""  :ch.val();

			if(strlen(k)) printf("%s: ", k); // as key
			else
				printf("*\n"); // (key is null, print as array)

			if(strlen(v) && strcmp(k,v)!=0) printf("%s", v);
			print_yaml_tree(ch, level + 1);
		}
	}
#endif
}
