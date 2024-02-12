// Lightweight, fast, single header YAML parser for c++
// MIT License

#ifndef __DIRECT_YAML_H__
#define __DIRECT_YAML_H__
#include "String.h"
#include <vector>
#include <string>
#define DEBUG_DYAML false
using std::vector;
using std::string;

// 1. comments start from line begining is always supported
// 2. if guaranteed source yaml contains no line-end-comments, undefine this to parse even faster
//#define __DYML_ALLOW_LINE_END_COMMENT__

namespace dyml
{
	template <typename T>
	struct PairT
	{
		PairT() {}
		PairT(const T& _k, const T& _v) : k(_k), v(_v) {}
		union
		{
			struct { T k, v; };
			struct { T start, count; };
		};
	};

	typedef PairT<size_t> kvSize;

	class Directyaml
	{
	public:
		struct Row
		{
			Row() {}
			Row(int lv, const char* k, const char* v) : level(lv), key(k), val(v) {}
			int level;
			const char* key;
			const char* val;
		};

		class Node
		{
			friend class Directyaml;

		protected:
			Node() : _dyml(nullptr) {}
			Node(int row, int lv, const Directyaml* dy) : _row(row), _level(lv), _dyml(dy) {}

		public:
			bool valid() const { return nullptr != _dyml; }

			// find named child (checked version)
			Node child(const char* key) const
			{
				const auto row = _dyml->child(this, key);
				return row >= 0
					? Node(row, _level + 1, _dyml)
					: Node(-1, -1, nullptr)
					;
			}

			Node operator[](const char* key) const
			{
				const auto row = _dyml->child(this, key);
				return Node(row, _level + 1, _dyml);
			}

			// fast but unsafe
			Node operator[](int childIndex) const
			{
				return Node(_row + childIndex + 1, _level + 1, _dyml);
			}

			// safe but slow
			Node child(int childIndex) const
			{
				const auto row = _dyml->child(this, childIndex);
				return Node(row, _level + 1, _dyml);
			}

			const char* key() const // can be null
			{
				const auto& row = _dyml->_rows[_row];
				return row.key;
			}

			const char* val() const // can be null
			{
				const auto& row = _dyml->_rows[_row];
				return row.val;
			}
			int getRow() const { return _row; }
			int getLevel() const { return _level; }
			// direct child count
			int children() const
			{
				return _dyml->children(this);
			}

		protected:
			int _row;
			int _level;
			const Directyaml* _dyml;
		};

	public :
		Directyaml() {}
		Directyaml(char* ymlstr, bool managed = false)
		{
			_lastError = "";
			parse(ymlstr, managed);
		}

	public :
		bool managed() const { return !_data.empty(); }
		void shrink()
		{
			_rows.shrink_to_fit();
			_topLevels.shrink_to_fit();
		}

	protected:
		void checkIndents();

		kvSize trim(const char* s);
		int first_not(const char* sp, char ch);
		int child(const Node* parent, const char* key) const;
		int child(const Node* parent, int childIndex) const;
		int children(const Node* parent) const; // return direct child count

#ifdef __DYML_ALLOW_LINE_END_COMMENT__
		int char_count(const char* sp, char ch);
		void remove_line_end_comment(char* sp);
#endif

	public:
		void parse(char* ymlstr, bool managed = false);

		vector<Row>& rows() { return _rows; }
		const vector<Row>& rows() const { return _rows; }

		Node root() const
		{
			return Node(-1, -1, this);
		}

	protected:
		vector<Row> _rows;
		vector<int> _topLevels;
		string _data;
	public:	
		string _lastError;
	};
#if DEBUG_DYAML
	void print_yaml_rows(Directyaml& my, int width);
	void print_yaml_tree(Directyaml::Node node, int level);  
#endif
}
#endif // __DIRECT_YAML_H__