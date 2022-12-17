#include <iostream>
#include <fstream>
#include <string>
#include <vector>

/* перечисление классов символьных лексем :
* буква, цифра, арифметическая операция, операция сравнения, пробел, \n, ;, конец, ошибка */
enum SymbolicTokenClass { Letter, Digit, Arithmetic, Cmp, Space, LF, SemiCol, EndOfFileSymbol, Other,
						  /* [, ], точка, : */
						  LSqBracket, RSqBracket, Dot, Colon,
						 };
const int SYMBOL_TOKEN_CLASS_COUNT = 13;  // количество символьных лексем

/* структура "символьная лексема" */
struct SymbolicToken {
	SymbolicTokenClass token_class; 	// класс лексемы
	int 			   value;			// значение лексемы
};

/* перечисление классов лексем стекового языка */
enum TokenClass { Push, Pop, Jmp, Ji, Read, Write, End,	// ключевые слова : push, pop, jmp, ji, read, write, end
				  ArithmeticOp, CmpOp,					// арифметическая операция, операция сравнения
				  Atpow, Deg, Derivative, Value,        // ключевые слова : atpow, deg, derivative, value
				  Comment, Error, EndOfFile 			// комментарий, ошибка, конец файла
				};

/* перечисление операций (и отдельных символов) сравнения 
* ! , = , != , < , <= , >, >=
* 
* ВАЖНО, ЧТО Negation + Equal = NotEqual ; Less + Equal = LessOrEqual ; Bigger + Equal = BiggerOrEqual
* в дальнешем на основе этого свойства будет реализовано создание лексем операций сравнения != , <= , >=
*/
enum CmpValue { Negation, Equal, NotEqual, Less, LessOrEqual, Bigger, BiggerOrEqual };

/* стурктура "лексема" */
struct Token {
	TokenClass    token_class;  // класс лексемы
	int           value;	    // значени лексемы
	int           line;			// номер строки

	
	Token() = default;
	/* конструктор с параметрами */
	Token(TokenClass tc, int v, int l) {
		token_class = tc;
		value       = v;
		line 		= l;
	}
};

/* оператор вывода лексемы */
std::ostream& operator <<(std::ostream& stream, Token token) {
	stream.width(2); stream.setf(std::ios::left, std::ios::right | std::ios::left);
	stream << token.line << ": (";
	switch (token.token_class) {
	case Push:  stream << "push, " << token.value; break;
	case Pop:   stream << "pop, "  << token.value; break;
	case Jmp:   stream << "jmp, "  << token.value; break;
	case Ji:    stream << "ji, "   << token.value; break;
	case Read:  stream << "read";  break;
	case Write: stream << "write"; break;
	case End:   stream << "end";   break;

	case Atpow: 	 stream << "atpow";      break;
	case Deg:   	 stream << "deg";        break;
	case Derivative: stream << "derivative"; break;
	case Value:      stream << "value";      break;

	case ArithmeticOp: stream << (char)token.value; break;
	case CmpOp: switch (token.value) {
				case Equal:         stream << '=';  break;
				case NotEqual:      stream << "!="; break;
				case Less:          stream << '<';  break;
				case LessOrEqual:   stream << "<="; break;
				case Bigger:	    stream << '>';  break;
				case BiggerOrEqual: stream << ">="; break;
				default:            stream << "Неизвестное отношение...";
				}
				break;

	case Comment:   stream << "Комментарий"; break;
	case Error:     stream << "Ошибка";      break;
	case EndOfFile: stream << "Конец файла"; break;
	default:        stream << "Неизвестная лексема...";
	}
	stream << ')';
	stream.setf(std::ios::right, std::ios::right | std::ios::left);

	return stream;
}

/* закрытое перечисление типов объекта : переменная или константа */
enum class ObjectType { Variable, IntConstant, PolConstant }; 

/* структура "имя объекта" */
struct ObjectName {
	ObjectType 		type;			// тип объекта      : константа или переменная
	void* 			name_pointer;	// указатель на имя : на строку или число

	/* конструктор с параметрами */
	ObjectName(ObjectType ot, void* np) {
		type = ot;
		name_pointer = np;
	}
};

/* вывод имени переменной или константы */
std::ostream& operator <<(std::ostream& stream, ObjectName object_name) {
	switch (object_name.type) {
	case ObjectType::Variable: 	  stream << "Переменная: " << *( (std::string*) object_name.name_pointer); break;
	case ObjectType::IntConstant: stream << "Константа: "  << *(    (int*)      object_name.name_pointer); break;
	case ObjectType::PolConstant: stream << "Константа: "  << *( (Polynomial*)  object_name.name_pointer); break;
	default:				      stream << "Неизвестный объект...";
	}

	return stream;
}

/*  класс "Обработанная программа" 
*
* 	состоит из имени программы,
* 			вектора лексем, выделенных анализатором
* 			и таблицы имён, построенной анализатором;
*
*	ананлизатор возвращает объект этого типа, после окончания обработки прогрмаммы
*/
class ParsedProgram {
private:
	const char*				program_name;		// имя программы
	std::vector<Token> 		      tokens;		// вектор лексем
	std::vector<ObjectName>   name_table;		// таблица имён

	friend class Interpreter;
public:
	/* конструктор по умолчанию : необходим для создани пустого объекта;
	*  анализатор возвращает пустую обработанную программу, если не удалось открыть файл */
	ParsedProgram(const char* filename) {
		program_name = filename;
		tokens.push_back(Token(Error, 0, 1));
	}

	/* конструктор с параметрами : данные передаёт анализатор после обработки файла
	*  имя программы передаётся в программу через аргументы main : достаточно только передать указатель 
	*  анализатор, передаёт вектор лексем и таблицу имён без копирования */
	ParsedProgram(std::vector<Token>&& parsed_tokens, std::vector<ObjectName>&& parsed_name_table, const char* filename) {
		program_name = filename;
		tokens       = parsed_tokens;
		name_table   = parsed_name_table;
	}
	/* конструктор перемещения : для того чтобы потом передать обработанную программу интерпретатору */
	ParsedProgram(ParsedProgram&& _program) {
		program_name = _program.program_name;
		_program.program_name = nullptr;

		tokens     = std::move(_program.tokens);
		name_table = std::move(_program.name_table);
	}
	/* анализатор создаёт таблицу переменных и констант в динамической памяти,
	*  а затем передаёт указатель на неё, поэтому необходимо эту память освободить в деструкторе данного класса */
	~ParsedProgram() {
		for (auto& name : name_table) {
			switch (name.type) {
			case ObjectType::Variable:    delete (std::string*)name.name_pointer;  break;
			case ObjectType::IntConstant: delete (int*)name.name_pointer; 		   break;
			case ObjectType::PolConstant: delete (Polynomial*)name.name_pointer;   break;
			}
		}
	}

	/* вывод списка лексем */
	void print_tokens(std::ostream& stream = std::cout) {
		stream << "Список лексем в файле " << program_name << ":\n";
		for (auto& token : tokens) {
			stream << token << '\n';
		}
	}
	/* вывод таблицы переменных и констант */
	void print_names(std::ostream& stream = std::cout) {
		stream << "Таблица переменных и констант:\n";
		for (int i = 0; i < name_table.size(); ++i) {
			stream << i << ". " << name_table[i] << '\n';
		}
	}

	/* вывод исполняемых лексем с именами переменных/констант, на которые есть ссылка */
	void print_executable_tokens(std::ostream& stream = std::cout) {
		stream << "Исполняемые лексемы в файле " << program_name << ":\n";
		for (auto& token : tokens) {
			if (token.token_class == Comment || token.token_class == Error || token.token_class == EndOfFile) continue;

			stream << token;
			if (token.token_class == Push || token.token_class == Pop || token.token_class == Jmp || token.token_class == Ji) {
				stream << " ("  << name_table[token.value] << ')';
			}
			stream << '\n';
		}
	}

	/* вывод ошибок */
	int print_errors(std::ostream& stream = std::cout) {
		stream << "Найдены ошибки в файле " << program_name << " в строках:\n";

		int errors_count = 0;
		for (auto& token : tokens) {
			if (token.token_class != Error) continue;

			stream << token.line << '\n';
			++errors_count;
		}

		return errors_count;
	}
};

/* перечисление состояний автомата */
enum State { s_A1, s_A2,		// поиск новой лексемы
			 s_B1,				// считывание ключевого слова
			 s_C1,				// считывание строки до конца или комментария после правильной команды
			 s_D1,				// считывание операции сравнения
			 s_E1, s_E2, s_E3,  // считывание пробелоа после ключевого слова, после которого нужен аргумент
			 s_F1, s_F2, s_F3,  // считывание пробелов до константы или перемнной после ключевого слова
			 s_G1,				// считывание константы
			 s_H1,				// считывание переменной
			 s_I1, s_I2,		// считывание комментария
			 s_J1,				// считывание строки до конца после ошибки
			 s_K1,				// ожидание очередного слагаемого многочлена
			 s_L1, s_L2, s_L3,	// считывание степени слагаемого многочлена
			 s_N1, s_N2, s_N3,	// считывание коэффициента при степени многочлена
			 s_Stop			    // остановка
		    };
const int STATES_COUNT = 23;    // количество состояний автомата (без s_Stop)

/* класс "лексический анализатор" */
class Parser {
private:
	/* транслитератор : получает очередной символ и создаёт его символьную лексему */
	static SymbolicToken transtilerator(char raw_symbol)
	{	
		SymbolicToken st;
		st.value = 0;

		if ( ('a' <= raw_symbol && raw_symbol <= 'z') ||
			 ('A' <= raw_symbol && raw_symbol <= 'Z') )
		{
			st.token_class = Letter;
			st.value = raw_symbol;
		}
		else if (isdigit(raw_symbol))
		{
			st.token_class = Digit;
			st.value = raw_symbol - '0';
		}
		else if (raw_symbol == '+' || raw_symbol == '-' || raw_symbol == '*' ||
				 raw_symbol == '/' || raw_symbol == '%')
		{
			st.token_class = Arithmetic;
			st.value = raw_symbol;
		}
		else if (raw_symbol == '<' || raw_symbol == '>' || raw_symbol == '=' ||
				 raw_symbol == '!')
		{
			st.token_class = Cmp;
			switch (raw_symbol) {
			case '<': st.value = Less;      break;
			case '>': st.value = Bigger;    break;
			case '=': st.value = Equal;     break;
			case '!': st.value = Negation;  break;
			}
		}
		else if (raw_symbol == ' ' || raw_symbol == '\t')
		{
			st.token_class = Space;
		}
		else if (raw_symbol == '\n')
		{
			st.token_class = LF;
		}
		else if (raw_symbol == ';')
		{
			st.token_class = SemiCol;
		}
		else if (raw_symbol == EOF)
		{
			st.token_class = EndOfFileSymbol;
		}
		else if (raw_symbol == '[')
		{
			st.token_class = LSqBracket;
		}
		else if (raw_symbol == ']')
		{
			st.token_class = RSqBracket;
		}
		else if (raw_symbol == '.')
		{
			st.token_class = Dot;
		}
		else if (raw_symbol == ':') {
			st.token_class = Colon;
		}
		else
		{
			st.token_class = Other;
		}
		return st;
	}

	/* процедура СОЗДАТЬ_ЛЕКСЕМУ : добавить в список лексем новую лексему */
	void add_token() {
		tokens.push_back(Token(token_class, token_value, line_number));
	}
	/* процедура СОЗДАТЬ_КОНСТАНТУ : проверить нет ли такой константы в таблице имён
	*								 если есть, то поменять регистр указателя, чтобы он указывал на уже существующий объект
	*								 если нет, 	то добавить и поменять регистр указателя, чтобы он указывал на новый объект
	*/
	void add_constant() {
		for(int i = 0; i < name_table.size(); ++i) {
			if (name_table[i].type != ObjectType::IntConstant) continue;

			if (*((int*)name_table[i].name_pointer) == number) {
				name_table_index = i;
				return;
			}
		}

		int* new_constant = new int(number);
		name_table.push_back(ObjectName(ObjectType::IntConstant, new_constant));
		name_table_index = name_table.size() - 1;
	}
	void add_polynomial() {
		for(int i = 0; i < name_table.size(); ++i) {
			if (name_table[i].type != ObjectType::PolConstant) continue;

			if (*((Polynomial*)name_table[i].name_pointer) == polynomial) {
				name_table_index = i;
				return;
			}
		}

		Polynomial* new_polynomial = new Polynomial(polynomial);
		name_table.push_back(ObjectName(ObjectType::PolConstant, new_polynomial));
		name_table_index = name_table.size() - 1;		
	}
	/* процедура СОЗДАТЬ_ПЕРЕМЕННУЮ проверить нет ли такой переменной в таблице имён
	*								если есть, то поменять регистр указателя, чтобы он указывал на уже существующий объект
	*								если нет,  то добавить и поменять регистр указателя, чтобы он указывал на новый объект
	*/
	void add_variable() {
		for(int i = 0; i < name_table.size(); ++i) {
			if (name_table[i].type != ObjectType::Variable) continue;

			if (*((std::string*)name_table[i].name_pointer) == variable_name) {
				name_table_index = i;
				return;
			}
		}

		std::string* new_variable_name = new std::string(variable_name);
		name_table.push_back(ObjectName(ObjectType::Variable, new_variable_name));
		name_table_index = name_table.size() - 1;
	}

	// ---------------------------------------------------------------------
	// процедуры автомата (подробное описание приведено в Стековый язык.pdf)
	// ---------------------------------------------------------------------
	State A1() {
		return s_A1;
	}
	State A1a() {
		add_token();
		line_number++;
		return s_A1;
	}
	State A1b() {
		line_number++;
		return s_A1;
	}
	State A2() {
		return s_A2;
	}
	State A2a() {
		line_number++;
		return s_A2;
	}
	State A2b() {
		add_token();
		line_number++;
		return s_A2;
	}
	State A2c() {
		add_constant();
		token_value = name_table_index;
		add_token();
		line_number++;
		return s_A2;
	}
	State A2d() {
		add_variable();
		token_value = name_table_index;
		add_token();
		line_number++;
		return s_A2;
	}
	State A2e() {
		if (cmp_value == Negation) return J1a();
		token_value = cmp_value;
		add_token();
		line_number++;

		return s_A2;
	}
	State A2f() {
		token_class = Error;
		token_value = 0;
		add_token();
		line_number++;

		return s_A2;
	}


	State B1() {
		return s_B1;
	}
	State B1a() {
		if ('A' <= symbolic_token.value && symbolic_token.value <= 'Z') return J1a();

		detection_index = detection_table.init_vector[symbolic_token.value - 'a'];
		if (detection_index == -1) return J1a();

		return s_B1;
	}
	State B1b() {
		detection_index++;
		return s_B1;
	}

	State C1() {
		return s_C1;
	}
	State C1a() {
		token_class = ArithmeticOp;
		token_value = symbolic_token.value;
		add_token();
		return s_C1;
	}
	State C1b() {
		token_class = End;
		token_value = 0;
		add_token();

		return s_C1;
	}
	State C1c() {
		token_class = Read;
		token_value = 0;
		add_token();

		return s_C1;
	}
	State C1d() {
		token_class = Write;
		token_value = 0;
		add_token();

		return s_C1;
	}
	State C1e() {
		add_constant();
		token_value = name_table_index;
		add_token();
		return s_C1;
	}
	State C1f() {
		add_variable();
		token_value = name_table_index;
		add_token();
		return s_C1;
	}
	State C1g() {
		if (cmp_value == Negation) return J1a();
		add_token();
		return s_C1;
	}
	State C1h() {
		if (cmp_value == Equal) return J1a();

		if (symbolic_token.value == Equal) {
			token_value = cmp_value + symbolic_token.value;
			add_token();
		}
		else return J1a();

		return s_C1;
	}
	State C1i() {
		token_class = Atpow;
		token_value = 0;
		add_token();

		return s_C1;
	}
	State C1j() {
		token_class = Deg;
		token_value = 0;
		add_token();

		return s_C1;
	}
	State C1k() {
		token_class = Derivative;
		token_value = 0;
		add_token();

		return s_C1;
	}
	State C1l() {
		token_class = Value;
		token_value = 0;
		add_token();

		return s_C1;
	}
	State C1m() {
		add_polynomial();
		token_value = name_table_index;
		add_token();

		return s_C1;
	}
	State C1n() {
		for (int i = 0; i < fract_count; ++i) {
			fract_part /= 10;
		}
		polynomial = polynomial + Polynomial(number, int_part + fract_part);

		add_polynomial();
		token_value = name_table_index;
		add_token();

		return s_C1;
	}

	State D1() {
		return s_D1;
	}
	State D1a() {
		token_class = CmpOp;
		token_value = cmp_value = (CmpValue)symbolic_token.value;

		return s_D1;
	}

	State E1() {
		return s_E1;
	}
	State E1a() {
		token_class = Push;
		return s_E1;
	}
	State E2() {
		return s_E2;
	}
	State E2a() {
		token_class = Ji;
		return s_E2;
	}
	State E2b() {
		token_class = Jmp;
		return s_E2;
	}
	State E3() {
		return s_E3;
	}
	State E3a() {
		token_class = Pop;
		return s_E3;
	}

	State F1(){
		return s_F1;
	}
	State F2(){
		return s_F2;
	}
	State F3(){
		return s_F3;
	}

	State G1() {
		return s_G1;
	}
	State G1a() {
		number = symbolic_token.value;

		return s_G1;
	}
	State G1b() {
		number = 10 * number + symbolic_token.value;

		return s_G1;
	}

	State H1() {
		return s_H1;
	}
	State H1a() {
		variable_name = symbolic_token.value;

		return s_H1;
	}
	State H1b() {
		variable_name += symbolic_token.value;

		return s_H1;
	}

	State I1() {
		return s_I1;
	}
	State I1a() {
		token_class = Comment;
		token_value = 0;

		return s_I1;
	}
	State I2() {
		return s_I2;
	}
	State I2a() {
		token_class = Comment;
		token_value = 0;

		return s_I2;
	}
	State I2b() {
		add_constant();
		token_value = name_table_index;
		add_token();

		token_class = Comment;
		token_value = 0;

		return s_I2;
	}
	State I2c() {
		add_variable();
		token_value = name_table_index;
		add_token();

		token_class = Comment;
		token_value = 0;

		return s_I2;
	}
	State I2d() {
		if (cmp_value == Negation) return J1a();

		token_value = cmp_value;
		add_token();

		return s_I2;
	}

	State M1() {
		while (true) {
			if (detection_table.table[detection_index].letter == symbolic_token.value) return (this->*detection_table.table[detection_index].procedure)();

			detection_index = detection_table.table[detection_index].alt;
			if (detection_index == -1) return J1a();
		}
	}

	State Exit1() {
		token_class = EndOfFile;
		token_value = 0;
		add_token();

		return s_Stop;
	}
	State Exit2() {
		if (cmp_value == Negation) return J1a();

		token_value = cmp_value;
		add_token();

		token_class = EndOfFile;
		token_value = 0;
		add_token();

		return s_Stop;
	}
	State Exit3() {
		add_constant();
		token_value = name_table_index;
		add_token();

		token_class = EndOfFile;
		token_value = 0;
		add_token();

		return s_Stop;
	}
	State Exit4() {
		add_variable();
		token_value = name_table_index;
		add_token();

		token_class = EndOfFile;
		token_value = 0;
		add_token();

		return s_Stop;
	}
	State Exit5() {
		add_token();

		token_class = EndOfFile;
		token_value = 0;
		add_token();

		return s_Stop;
	}

	State J1() {
		return s_J1;
	}

	State J1a() {
		token_class = Error;
		token_value = 0;
		add_token();

		return s_J1;
	}

	State K1() {
		return s_K1;
	}
	State K1a() {
		polynomial.clear();
		return s_K1;
	}
	State K1b() {
		for (int i = 0; i < fract_count; ++i) {
			fract_part /= 10;
		}
		polynomial = polynomial + Polynomial(number, int_part + fract_part);

		return s_K1;
	}

	State L1() {
		return s_L1;
	}
	State L1a() {
		switch (symbolic_token.value) {
		case '+': sign =  1; break;
		case '-': sign = -1; break;
		default:  return J1a();
		}

		return s_L1;
	}
	State L1b() {
		for (int i = 0; i < fract_count; ++i) {
			fract_part /= 10;
		}
		polynomial = polynomial + Polynomial(number, int_part + fract_part);

		switch (symbolic_token.value) {
		case '+': sign =  1; break;
		case '-': sign = -1; break;
		default:  return J1a();
		}

		return s_L1;
	}

	State L2() {
		return s_L2;
	}
	State L2a() {
		number = symbolic_token.value;

		return s_L2;
	}
	State L2b() {
		number = number * 10 + symbolic_token.value;
		return s_L2;
	}

	State L3() {
		return s_L3;
	}

	State N1() {
		return s_N1;
	}
	State N1a() {
		fract_part = 0;
		fract_count = 0;

		return s_N1;
	}

	State N2() {
		return s_N2;
	}
	State N2a() {
		int_part = symbolic_token.value;

		return s_N2;
	}
	State N2b() {
		int_part = int_part * 10 + symbolic_token.value;

		return s_N2;
	}

	State N3() {
		return s_N3;
	}

	State N3a() {
		fract_part = fract_part * 10 + symbolic_token.value;
		fract_count++;

		return s_N3;
	}

public:
	/* констурктор задаёт начальное значение номера строки и указателя на таблицу имён 
	*  а также инициализирует таблицу процедур и таблицу обнаружений
	*/
	Parser()
	{	
		// --------------------------------------
		// инициализация табллицы процедур
		// --------------------------------------

		for (int i = 0; i < STATES_COUNT; ++i)
		{
			for (int j = 0; j < SYMBOL_TOKEN_CLASS_COUNT; ++j)
			{
				procedure_table[i][j] = &Parser::J1a;
			}
		}

		procedure_table[s_A1][Letter] = &Parser::B1a;	procedure_table[s_F1][Digit] = &Parser::G1a;				procedure_table[s_A1][Arithmetic] = &Parser::C1a;
		procedure_table[s_A2][Letter] = &Parser::B1a;	procedure_table[s_F2][Digit] = &Parser::G1a;				procedure_table[s_A2][Arithmetic] = &Parser::C1a;
		procedure_table[s_B1][Letter] = &Parser::M1; 	procedure_table[s_G1][Digit] = &Parser::G1b;				procedure_table[s_A1][Arithmetic] = &Parser::C1a;
		procedure_table[s_F1][Letter] = &Parser::H1a;	procedure_table[s_H1][Digit] = &Parser::H1b;				procedure_table[s_I1][Arithmetic] = &Parser::I1;	
		procedure_table[s_F3][Letter] = &Parser::H1a;	procedure_table[s_I1][Digit] = &Parser::I1;					procedure_table[s_I2][Arithmetic] = &Parser::I2;	
		procedure_table[s_H1][Letter] = &Parser::H1b;	procedure_table[s_I2][Digit] = &Parser::I2;					procedure_table[s_J1][Arithmetic] = &Parser::J1;	
		procedure_table[s_I1][Letter] = &Parser::I1;	procedure_table[s_J1][Digit] = &Parser::J1;					procedure_table[s_K1][Arithmetic] = &Parser::L1a;
		procedure_table[s_I2][Letter] = &Parser::I2;	procedure_table[s_L1][Digit] = &Parser::L2a;				procedure_table[s_N2][Arithmetic] = &Parser::L1b;
		procedure_table[s_J1][Letter] = &Parser::J1;	procedure_table[s_L2][Digit] = &Parser::L2b;				procedure_table[s_N3][Arithmetic] = &Parser::L1b;
														procedure_table[s_N1][Digit] = &Parser::N2a;
														procedure_table[s_N2][Digit] = &Parser::N2b;
														procedure_table[s_N3][Digit] = &Parser::N3a;

		procedure_table[s_A1][Cmp] = &Parser::D1a;		procedure_table[s_A1][Space] = &Parser::A1;					procedure_table[s_A1][LF] = &Parser::A1b;
		procedure_table[s_A2][Cmp] = &Parser::D1a;		procedure_table[s_A2][Space] = &Parser::A2;					procedure_table[s_A2][LF] = &Parser::A2a;
		procedure_table[s_D1][Cmp] = &Parser::C1h;		procedure_table[s_C1][Space] = &Parser::C1;					procedure_table[s_B1][LF] = &Parser::A2f;
		procedure_table[s_I1][Cmp] = &Parser::I1;		procedure_table[s_D1][Space] = &Parser::C1g;				procedure_table[s_C1][LF] = &Parser::A2a;
		procedure_table[s_I2][Cmp] = &Parser::I2;		procedure_table[s_E1][Space] = &Parser::F1;					procedure_table[s_D1][LF] = &Parser::A2e;
		procedure_table[s_J1][Cmp] = &Parser::J1;		procedure_table[s_E2][Space] = &Parser::F2;					procedure_table[s_E1][LF] = &Parser::A2f;
														procedure_table[s_E3][Space] = &Parser::F3;					procedure_table[s_E2][LF] = &Parser::A2f;
														procedure_table[s_F1][Space] = &Parser::F1;					procedure_table[s_E3][LF] = &Parser::A2f;
														procedure_table[s_F2][Space] = &Parser::F2;					procedure_table[s_F1][LF] = &Parser::A2f;
														procedure_table[s_F3][Space] = &Parser::F3;					procedure_table[s_F2][LF] = &Parser::A2f;
														procedure_table[s_G1][Space] = &Parser::C1e;				procedure_table[s_F3][LF] = &Parser::A2f;
														procedure_table[s_H1][Space] = &Parser::C1f;				procedure_table[s_G1][LF] = &Parser::A2c;
														procedure_table[s_I1][Space] = &Parser::I1;					procedure_table[s_H1][LF] = &Parser::A2d;
														procedure_table[s_I2][Space] = &Parser::I2;					procedure_table[s_I1][LF] = &Parser::A1a;
														procedure_table[s_J1][Space] = &Parser::J1;					procedure_table[s_I2][LF] = &Parser::A2b;
														procedure_table[s_K1][Space] = &Parser::K1;					procedure_table[s_J1][LF] = &Parser::A2a;
														procedure_table[s_L1][Space] = &Parser::L1;
														procedure_table[s_L2][Space] = &Parser::L3;
														procedure_table[s_L3][Space] = &Parser::L3;
														procedure_table[s_N1][Space] = &Parser::N1;
														procedure_table[s_N2][Space] = &Parser::K1b;
														procedure_table[s_N3][Space] = &Parser::K1b;

		procedure_table[s_A1][SemiCol] = &Parser::I1a;	procedure_table[s_A2][EndOfFileSymbol] = &Parser::Exit1;	procedure_table[s_I1][Other] = &Parser::I1;
		procedure_table[s_A2][SemiCol] = &Parser::I2a;	procedure_table[s_C1][EndOfFileSymbol] = &Parser::Exit1;	procedure_table[s_I2][Other] = &Parser::I2;
		procedure_table[s_C1][SemiCol] = &Parser::I2a;	procedure_table[s_D1][EndOfFileSymbol] = &Parser::Exit2;	procedure_table[s_J1][Other] = &Parser::J1;
		procedure_table[s_D1][SemiCol] = &Parser::I2d;	procedure_table[s_G1][EndOfFileSymbol] = &Parser::Exit3;
		procedure_table[s_G1][SemiCol] = &Parser::I2b;	procedure_table[s_H1][EndOfFileSymbol] = &Parser::Exit4;
		procedure_table[s_H1][SemiCol] = &Parser::I2c;	procedure_table[s_I2][EndOfFileSymbol] = &Parser::Exit5;
		procedure_table[s_I1][SemiCol] = &Parser::I1;	procedure_table[s_J1][EndOfFileSymbol] = &Parser::Exit1;
		procedure_table[s_I2][SemiCol] = &Parser::I2;
		procedure_table[s_J1][SemiCol] = &Parser::J1;

		procedure_table[s_F1][LSqBracket] = &Parser::K1a;
		procedure_table[s_I1][LSqBracket] = &Parser::I1;  procedure_table[s_I1][RSqBracket] = &Parser::I1;  procedure_table[s_I1][Dot] = &Parser::I1;
		procedure_table[s_I2][LSqBracket] = &Parser::I2;  procedure_table[s_I2][RSqBracket] = &Parser::I2;  procedure_table[s_I2][Dot] = &Parser::I2;
		procedure_table[s_J1][LSqBracket] = &Parser::J1;  procedure_table[s_J1][RSqBracket] = &Parser::J1;  procedure_table[s_J1][Dot] = &Parser::J1;
														  procedure_table[s_K1][RSqBracket] = &Parser::C1m; procedure_table[s_N2][Dot] = &Parser::N3;
														  procedure_table[s_N2][RSqBracket] = &Parser::C1n;
														  procedure_table[s_N3][RSqBracket] = &Parser::C1n;

		procedure_table[s_I1][Colon] = &Parser::I1;
		procedure_table[s_I2][Colon] = &Parser::I2;
		procedure_table[s_J1][Colon] = &Parser::J1;
		procedure_table[s_L2][Colon] = &Parser::N1a;
		procedure_table[s_L3][Colon] = &Parser::N1a;

		// --------------------------------------
		// инициализация таблицы обнаружений
		// --------------------------------------

		/* инициализация начального вектора */
		for(int i = 0; i < 26; ++i)
		{
			detection_table.init_vector[i] = -1;
		}

		detection_table.init_vector['a' - 'a'] = 17;
		detection_table.init_vector['d' - 'a'] = 21;
		detection_table.init_vector['e' - 'a'] =  0;
		detection_table.init_vector['j' - 'a'] =  2;
		detection_table.init_vector['p' - 'a'] =  5;
		detection_table.init_vector['r' - 'a'] = 10;
		detection_table.init_vector['v' - 'a'] = 31;
		detection_table.init_vector['w' - 'a'] = 13;

		/* инициализация самой таблицы */
		for (int i = 0; i < 35; ++i)
		{
			detection_table.table[i].alt = -1;
			detection_table.table[i].procedure = &Parser::B1b;
		}

		detection_table.table[0].letter =  'n';											
		detection_table.table[1].letter =  'd';											detection_table.table[1].procedure = &Parser::C1b;
												// end
		detection_table.table[2].letter =  'i';		detection_table.table[2].alt = 3;	detection_table.table[2].procedure = &Parser::E2a;
												// ji
		detection_table.table[3].letter =  'm';											
		detection_table.table[4].letter =  'p';											detection_table.table[4].procedure = &Parser::E2b;
												// jmp
		detection_table.table[5].letter =  'o';		detection_table.table[5].alt = 7;	
		detection_table.table[6].letter =  'p';											detection_table.table[6].procedure = &Parser::E3a;
												// pop
		detection_table.table[7].letter =  'u';											
		detection_table.table[8].letter =  's';											
		detection_table.table[9].letter =  'h';											detection_table.table[9].procedure = &Parser::E1a;
												// push
		detection_table.table[10].letter = 'e';											
		detection_table.table[11].letter = 'a';											
		detection_table.table[12].letter = 'd';											detection_table.table[12].procedure = &Parser::C1c;
												// read
		detection_table.table[13].letter = 'r';											
		detection_table.table[14].letter = 'i';											
		detection_table.table[15].letter = 't';											
		detection_table.table[16].letter = 'e';											detection_table.table[16].procedure = &Parser::C1d;
												// write
		detection_table.table[17].letter = 't';											
		detection_table.table[18].letter = 'p';											
		detection_table.table[19].letter = 'o';											
		detection_table.table[20].letter = 'w';											detection_table.table[20].procedure = &Parser::C1i;
												// atpow
		detection_table.table[21].letter = 'e';											
		detection_table.table[22].letter = 'g';		detection_table.table[22].alt = 23; detection_table.table[22].procedure = &Parser::C1j;
												// deg
		detection_table.table[23].letter = 'r';											
		detection_table.table[24].letter = 'i';											
		detection_table.table[25].letter = 'v';											
		detection_table.table[26].letter = 'a';											
		detection_table.table[27].letter = 't';											
		detection_table.table[28].letter = 'i';											
		detection_table.table[29].letter = 'v';											
		detection_table.table[30].letter = 'e';											detection_table.table[30].procedure = &Parser::C1k;
												// derivative
		detection_table.table[31].letter = 'a';											
		detection_table.table[32].letter = 'l';											
		detection_table.table[33].letter = 'u';											
		detection_table.table[34].letter = 'e';											detection_table.table[34].procedure = &Parser::C1l;
												// value
	}

	/* основная функция, обрабатывающая программу */
	ParsedProgram run(const char* filename) {
		std::ifstream in(filename);

		if (!in) {
			std::cout << "Не удалось открыть файл " << filename << std::endl;
			return ParsedProgram(filename);
		}	

		char symbol;
		// начальное состояние
		State state = s_A1;

		line_number = 1;
		while (state != s_Stop)
		{
			// читаем символ
			symbol = in.get(); 						 //std::cout << symbol << ' ';
			// строим его символьную лексему
			symbolic_token = transtilerator(symbol); 
			// применяем процедуру
			state = (this->*procedure_table[state][symbolic_token.token_class])();
			//std::cout << state << '\n';
		}

		in.close();

		// возвращаем обработанную программу
		return ParsedProgram(std::move(tokens), std::move(name_table), filename);
	}
private:
	SymbolicToken symbolic_token;											// очередная символьная лексема

	TokenClass    token_class;												// класс лексемы языка
	int           token_value;												// значени лексемы языка
	int 		  line_number;												// номер строки

	int           name_table_index;											// регистр указателя

	int 		  number;													// регистр числа

	CmpValue 	  cmp_value;											    // регистр отношения

	std::string   variable_name;											// регистр имени переменной

	int sign;																// регистр знака
	int int_part;															// регистр целой части
	float fract_part;														// регистр дробной части
	int fract_count;														// регистр порядка
	Polynomial polynomial;													// регистр многочлена


	using parser_procedure = State (Parser::*)();
	parser_procedure procedure_table[STATES_COUNT][SYMBOL_TOKEN_CLASS_COUNT];	// таблица процедур автомата

	// структура "таблица обнаружений"
	struct DetectionTable {
		int init_vector[26];				// начальный вектор

		struct DetectionTableLine {
			char letter;				// буква
			char alt;					// альтернатива
			parser_procedure procedure;	// процедура
		};
		DetectionTableLine table[35];		// таблица
	};
	DetectionTable 		 detection_table;									// таблциа обнаружений
	int 		         detection_index;									// регистр обнаружений

	std::vector<Token> 		      tokens;									// вектор лексем
	std::vector<ObjectName>   name_table;									// таблица имён
};
