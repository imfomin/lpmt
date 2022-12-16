#include <iostream>
#include <vector>
#include <map>
#include "polynomial.hpp"
#include "parser.cpp"

/* перечисление типов значений объектов в стековом языке : натуральное число и многочлен с вещественными коэффициентами */
enum class ValueType { Integer, Polynomial };

/* класс "объект"; состоит из указателя на реальное значени и значение типа */
class Object {
private:
	void* value_pointer;
	ValueType type;
public:
	/* конструкторы и декструкоры ; операторы присваивания */
	Object(void* ptr = nullptr, ValueType t = ValueType::Integer) {
		value_pointer = ptr;
		type = t;
	}
	void clear() {
		if (value_pointer) {
			switch (type) {
			case ValueType::Integer:    delete (int*)value_pointer; 	   break;
			case ValueType::Polynomial: delete (Polynomial*)value_pointer; break; 
			}
		}
		value_pointer = nullptr;
		type = ValueType::Integer;
	}
	~Object() {
		clear();
	}
	Object(const Object& other) {
		type = other.type;
		switch (type) {
		case ValueType::Integer:    value_pointer = new int(*(int*)other.value_pointer); 			   break;
		case ValueType::Polynomial: value_pointer = new Polynomial(*(Polynomial*)other.value_pointer); break;
		}
	}
	Object(Object&& other) {
		type = other.type;
		other.type = ValueType::Integer;

		value_pointer = other.value_pointer;
		other.value_pointer = nullptr;
	}
	Object& operator=(const Object& other) {
		if (this == &other) return *this;

		clear();

		type = other.type;
		switch (type) {
		case ValueType::Integer:    value_pointer = new int(*(int*)other.value_pointer); 			   break;
		case ValueType::Polynomial: value_pointer = new Polynomial(*(Polynomial*)other.value_pointer); break;
		}

		return *this;
	}
	Object& operator=(Object&& other) {
		if (this == &other) return *this;

		clear();

		type = other.type;
		other.type = ValueType::Integer;

		value_pointer = other.value_pointer;
		other.value_pointer = nullptr;

		return *this;
	}

	/* доступ к полям */
	ValueType get_type() {
		return type;
	}
	void* get_ptr() {
		return value_pointer;
	}
	void set(void* ptr, ValueType _type) {
		clear();

		value_pointer = ptr;
		type = _type;
	}

/* макрос с параметорм : значок операции +, -, *, /, %
*  применяет операцию к двум объектам ; используется только в соостветсвующих операторах
*  т. к. использует имена формальных параметров */
#define CALCULATE(infix_operator) \
		int* int_ptr1 = nullptr; Polynomial* pol_ptr1 = nullptr;\
		switch (type) {\
		case ValueType::Integer:    int_ptr1 = (int*)        value_pointer; break;\
		case ValueType::Polynomial: pol_ptr1 = (Polynomial*) value_pointer; break;\
		}\
		int* int_ptr2 = nullptr; Polynomial* pol_ptr2 = nullptr;\
		switch (other.type) {\
		case ValueType::Integer:    int_ptr2 = (int*)        other.value_pointer; break;\
		case ValueType::Polynomial: pol_ptr2 = (Polynomial*) other.value_pointer; break;\
		}\
\
		Object new_obj;\
		if (int_ptr1 && int_ptr2) {\
			new_obj.set(new int(*int_ptr1 infix_operator *int_ptr2), ValueType::Integer);\
		}\
		if (pol_ptr1 && int_ptr2) {\
			new_obj.set(new Polynomial(*pol_ptr1 infix_operator Polynomial(*int_ptr2)), ValueType::Polynomial);\
		}\
		if (int_ptr1 && pol_ptr2) {\
			new_obj.set(new Polynomial(Polynomial(*int_ptr1) infix_operator *pol_ptr2), ValueType::Polynomial);\
		}\
		if (pol_ptr1 && pol_ptr2) {\
			new_obj.set(new Polynomial(*pol_ptr1 infix_operator *pol_ptr2), ValueType::Polynomial);\
		}\
		return new_obj;
// end define

	/* операторы арифметических дейсвий с объектами */
	Object operator +(const Object& other) const {
		CALCULATE(+)
	}
	Object operator -(const Object& other) const {
		CALCULATE(-)
	}
	Object operator *(const Object& other) const {
		CALCULATE(*)
	}
	Object operator /(const Object& other) const {
		CALCULATE(/)
	}
	Object operator %(const Object& other) const {
		CALCULATE(%)
	}

/* аналогичный макрос для сравнений ==, != */
#define COMPARE_1(infix_operator) \
		int* int_ptr1 = nullptr; Polynomial* pol_ptr1 = nullptr;\
		switch (type) {\
		case ValueType::Integer:    int_ptr1 = (int*)        value_pointer; break;\
		case ValueType::Polynomial: pol_ptr1 = (Polynomial*) value_pointer; break;\
		}\
		int* int_ptr2 = nullptr; Polynomial* pol_ptr2 = nullptr;\
		switch (other.type) {\
		case ValueType::Integer:    int_ptr2 = (int*)        other.value_pointer; break;\
		case ValueType::Polynomial: pol_ptr2 = (Polynomial*) other.value_pointer; break;\
		}\
\
		if (int_ptr1 && int_ptr2) {\
			return *int_ptr1 infix_operator *int_ptr2;\
		}\
		if (pol_ptr1 && int_ptr2) {\
			return *pol_ptr1 infix_operator Polynomial(*int_ptr2);\
		}\
		if (int_ptr1 && pol_ptr2) {\
			return Polynomial(*int_ptr1) infix_operator *pol_ptr2;\
		}\
		if (pol_ptr1 && pol_ptr2) {\
			return *pol_ptr1 infix_operator *pol_ptr2;\
		}
// end define

	/* операторы сравнений */
	bool operator ==(const Object& other) const {
		COMPARE_1(==)
		throw 1;
	}
	bool operator !=(const Object& other) const {
		COMPARE_1(!=)
		throw 1;
	}

/* аналогичный макрос для операций <, <=, >, >= */
#define COMPARE_2(infix_operator) \
		int* int_ptr1 = nullptr; Polynomial* pol_ptr1 = nullptr;\
		switch (type) {\
		case ValueType::Integer:    int_ptr1 = (int*)        value_pointer; break;\
		case ValueType::Polynomial: pol_ptr1 = (Polynomial*) value_pointer; break;\
		}\
		int* int_ptr2 = nullptr; Polynomial* pol_ptr2 = nullptr;\
		switch (other.type) {\
		case ValueType::Integer:    int_ptr2 = (int*)        other.value_pointer; break;\
		case ValueType::Polynomial: pol_ptr2 = (Polynomial*) other.value_pointer; break;\
		}\
\
		if (int_ptr1 && int_ptr2) {\
			return *int_ptr1 infix_operator *int_ptr2;\
		}
// end define

	/* операторы сравнений <, <=, >, >= 
	* выбрасывют исключение 1, если не удалось сравнить объекты,
	* например, если один операнд - число, а другой - многчлен */
	bool operator <(const Object& other) const {
		COMPARE_2(<)
		throw 1;
	}
	bool operator <=(const Object& other) const {
		COMPARE_2(<=)
		throw 1;
	}
	bool operator >(const Object& other) const {
		COMPARE_2(>)
		throw 1;
	}
	bool operator >=(const Object& other) const {
		COMPARE_2(>=)
		throw 1;
	}
};

class Interpreter {
private:
	std::vector<Object> 			Stack;		// стек
	std::map<std::string, Object> 	Variables;	// отображение (строка (имя переменной) - объект)

	ParsedProgram program;						// интерпретируемая программа

	int executable_token_index;					// номер (индекс) исполняемой лексемы

	bool interpreting;							// флаг интерпретации
	
/* макрос для проверки стека, перед извлечением оттуда объектоы */
#define CHECK_STACK_SIZE(_size) if (Stack.size() < (_size)) { error(); return; }

	// ---------------------------------------
	// процедуры интерпретатора
	// ---------------------------------------
	void push(int name_table_index) {
		ObjectName& obj_name = program.name_table[name_table_index];

		Object new_obj;
		switch (obj_name.type) {
		case ObjectType::Variable: if (Variables.count(*(std::string*)obj_name.name_pointer) == 0) { error(); return; }
								   new_obj = Variables[*(std::string*)obj_name.name_pointer];
								   break;								
		case ObjectType::Constant: new_obj.set(new int(*(int*)obj_name.name_pointer), ValueType::Integer); break;
		}

		Stack.push_back(std::move(new_obj));
		executable_token_index++;
	}
	void pop(int name_table_index) {
		CHECK_STACK_SIZE(1)

		ObjectName& obj_name = program.name_table[name_table_index];

		Object obj = std::move(Stack.back());
		Stack.pop_back();

		Variables[*(std::string*)obj_name.name_pointer] = obj;

		executable_token_index++;
	}
	void jmp(int name_table_index) {
		ObjectName& obj_name = program.name_table[name_table_index];
		int line_to_jump = *(int*)obj_name.name_pointer;

		if (line_to_jump == 0) { error(); return; }

		for (int token_index = line_to_jump - 1; token_index < program.tokens.size(); ++token_index) {
			if (program.tokens[token_index].line == line_to_jump) {
				executable_token_index = token_index;
				return;
			}
		}

		error();
	}
	void ji(int name_table_index) {
		CHECK_STACK_SIZE(1)

		Object obj = std::move(Stack.back());
		Stack.pop_back();

		bool jump;
		switch (obj.get_type()) {
		case ValueType::Integer:   	jump = *(int*)       obj.get_ptr(); break;
		case ValueType::Polynomial:	jump = *(Polynomial*)obj.get_ptr(); break;
		}

		if (jump) jmp(name_table_index);
		else      executable_token_index++;
	}
	void read() {
		while (std::cin.peek() == ' ' || std::cin.peek() == '\n' || std::cin.peek() == '\t') std::cin.ignore();

		Object new_obj;
		if (isdigit(std::cin.peek())) {
			int* n = new int; std::cin >> *n;
			new_obj.set(n, ValueType::Integer);
		}
		else if (std::cin.peek() == '[') {
			Polynomial* p = new Polynomial; std::cin >> *p;
			new_obj.set(p, ValueType::Polynomial);
		}
		else { error(); return; }

		Stack.push_back(std::move(new_obj));
		executable_token_index++;
	}
	void write() {
		CHECK_STACK_SIZE(1)

		Object obj = std::move(Stack.back());
		Stack.pop_back();

		switch (obj.get_type()) {
		case ValueType::Integer:    std::cout << *(int*)       obj.get_ptr(); break;
		case ValueType::Polynomial: std::cout << *(Polynomial*)obj.get_ptr(); break;
		}
		std::cout << std::endl;
		executable_token_index++;
	}
	void end() {
		interpreting = false;
	}

	void calculate(char operation) {
		CHECK_STACK_SIZE(2)

		Object obj2 = std::move(Stack.back());
		Stack.pop_back();
		Object obj1 = std::move(Stack.back());
		Stack.pop_back();

		switch (operation) {
		case '+': Stack.push_back(obj1 + obj2); break;
		case '-': Stack.push_back(obj1 - obj2); break;
		case '*': Stack.push_back(obj1 * obj2); break;
		case '/': Stack.push_back(obj1 / obj2); break;
		case '%': Stack.push_back(obj1 % obj2); break;
		}
		executable_token_index++;
	}
	void compare(CmpValue operation) {
		CHECK_STACK_SIZE(2)

		Object obj2 = std::move(Stack.back());
		Stack.pop_back();
		Object obj1 = std::move(Stack.back());
		Stack.pop_back();

		try {
			bool cmp_result;

			switch (operation) {
			case Equal:			cmp_result = (obj1 == obj2); break;
			case NotEqual:		cmp_result = (obj1 != obj2); break;
			case Less:			cmp_result = (obj1 <  obj2); break;
			case LessOrEqual:	cmp_result = (obj1 <= obj2); break;
			case Bigger:		cmp_result = (obj1 >  obj2); break;
			case BiggerOrEqual:	cmp_result = (obj1 >= obj2); break;
			}

			Stack.push_back(Object(new int(cmp_result), ValueType::Integer));
			executable_token_index++;
		}
		catch (...) { error(); }
	}

	void deg() {
		CHECK_STACK_SIZE(1)

		Object obj = std::move(Stack.back());
		Stack.pop_back();

		Polynomial p;
		switch (obj.get_type()) {
		case ValueType::Integer:    p = *(int*)       obj.get_ptr(); break;
		case ValueType::Polynomial: p = *(Polynomial*)obj.get_ptr(); break;
		}

		obj.set(new int(p.deg()), ValueType::Integer);

		Stack.push_back(std::move(obj));
		executable_token_index++;
	}
	void derivative() {
		CHECK_STACK_SIZE(1)

		Object obj = std::move(Stack.back());
		Stack.pop_back();

		Polynomial p;
		switch (obj.get_type()) {
		case ValueType::Integer:    p = *(int*)       obj.get_ptr(); break;
		case ValueType::Polynomial: p = *(Polynomial*)obj.get_ptr(); break;
		}

		obj.set(new Polynomial(p.derivative()), ValueType::Polynomial);

		Stack.push_back(std::move(obj));
		executable_token_index++;
	}

	void atpow() {
		CHECK_STACK_SIZE(2)

		Object obj2 = std::move(Stack.back());
		Stack.pop_back();
		Object obj1 = std::move(Stack.back());
		Stack.pop_back();

		if (obj2.get_type() == ValueType::Polynomial) { error(); return; }

		Polynomial p;
		switch (obj1.get_type()) {
		case ValueType::Integer:    p = *(int*)       obj1.get_ptr(); break;
		case ValueType::Polynomial: p = *(Polynomial*)obj1.get_ptr(); break;
		}

		obj1.set(new int(p[*(int*)obj2.get_ptr()]), ValueType::Integer);
		Stack.push_back(std::move(obj1));
		executable_token_index++;

	}
	void value() {
		CHECK_STACK_SIZE(2)

		Object obj2 = std::move(Stack.back());
		Stack.pop_back();
		Object obj1 = std::move(Stack.back());
		Stack.pop_back();

		if (obj2.get_type() == ValueType::Polynomial) { error(); return; }

		Polynomial p;
		switch (obj1.get_type()) {
		case ValueType::Integer:    p = *(int*)       obj1.get_ptr(); break;
		case ValueType::Polynomial: p = *(Polynomial*)obj1.get_ptr(); break;
		}

		obj1.set(new int(p(*(int*)obj2.get_ptr())), ValueType::Integer);
		Stack.push_back(std::move(obj1));
		executable_token_index++;
	}

	void skip() {
		executable_token_index++;
	}
	void error() {
		std::cout << "Ошибка во время выполнения программы...\n";
		interpreting = false;
	}

	/* процедура "обработать лексему" --- исполняет очередную лексему */
	void execute_token(Token token) {
		switch (token.token_class) {
		case Push:  		push(token.value); 				  break;
		case Pop:   		pop(token.value);  				  break;
		case Jmp:   		jmp(token.value);  				  break;
		case Ji:    		ji(token.value);   				  break;
		case Read:  		read();            				  break;
		case Write: 		write();           				  break;
		case End:			end();             				  break;
		case ArithmeticOp:  calculate(token.value); 		  break;
		case CmpOp:			compare((CmpValue)token.value);   break;
		case Comment:		skip();                 		  break;
		case Error:			error(); 						  break;
		case EndOfFile:		end();  						  break;

		case Atpow:			atpow(); 						  break;
		case Deg:			deg();  						  break;
		case Derivative:	derivative(); 					  break;
		case Value:			value(); 						  break;
		}
	}
public:
	Interpreter(ParsedProgram&& _program) : program(std::move(_program)) {}
	void run() {
		executable_token_index = 0;
		interpreting = true;

		while (interpreting) {
			execute_token(program.tokens[executable_token_index]);
		}
	}
};