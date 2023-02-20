#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include "../stack_lng/polynomial.hpp"
#include "../stack_lng/polynomial.cpp"

// перечисление классов символьных лексем
//
enum SymbolicTokenClass { Letter, Digit, Dot, S_Comma, S_Semicol, S_Colon, Arithmetic, Compare, LRoundBracket, RRoundBracket,
                          SqBracket, Space, EndOfFileSymbol, Other };
const size_t SYMBOLIC_COUNT = Other + 1;

// структура "символьная лексема"
//
struct SymbolicToken {
	SymbolicTokenClass clazz;
	int value;

	void set(SymbolicTokenClass _class, int _value) { clazz = _class; value = _value; }
};

// перечисление классов лексем языка
//
enum LngTokenClass { Keyword, TypeKeyword, Identifier, Constant, Comma, Semicol, Colon, LBracket, RBracket, ArithmeticOperator, CompareOperator,
					 Assignment, Comment, LexError };

// перечисление ключевых слов
//
enum Keywords { Let, For, To, Adding, Next, If, Goto, Else, Load, Put, Switch, Case, Default, Break, Error, End };

// перечисление ключевых слов, обозначающих тип
//
enum TypeKeywords { Int, Pol };

// перечисление операторов сравнения
//
enum CompareOperators { Equal, NotEqual, Less, LessOrEqual, Bigger, BiggerOrEqual };

// перечисление типов данных языка
//
enum class Type { Integer, Polynomial };

//
// макросы для работы с указателями типа void*
//
// SWITCH_TYPE - оператор switch для выбора правильного типа объекта
// CDR - преобразование к нужному типу и разыменовывание (CDR --- cast-dereferense)
// CDRC_ALLOC - CDR + создание нового объекта в динамической памяти и присваивание указателя на него (CDRC --- cast-dereferense-copy)
//
#define SWITCH_TYPE(type_var, expr1, expr2) switch (type_var) { case Type::Integer: expr1 break; case Type::Polynomial: expr2 break; }
#define CDR(_type, ptr) (*(_type*)ptr)
#define CDRC_ALLOC(destination, source, _type) destination = new _type(*(_type*)source);

// структура "константа языка"
//
struct LngConstant {
	Type type;            // тип значения
	void* value_pointer;  // указатель на значение

	void set(Type _type, void* pointer) { type = _type, value_pointer = pointer; }

	~LngConstant() {
		if (!value_pointer) return;

		SWITCH_TYPE(type, delete (int*)value_pointer;, delete (Polynomial*)value_pointer;)
	}

	LngConstant(Type _type = Type::Integer, void* _ptr = nullptr) { type = _type; value_pointer = _ptr; }

	LngConstant(const LngConstant& other) {
		type = other.type;
		
		SWITCH_TYPE(type, CDRC_ALLOC(value_pointer, other.value_pointer, int), CDRC_ALLOC(value_pointer, other.value_pointer, Polynomial))
	}
	LngConstant(LngConstant&& other) {
		type = other.type;
		value_pointer = other.value_pointer;

		other.type = Type::Integer;
		other.value_pointer = nullptr;
	}

	LngConstant& operator =(const LngConstant& other) {
		if (this == &other) return *this;

		SWITCH_TYPE(type, CDRC_ALLOC(value_pointer, other.value_pointer, int), CDRC_ALLOC(value_pointer, other.value_pointer, Polynomial))

		return *this;
	}
	LngConstant& operator =(LngConstant&& other) {
		if (this == &other) return *this;

		type = other.type;
		value_pointer = other.value_pointer;

		other.type = Type::Integer;
		other.value_pointer = nullptr;

		return *this;
	}
};

std::ostream& operator <<(std::ostream& os, const LngConstant& Const) {
	SWITCH_TYPE(Const.type, os << CDR(int, Const.value_pointer);, os << CDR(Polynomial, Const.value_pointer);)
	return os;
}

// структура "лексема языка"
//
struct LngToken {
	LngTokenClass clazz;
	int value;
	size_t line;

	void set(LngTokenClass _class, int _value, size_t _line) {
		clazz = _class; value = _value; line = _line;
	}
};

// транслитератор: по символу строит его символьную лексему
//
SymbolicToken transliterator(char raw_char) {
	SymbolicToken st;
	if ('a' <= raw_char && raw_char <= 'z' ||
		'A' <= raw_char && raw_char <= 'Z')
	{
		st.set(Letter, raw_char);
	}
	else if ('0' <= raw_char && raw_char <= '9') {
		st.set(Digit, raw_char - '0');
	}
	else if (raw_char == '.') {
		st.set(Dot, 0);
	}
	else if (raw_char == ',') {
		st.set(S_Comma, 0);
	}
	else if (raw_char == ';') {
		st.set(S_Semicol, 0);
	}
	else if (raw_char == ':') {
		st.set(S_Colon, 0);
	}
	else if (raw_char == '+' || raw_char == '-' ||
		     raw_char == '*' || raw_char == '/' ||
		     raw_char == '%') 
	{
		st.set(Arithmetic, raw_char);
	}
	else if (raw_char == '=' || raw_char == '<' ||
		     raw_char == '>' || raw_char == '!')
	{
		st.set(Compare, raw_char);
	}
	else if (raw_char == '(') {
		st.set(LRoundBracket, 0);
	}
	else if (raw_char == ')') {
		st.set(RRoundBracket, 0);
	}
	else if (raw_char == '[' || raw_char == ']') {
		st.set(SqBracket, raw_char);
	}
	else if (raw_char == '\n' || raw_char == '\t' ||
		     raw_char == ' ')
	{
		st.set(Space, raw_char);
	}
	else if (raw_char == EOF) {
		st.set(EndOfFileSymbol, 0);
	}
	else
		st.set(Other, 0);

	return st;
}

// класс "обработання программа"
class ParsedProgram {
private:
	const char* m_name;                        // имя файла программы
	std::vector<LngToken> m_token_table;       // вектор лексем
	std::vector<std::string> m_id_table;       // вектор переменных
	std::vector<LngConstant> m_constant_table; // вектор констант
	bool is_wrong;
public:
	ParsedProgram() { m_name = nullptr; }
	/*ParsedProgram(const ParsedProgram& other) {
		m_name = other.m_name;
		m_token_table = other.m_token_table;
		m_id_table = other.m_id_table;
		m_constant_table = other.m_constant_table;
	}
	ParsedProgram(ParsedProgram&& other) {
		m_name = other.m_name;
		other.m_name = nullptr;

		m_token_table = std::move(other.m_token_table);
		m_id_table = std::move(other.m_id_table);
		m_constant_table = std::move(other.m_constant_table);
	}

	ParsedProgram& operator =(const ParsedProgram& other) {
		if (this == &other) return *this;

		m_name = other.m_name;
		m_token_table = other.m_token_table;
		m_id_table = other.m_id_table;
		m_constant_table = other.m_constant_table;

		return *this;
	}
	ParsedProgram& operator =(ParsedProgram&& other) {
		if (this == &other) return *this;

		m_name = other.m_name;
		other.m_name = nullptr;

		m_token_table = std::move(other.m_token_table);
		m_id_table = std::move(other.m_id_table);
		m_constant_table = std::move(other.m_constant_table);

		return *this;
	}*/

	void rename(const char* filename) {
		m_name = filename;
	}

	void add_token(const LngToken& token) {
		m_token_table.push_back(token);
	}

	int add_identifier(std::string& identifier) {
		int index = 0;
		for (auto& id : m_id_table) {
			if (id == identifier) return index;
			++index;
		}

		m_id_table.push_back(identifier);
		return m_id_table.size() - 1;
	}

	int add_int_constant(int number) {
		int index = 0;
		for (auto& Const : m_constant_table) {
			SWITCH_TYPE(Const.type, if (number == CDR(int, Const.value_pointer)) return index;, )
			++index;
		}

		m_constant_table.push_back(LngConstant(Type::Integer, new int(number)));
		return m_constant_table.size() - 1;
	}

	int add_pol_constant(const Polynomial& polynomial) {
		int index = 0;
		for (auto& Const : m_constant_table) {
			SWITCH_TYPE(Const.type, , if (polynomial == CDR(Polynomial, Const.value_pointer)) return index;)
			++index;
		}

		m_constant_table.push_back(LngConstant(Type::Polynomial, new Polynomial(polynomial)));
		return m_constant_table.size() - 1;
	}

	void set_error_flag() {
		is_wrong = true;
	}

	void print_tokens(std::ostream& os = std::cout) {
		int current_line = 0;
		for (auto& token : m_token_table) {
			if (token.line != current_line) {
				os << '\n' << token.line << ":  ";
				current_line = token.line;
			}

		    switch (token.clazz) {
	        case Keyword: switch (token.value) {
                          case Let:     os << "let"; break;
                          case For:     os << "for"; break;
                          case To:      os << "to"; break;
                          case Adding:  os << "adding"; break;
                          case Next:    os << "next"; break;
                          case If:      os << "if"; break;
                          case Goto:    os << "goto"; break;
                          case Else:    os << "else"; break;
                          case Load:    os << "load"; break;
                          case Put:     os << "put"; break;
                          case Switch:  os << "switch"; break;
                          case Case:    os << "case"; break;
                          case Default: os << "default"; break;
                          case Break:   os << "break"; break;
                          case Error:   os << "error"; break;
                          case End:     os << "end"; break;
	                      }
	                      break;
	        case TypeKeyword: switch (token.value) {
                              case Int: os << "int"; break;
                              case Pol: os << "pol"; break;
	                          } break;
	        case Identifier: os << "<V " << token.value << ": " << m_id_table[token.value] << ">"; break;
	        case Constant:   os << "<C " << token.value << ": " << m_constant_table[token.value] << ">"; break;

	        case Comma:      os << ","; break;
	        case Semicol:    os << ";"; break;
	        case Colon:      os << ":"; break;
	        case LBracket:   os << "("; break;
	        case RBracket:   os << ")"; break;

	        case ArithmeticOperator: os << (char)token.value; break;
	        case CompareOperator: switch(token.value) {
                                  case NotEqual:      os << "!="; break;
                                  case Equal:         os << "=="; break;
                                  case Less:          os << "<"; break;
                                  case LessOrEqual:   os << "<="; break;
                                  case Bigger:        os << ">"; break;
                                  case BiggerOrEqual: os << ">="; break;
	                              }
	                              break;
	        case Assignment: os << "="; break;
	        case Comment:    os << "(Комментарий)"; break;
	        case LexError:   os << "(Ошибка)"; break;
	        }
	        os << " ";
		}
		os << std::endl;
	}

	void print_id_table(std::ostream& os = std::cout) {
		int index = 0;
		os << "Таблица идентификаторов:\n";
		for (auto& id : m_id_table) {
			os << index++ << ". " << id << '\n';
		}
	}

	void print_constant_table(std::ostream& os = std::cout) {
		int index = 0;
		os << "Таблица констант:\n";
		for (auto& Const : m_constant_table) {
			os << index++ << ". " << Const << '\n';
		}
	}

	void print_lines_with_errors(std::ostream& os = std::cout) {
		if (!is_wrong) return;

		os << "Ошибки в строках:\n";
		int last_printed_line = 0;
		for (auto& token : m_token_table) {
			if (token.clazz == LexError) {
				if (last_printed_line == token.line) continue;

				os << token.line << '\n';
				last_printed_line = token.line;
			}
		}
	}
};

// перечисление состояний
//
enum State { s_A1,
             s_B1, s_B2, s_B3,
             s_C1,
             s_D1, s_D2, s_D3, s_D4, s_D5,
             s_E1,
             s_F1, s_F2, s_F3,
             s_G1, s_G2, s_G3, s_G4,
             s_Err1,
             s_Stop 
            };
const size_t STATE_COUNT = s_Stop;

//
// необходимые регистры
//
ParsedProgram r_program;        // обработанная программа

State r_state;                  // текущее состояние автомата

SymbolicToken r_symbolic_token; // очередная символьная лексема

LngToken r_token;
LngTokenClass r_token_class;    // класс лексемы языка
int r_token_value;              // значение лексемы языка
int r_line_number;              // номер строки

std::string r_string;           // регистр строки

int r_detection_index;          // индекс в таблице обнаружений
int r_id_table_index;           // индекс в таблице имён
int r_constant_table_index;     // индекс в таблице констант

int r_number;                   // регистр числа

char r_cmp_symbol1;             // регистр сравнения 1
char r_cmp_symbol2;             // регистр сравнения 2

Polynomial r_polynomial;        // регистр многочлена
int r_sign;                     // регистр знака
int r_int_part;                 // регистр целой части
float r_fract_part;             // регситр дробной части
int r_fract_count;              // регситр порядка

using Procedure = State (*)();
  
// структура "строка в таблице обнаружений"
struct DetectionTableLine {
	char letter;         // буква
	int alt;             // альтернатива
	Procedure procedure; // процедура
};

std::ostream& operator <<(std::ostream& os, const DetectionTableLine& DTL) {
	os << DTL.letter << ' ' << DTL.alt << ' ' << (void*)DTL.procedure;
	return os;
}

// таблица обнаружений
#define DT_SIZE 53
DetectionTableLine detection_table[DT_SIZE] = { 0 };
int init_vector[26];

//
// процедуры автомата
//
#define ADD_TOKEN(clazz, value) r_token.set(clazz, value, r_line_number); r_program.add_token(r_token);

State ERR() {
	r_program.set_error_flag();
	ADD_TOKEN(LexError, 0)
	return s_Err1;
}

State A1a() {
	ADD_TOKEN(Comma, 0)
	return s_A1;
}

State A1b() {
	ADD_TOKEN(Semicol, 0)
	return s_A1;
}

State A1c() {
	ADD_TOKEN(Colon, 0)
	return s_A1;
}

State A1d() {
	ADD_TOKEN(ArithmeticOperator, r_symbolic_token.value)
	return s_A1;
}

State A1e() {
	ADD_TOKEN(LBracket, 0)
	return s_A1;
}

State A1f() {
	ADD_TOKEN(RBracket, 0)
	return s_A1;
}

State A1g() {
	if (r_symbolic_token.value == '\n') ++r_line_number;
	return s_A1;
}

State A1h() {
	ADD_TOKEN(Comment, 0)
	return s_A1;
}

State B1a() {
	r_string = r_symbolic_token.value;
	if ('a' <= r_symbolic_token.value && r_symbolic_token.value <= 'z') r_detection_index = init_vector[r_symbolic_token.value - 'a'];
	else return s_B2;

	return s_B1;
}

State B1b() {
	++r_detection_index;
	return s_B1;
}


State B2a() {
	if (r_symbolic_token.value < 10) r_string += ( r_symbolic_token.value + '0' );
	else r_string += r_symbolic_token.value;
	return s_B2;
}

#define SET_KEYWORD(kw) r_token_class = Keyword; r_token_value = kw;
#define SET_TYPE_KEYWORD(kw) r_token_class = TypeKeyword; r_token_value = kw;

State B3a() {
	SET_KEYWORD(Let)
	return s_B3;
}

State B3b() {
	SET_KEYWORD(For)
	return s_B3;
}

State B3c() {
	SET_KEYWORD(To)
	return s_B3;
}

State B3d() {
	SET_KEYWORD(Adding)
	return s_B3;
}

State B3e() {
	SET_KEYWORD(Next)
	return s_B3;
}

State B3f() {
	SET_KEYWORD(If)
	return s_B3;
}

State B3g() {
	SET_KEYWORD(Goto)
	return s_B3;
}

State B3h() {
	SET_KEYWORD(Else)
	return s_B3;
}

State B3i() {
	SET_KEYWORD(Load)
	return s_B3;
}

State B3j() {
	SET_KEYWORD(Put)
	return s_B3;
}

State B3k() {
	SET_KEYWORD(Switch)
	return s_B3;
}

State B3l() {
	SET_KEYWORD(Case)
	return s_B3;
}

State B3m() {
	SET_KEYWORD(Default)
	return s_B3;
}

State B3n() {
	SET_KEYWORD(Break)
	return s_B3;
}

State B3o() {
	SET_KEYWORD(Error)
	return s_B3;
}

State B3p() {
	SET_TYPE_KEYWORD(Int)
	return s_B3;
}

State B3q() {
	SET_TYPE_KEYWORD(Pol)
	return s_B3;
}

State B3r() {
	SET_KEYWORD(End)
	return s_B3;
}

State C1a() {
	r_number = r_symbolic_token.value;
	return s_C1;
}

State C1b() {
	r_number = 10 * r_number + r_symbolic_token.value;
	return s_C1;
}

State D1a() {
	r_cmp_symbol1 = r_symbolic_token.value;
	return s_D1;
}

State D2a() {
	r_cmp_symbol2 = r_symbolic_token.value;
	return s_D2;
}

State D3() {
	return s_D3;
}

State D3a() {
	if (r_symbolic_token.value == '\n') ++r_line_number;
	return s_D3;
}

State D4a() {
	if (r_symbolic_token.value == '>') return s_D4;
	return s_D3; 
}

State D5a() {
	if (r_symbolic_token.value == '>') return s_D5;
	return s_D3; 
}

State E1a() {
	if (r_symbolic_token.value == ']') return ERR();
	
	r_polynomial.clear();
	return s_E1;
}

State E1bg() {
	for (int i = 0; i < r_fract_count; ++i) r_fract_part /= 10;
	r_polynomial = r_polynomial + Polynomial(r_number, r_sign * (r_int_part + r_fract_part));

	if (r_symbolic_token.value == '\n') ++r_line_number;
	return s_E1;
}

State E1g() {
	if (r_symbolic_token.value == '\n') ++r_line_number;
	return s_E1;
}

State F1a() {
	switch (r_symbolic_token.value) {
	case '+': r_sign = 1;  break;
	case '-': r_sign = -1; break;
	default: return ERR();
	}

	return s_F1;
}

State F1b() {
	for (int i = 0; i < r_fract_count; ++i) r_fract_part /= 10;
	r_polynomial = r_polynomial + Polynomial(r_number, r_sign * (r_int_part + r_fract_part));

	return F1a();
}

State F1g() {
	if (r_symbolic_token.value == '\n') ++r_line_number;
	return s_F1;
}

State F2a() {
	r_number = r_symbolic_token.value;
	return s_F2;
}

State F2b() {
	r_number = 10 * r_number + r_symbolic_token.value;
	return s_F2;
}

State F3g() {
	if (r_symbolic_token.value == '\n') ++r_line_number;
	return s_F3;
}

State G1a() {
	r_int_part = r_fract_part = r_fract_count = 0;
	return s_G1;
}

State G1g() {
	if (r_symbolic_token.value == '\n') ++r_line_number;
	return s_G1;
}

State G2a() {
	r_int_part = r_symbolic_token.value;
	return s_G2;
}

State G2b() {
	r_int_part = 10 * r_int_part + r_symbolic_token.value;
	return s_G2;
}

State G3() {
	return s_G3;
}

State G3a() {
	r_fract_part = 10 * r_fract_part + r_symbolic_token.value;
	++r_fract_count;
	return s_G3;
}

State G3b() {
	r_int_part = 0;
	r_fract_part = r_symbolic_token.value;
	r_fract_count = 1;

	return s_G3;
}

State G4() {
	return s_G4;
}

State M1() {
	r_string += r_symbolic_token.value;
	while (true) {
		if (r_detection_index == -1) return s_B2;
		if (r_symbolic_token.value == detection_table[r_detection_index].letter) return detection_table[r_detection_index].procedure();
		r_detection_index = detection_table[r_detection_index].alt;
	}
}

State EXIT() {
	return s_Stop;
}

State ERREXIT() {
	ERR();
	return EXIT();
}

State ERR1() {
	return s_Err1;
}

State P1() {
	if (r_symbolic_token.value == '[') { ERR(); return E1a(); }

	r_constant_table_index = r_program.add_pol_constant(r_polynomial);
	ADD_TOKEN(Constant, r_constant_table_index)

	return s_A1;
}

State P2() {
	for (int i = 0; i < r_fract_count; ++i) r_fract_part /= 10;
	r_polynomial = r_polynomial + Polynomial(r_number, r_sign * (r_int_part + r_fract_part));

	return P1(); 
}

void N() {
	r_constant_table_index = r_program.add_int_constant(r_number);
	ADD_TOKEN(Constant, r_constant_table_index)
}

State NA1a() {
	N();
	return A1a();
}

State NA1b() {
	N();
	return A1b();
}

State NA1c() {
	N();
	return A1c();
}

State NA1d() {
	N();
	return A1d();
}

State NA1e() {
	N();
	return A1e();
}

State NA1f() {
	N();
	return A1f();
}

State NA1g() {
	N();
	return A1g();
}

State ND1a() {
	N();
	return D1a();
}

State NEXIT() {
	N();
	return EXIT();
}

State NERR() {
	N();
	return ERR();
}

State NERRE1a() {
	N();
	ERR();
	return E1a();
}

void CM1() {
	switch (r_cmp_symbol1) {
	case '!': ADD_TOKEN(LexError, 0) break;
	case '<': ADD_TOKEN(CompareOperator, Less) break;
	case '>': ADD_TOKEN(CompareOperator, Bigger) break;
	case '=': ADD_TOKEN(Assignment, 0) break;
	}
}

State CM1A1a() {
	CM1();
	return A1a();
}

State CM1A1b() {
	CM1();
	return A1b();
}

State CM1A1c() {
	CM1();
	return A1c();
}

State CM1A1d() {
	CM1();
	return A1d();
}

State CM1A1e() {
	CM1();
	return A1e();
}

State CM1A1f() {
	CM1();
	return A1f();
}

State CM1A1g() {
	CM1();
	return A1g();
}

State CM1B1a() {
	CM1();
	return B1a();
}

State CM1C1a() {
	CM1();
	return C1a();
}

State CM1EXIT() {
	CM1();
	return EXIT();
}

State CM1ERR() {
	CM1();
	return ERR();
}

State CM1E1a() {
	CM1();
	return E1a();
}

#define IS_RELATION(part1, part2) (r_cmp_symbol1 == part1 && r_cmp_symbol2 == part2)

void CM2() {
	if      (IS_RELATION('!', '=')) { ADD_TOKEN(CompareOperator, NotEqual) }
	else if (IS_RELATION('=', '=')) { ADD_TOKEN(CompareOperator, Equal) }
	else if (IS_RELATION('<', '=')) { ADD_TOKEN(CompareOperator, LessOrEqual) }
	else if (IS_RELATION('>', '=')) { ADD_TOKEN(CompareOperator, BiggerOrEqual) }
	else { ADD_TOKEN(LexError, 0) }
}

State CM2A1a() {
	CM2();
	return A1a();
}

State CM2A1b() {
	CM2();
	return A1b();
}

State CM2A1c() {
	CM2();
	return A1c();
}

State CM2A1d() {
	CM2();
	return A1d();
}

State CM2A1e() {
	CM2();
	return A1e();
}

State CM2A1f() {
	CM2();
	return A1f();
}

State CM2A1g() {
	CM2();
	return A1g();
}

State CM2B1a() {
	CM2();
	return B1a();
}

State CM2C1a() {
	CM2();
	return C1a();
}

State CM2EXIT() {
	CM2();
	return EXIT();
}

State CM2ERR() {
	CM2();
	return ERR();
}

State CM2E1a() {
	CM2();
	return E1a();
}

State CM3() {
	if (r_cmp_symbol1 == '<' && r_cmp_symbol2 == '<' && r_symbolic_token.value == '<') return s_D3;

	return ERR();
}

void KW() {
	ADD_TOKEN(r_token_class, r_token_value)
}

State KWA1a() {
	KW();
	return A1a();
}

State KWA1b() {
	KW();
	return A1b();
}

State KWA1c() {
	KW();
	return A1c();
}

State KWA1d() {
	KW();
	return A1d();
}

State KWA1e() {
	KW();
	return A1e();
}

State KWA1f() {
	KW();
	return A1f();
}

State KWA1g() {
	KW();
	return A1g();
}

State KWD1a() {
	KW();
	return D1a();
}

State KWEXIT() {
	KW();
	return EXIT();
}

State KWERR() {
	KW();
	return ERR();
}

State KWERRE1a() {
	KW();
	ERR();
	return E1a();
}

void W() {
	r_id_table_index = r_program.add_identifier(r_string);
	ADD_TOKEN(Identifier, r_id_table_index)
}

State WA1a() {
	W();
	return A1a();
}

State WA1b() {
	W();
	return A1b();
}

State WA1c() {
	W();
	return A1c();
}

State WA1d() {
	W();
	return A1d();
}

State WA1e() {
	W();
	return A1e();
}

State WA1f() {
	W();
	return A1f();
}

State WA1g() {
	W();
	return A1g();
}

State WD1a() {
	W();
	return D1a();
}

State WEXIT() {
	W();
	return EXIT();
}

State WERR() {
	W();
	return ERR();
}

State WERRE1a() {
	W();
	ERR();
	return E1a();
}

// загрузка букв в таблицу обнаружений;
// таже заполняет поля alt и procedure значениями по умолчанию (-1, B1b)
//
inline void load_detection_table() {
	std::ifstream in("keywords");

	if (!in) {
		std::cout << "Failed to open DT_data file...! Assuming detection table is empty...\n";
		for (int i = 0; i < DT_SIZE; ++i) detection_table[i].alt = -1;
		return;
	}

	size_t DT_index = 0; bool begin_word = true;
	while (in && DT_index < DT_SIZE) {
		char ch = in.get();

		if (ch == '\n') { begin_word = true;  continue; }
		if (begin_word) { begin_word = false; continue; }

		detection_table[DT_index].letter = ch;
		detection_table[DT_index].alt = -1;
		detection_table[DT_index].procedure = B1b;
		++DT_index;
	}
}

// дополнение таблицы обнаружений альтернативами и процедурами
//
inline void complete_detection_table() {
	// добавление альтернатив
	detection_table[0].alt = 20;
	detection_table[13].alt = 47;
	detection_table[17].alt = 43;
	detection_table[23].alt = 49;
	detection_table[43].alt = 51;

	detection_table[1].procedure = B3a;  // let
	detection_table[3].procedure = B3b;  // for
	detection_table[4].procedure = B3c;  // to
	detection_table[9].procedure = B3d;  // adding
	detection_table[12].procedure = B3e; // next
	detection_table[13].procedure = B3f; // if
	detection_table[16].procedure = B3g; // goto
	detection_table[19].procedure = B3h; // else
	detection_table[22].procedure = B3i; // load
	detection_table[24].procedure = B3j; // put
	detection_table[29].procedure = B3k; // switch
	detection_table[32].procedure = B3l; // case
	detection_table[38].procedure = B3m; // default
	detection_table[42].procedure = B3n; // break
	detection_table[46].procedure = B3o; // error
	detection_table[48].procedure = B3p; // int
	detection_table[50].procedure = B3q; // pol
	detection_table[52].procedure = B3r; // end
}

inline void fill_init_vector() {
	for (int i = 0; i < 26; ++i) {
		init_vector[i] = -1;
	}
	init_vector['a' - 'a'] = 5;
	init_vector['b' - 'a'] = 39;
	init_vector['c' - 'a'] = 30;
	init_vector['d' - 'a'] = 33;
	init_vector['e' - 'a'] = 17;
	init_vector['f' - 'a'] = 2;
	init_vector['g' - 'a'] = 14;
	// h
	init_vector['i' - 'a'] = 13;
	// j
	// k
	init_vector['l' - 'a'] = 0;
	// m
	init_vector['n' - 'a'] = 10;
	// o
	init_vector['p' - 'a'] = 23;
	// q
	// r
	init_vector['s' - 'a'] = 25;
	init_vector['t' - 'a'] = 4;
	// u
	// v
	// w
	// x
	// y
	// z
}

// заполнение таблицы процедур
//
Procedure procedure_table[STATE_COUNT][SYMBOLIC_COUNT];

#define PTSET(state, st_class, procedure) procedure_table[state][st_class] = procedure;

inline void fill_procedure_table() {
	for (int i = 0; i < STATE_COUNT; ++i) {
		for (int j = 0; j < SYMBOLIC_COUNT; ++j) {
			PTSET(i, j, ERR)
		}
	}
	PTSET(s_A1, Letter, B1a)    PTSET(s_A1, Digit, C1a)    /*PTSET(s_A1, Dot, )*/   PTSET(s_A1, S_Comma, A1a)    PTSET(s_A1, S_Semicol, A1b)
	PTSET(s_B1, Letter, M1)     PTSET(s_B1, Digit, B2a)    PTSET(s_B1, Dot, WERR)   PTSET(s_B1, S_Comma, WA1a)   PTSET(s_B1, S_Semicol, WA1b)
	PTSET(s_B2, Letter, B2a)    PTSET(s_B2, Digit, B2a)    PTSET(s_B2, Dot, WERR)   PTSET(s_B2, S_Comma, WA1a)   PTSET(s_B2, S_Semicol, WA1b)
	PTSET(s_B3, Letter, B2a)    PTSET(s_B3, Digit, B2a)    PTSET(s_B3, Dot, KWERR)  PTSET(s_B3, S_Comma, KWA1a)  PTSET(s_B3, S_Semicol, KWA1b)
	/*PTSET(s_C1, Letter, )*/   PTSET(s_C1, Digit, C1b)    PTSET(s_C1, Dot, NERR)   PTSET(s_C1, S_Comma, NA1a)   PTSET(s_C1, S_Semicol, NA1b)
	PTSET(s_D1, Letter, CM1B1a) PTSET(s_D1, Digit, CM1C1a) PTSET(s_D1, Dot, CM1ERR) PTSET(s_D1, S_Comma, CM1A1a) PTSET(s_D1, S_Semicol, CM1A1b)
	PTSET(s_D2, Letter, CM2B1a) PTSET(s_D2, Digit, CM2C1a) PTSET(s_D2, Dot, CM2ERR) PTSET(s_D2, S_Comma, CM2A1a) PTSET(s_D2, S_Semicol, CM2A1b)

	PTSET(s_A1, S_Colon, A1c)    PTSET(s_A1, Arithmetic, A1d)    PTSET(s_A1, Compare, D1a)   PTSET(s_A1, LRoundBracket, A1e)    PTSET(s_A1, RRoundBracket, A1f)
	PTSET(s_B1, S_Colon, WA1c)   PTSET(s_B1, Arithmetic, WA1d)   PTSET(s_B1, Compare, WD1a)  PTSET(s_B1, LRoundBracket, WA1e)   PTSET(s_B1, RRoundBracket, WA1f)
	PTSET(s_B2, S_Colon, WA1c)   PTSET(s_B2, Arithmetic, WA1d)   PTSET(s_B2, Compare, WD1a)  PTSET(s_B2, LRoundBracket, WA1e)   PTSET(s_B2, RRoundBracket, WA1f)
	PTSET(s_B3, S_Colon, KWA1c)  PTSET(s_B3, Arithmetic, KWA1d)  PTSET(s_B3, Compare, KWD1a) PTSET(s_B3, LRoundBracket, KWA1e)  PTSET(s_B3, RRoundBracket, KWA1f)
	PTSET(s_C1, S_Colon, NA1c)   PTSET(s_C1, Arithmetic, NA1d)   PTSET(s_C1, Compare, ND1a)  PTSET(s_C1, LRoundBracket, NA1e)   PTSET(s_C1, RRoundBracket, NA1f)
	PTSET(s_D1, S_Colon, CM1A1c) PTSET(s_D1, Arithmetic, CM1A1d) PTSET(s_D1, Compare, D2a)   PTSET(s_D1, LRoundBracket, CM1A1e) PTSET(s_D1, RRoundBracket, CM1A1f)
	PTSET(s_D2, S_Colon, CM2A1c) PTSET(s_D2, Arithmetic, CM2A1d) PTSET(s_D2, Compare, CM3)   PTSET(s_D2, LRoundBracket, CM2A1e) PTSET(s_D2, RRoundBracket, CM2A1f)

	PTSET(s_A1, SqBracket, E1a)      PTSET(s_A1, Space, A1g)    PTSET(s_A1, EndOfFileSymbol, EXIT)    /*PTSET(s_A1, Other, )*/
	PTSET(s_B1, SqBracket, WERRE1a)  PTSET(s_B1, Space, WA1g)   PTSET(s_B1, EndOfFileSymbol, WEXIT)   /*PTSET(s_B1, Other, )*/
	PTSET(s_B2, SqBracket, WERRE1a)  PTSET(s_B2, Space, WA1g)   PTSET(s_B2, EndOfFileSymbol, WEXIT)   /*PTSET(s_B2, Other, )*/
	PTSET(s_B3, SqBracket, KWERRE1a) PTSET(s_B3, Space, KWA1g)  PTSET(s_B3, EndOfFileSymbol, KWEXIT)  /*PTSET(s_B3, Other, )*/
	PTSET(s_C1, SqBracket, NERRE1a)  PTSET(s_C1, Space, NA1g)   PTSET(s_C1, EndOfFileSymbol, NEXIT)   /*PTSET(s_C1, Other, )*/
	PTSET(s_D1, SqBracket, CM1E1a)   PTSET(s_D1, Space, CM1A1g) PTSET(s_D1, EndOfFileSymbol, CM1EXIT) PTSET(s_D1, Other, CM1ERR)
	PTSET(s_D2, SqBracket, CM2E1a)   PTSET(s_D2, Space, CM2A1g) PTSET(s_D2, EndOfFileSymbol, CM2EXIT) PTSET(s_D2, Other, CM2ERR)

	for (int i = s_D3; i <= s_D5; ++i) {
		for (int j = 0; j < SYMBOLIC_COUNT; ++j) {
			PTSET(i, j, D3)
		}
	}

	PTSET(s_D3, Compare, D4a) PTSET(s_D3, Space, D3a) PTSET(s_D3, EndOfFileSymbol, ERREXIT) 
	PTSET(s_D4, Compare, D5a) PTSET(s_D4, Space, D3a) PTSET(s_D4, EndOfFileSymbol, ERREXIT)
	PTSET(s_D5, Compare, A1h) PTSET(s_D5, Space, D3a) PTSET(s_D5, EndOfFileSymbol, ERREXIT)

	/*PTSET(s_E1, Digit, )*/ /*PTSET(s_E1, Dot, )*/ /*PTSET(s_E1, S_Colon, )*/ PTSET(s_E1, Arithmetic, F1a)  PTSET(s_E1, SqBracket, P1)
	PTSET(s_F1, Digit, F2a)  /*PTSET(s_F1, Dot, )*/ /*PTSET(s_F1, S_Colon, )*/ /*PTSET(s_F1, Arithmetic, )*/ /*PTSET(s_F1, SqBracket, )*/
	PTSET(s_F2, Digit, F2b)  /*PTSET(s_F2, Dot, )*/ PTSET(s_F2, S_Colon, G1a)  /*PTSET(s_F2, Arithmetic, )*/ /*PTSET(s_F2, SqBracket, )*/
	/*PTSET(s_F3, Digit, )*/ /*PTSET(s_F3, Dot, )*/ PTSET(s_F3, S_Colon, G1a)  /*PTSET(s_F3, Arithmetic, )*/ /*PTSET(s_F3, SqBracket, )*/
	PTSET(s_G1, Digit, G2a)  PTSET(s_G1, Dot, G4)   /*PTSET(s_G1, S_Colon, )*/ /*PTSET(s_G1, Arithmetic, )*/ /*PTSET(s_G1, SqBracket, )*/
	PTSET(s_G2, Digit, G2b)  PTSET(s_G2, Dot, G3)   /*PTSET(s_G2, S_Colon, )*/ PTSET(s_G2, Arithmetic, F1b)  PTSET(s_G2, SqBracket, P2)
 	PTSET(s_G3, Digit, G3a)  /*PTSET(s_G3, Dot, )*/ /*PTSET(s_G3, S_Colon, )*/ PTSET(s_G3, Arithmetic, F1b)  PTSET(s_G3, SqBracket, P2)
	PTSET(s_G4, Digit, G3b)  /*PTSET(s_G4, Dot, )*/ /*PTSET(s_G4, S_Colon, )*/ /*PTSET(s_G4, Arithmetic, )*/ /*PTSET(s_G4, SqBracket, )*/

	PTSET(s_E1, Space, E1g)  PTSET(s_E1, EndOfFileSymbol, ERREXIT)
	PTSET(s_F1, Space, F1g)  PTSET(s_F1, EndOfFileSymbol, ERREXIT)
	PTSET(s_F2, Space, F3g)  PTSET(s_F2, EndOfFileSymbol, ERREXIT)
	PTSET(s_F3, Space, F3g)  PTSET(s_F3, EndOfFileSymbol, ERREXIT)
	PTSET(s_G1, Space, G1g)  PTSET(s_G1, EndOfFileSymbol, ERREXIT)  
	PTSET(s_G2, Space, E1bg) PTSET(s_G2, EndOfFileSymbol, ERREXIT)  
 	PTSET(s_G3, Space, E1bg) PTSET(s_G3, EndOfFileSymbol, ERREXIT)
	/*PTSET(s_G4, Space, )*/ PTSET(s_G4, EndOfFileSymbol, ERREXIT)

	for (int i = 0; i < SYMBOLIC_COUNT; ++i) {
		PTSET(s_Err1, i, ERR1)
	}

	PTSET(s_Err1, Space, A1g)
	PTSET(s_Err1, EndOfFileSymbol, EXIT)

	/*PTSET(s_Err1, Letter, ERR1)
	PTSET(s_Err1, Digit, ERR1)
	PTSET(s_Err1, Dot, ERR1)
	PTSET(s_Err1, S_Comma, A1a)
	PTSET(s_Err1, S_Semicol, A1b)
	PTSET(s_Err1, S_Colon, A1c)
	PTSET(s_Err1, Arithmetic, A1d)
	PTSET(s_Err1, Compare, D1a)
	PTSET(s_Err1, LRoundBracket, A1e)
	PTSET(s_Err1, RRoundBracket, A1f)
	PTSET(s_Err1, SqBracket, E1a)
	PTSET(s_Err1, Space, A1g)
	PTSET(s_Err1, EndOfFileSymbol, EXIT)
	PTSET(s_Err1, Other, ERR1)*/
}

// начальная настройка лексического анализатора
//
inline void setup_lexer() {
	load_detection_table();
	complete_detection_table();
	fill_init_vector();

	fill_procedure_table();

	r_state = s_A1;
	r_line_number = 1;
}

// запуск лексического анализатора
//
void run_lexer(const char* filename) {
	std::ifstream in(filename);

	if (!in) {
		std::cout << "Failed to open input file...!\n";
		return;
	}

	setup_lexer();
	r_program.rename(filename);

	while (r_state != s_Stop) {
		char symbol = in.get();
		r_symbolic_token = transliterator(symbol);
		r_state = procedure_table[r_state][r_symbolic_token.clazz]();
	}
}

int main() {
	run_lexer("input1");

	std::ofstream out("tokens.old");
	r_program.print_tokens(out);
	out << "\n\n\n";
	r_program.print_id_table(out);
	out << "\n\n\n";
	r_program.print_constant_table(out);
	r_program.print_lines_with_errors();

	out.close();
	return 0;
}