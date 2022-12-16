#include <iostream>
#include "interpreter.cpp"

/* 	программа анализирует файл с программой и создаёт три файла, в которые записывается результат:
*
*	pinput_raw    :    полный список обнаруженных лексем и таблица переменных
*
*   pinput_exe    :    список лексем, которые могут быть исполнены интерпретатором 
*   				   с указанием имён переменных и констант, на которые ссылаются некоторые лексмеы,
*   				   также в конце записывается таблица переменных
*
*   pinput_err    :    список номеров строк, в которых найдены ошибки
*
*   если программа корректная, то она интерпретируется
*/
int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cout << "Нужен файл для трансляции...\n";
		return 1;
	}

	/* запуск лексичского анализатора */
	Parser parser;
	ParsedProgram program = parser.run(argv[1]);

	/* запись всех лексем и таблицы имён */
	std::ofstream fout("pinput_raw");

	program.print_tokens(fout);
	fout << std::endl;

	program.print_names(fout);
	fout << std::endl;

	fout.close();

	/* запись только исполняемых лексем (с именами переменных) */
	fout.open("pinput_exe");

	program.print_executable_tokens(fout);
	fout << std::endl;

	program.print_names(fout);
	fout << std::endl;

	fout.close();

	/* запись ошибок */
	fout.open("pinput_err");

	int errors_count = program.print_errors(fout);
	fout << std::endl;

	fout.close();

	/* интрерпретация */
	if (errors_count != 0) {
		std::cout << "Программа содержит ошибки...\nНомера строк с ошибками можно посмотреть в файле 'pinput_err'" << std::endl;
		return 0;
	}

	Interpreter interpreter(std::move(program));
	interpreter.run();
	
	return 0;
}