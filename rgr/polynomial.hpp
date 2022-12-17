#pragma once

#include <iostream>
#include <fstream>

class Polynomial {
private:
	struct Term {
		float coefficient;
		int power;
	};

	Term* Terms;
	int count_terms;

	void alloc(int _count_terms);
public:

	/* многочлен P(x) = 0 в программе задаётся объектом со значениями поля count_terms = 0 ;
	степень такого многочлена считается равной нулю */

	Polynomial();
	Polynomial(float coeff);
	Polynomial(int power, float coeff);
	Polynomial(const Polynomial& other);
	Polynomial(Polynomial&& other) noexcept;
	~Polynomial();

	void clear();

	// реализация итератора
	class Iterator {
	private:
		Term* ptr;
	public:
		friend class Polynomial;
		Iterator(Term* _ptr = nullptr);
		Iterator(const Iterator&) = default;
		Iterator(Iterator&&) = default;
		~Iterator() = default;

		Term& operator*() const;
		bool operator==(const Iterator&) const;
		bool operator!=(const Iterator&) const;
		Iterator& operator++();
		Iterator operator++(int);
	};
	Iterator begin() const; 
	Iterator end() const;

	// перегрузка операторов
	Polynomial& operator =(const Polynomial& other);
	Polynomial& operator =(Polynomial&& other) noexcept;

	// нахождение значения многочлена при заданном x
	float operator ()(float x) const;

	// возвращает коэффициент при заданной степени
	float operator [](int power) const;

	Polynomial operator +(const Polynomial& added) const;
	Polynomial operator -() const;
	Polynomial operator -(const Polynomial& subbed) const;
	Polynomial operator *(const Polynomial& multed) const;
	Polynomial operator /(const Polynomial& divisor) const;
	Polynomial operator %(const Polynomial& divisor) const;

	bool operator ==(const Polynomial& polynomial) const;
	bool operator !=(const Polynomial& polynomial) const;

	// преобразование в bool
	operator bool();

	// степень многочлена
	int deg() const;

	// производная
	Polynomial derivative() const;

	/* ввод - вывод
	* формат ввода-вывода: [±0 : a0 ±1 : a1 ±2 : a2 ...] */

	friend std::ostream& operator <<(std::ostream& stream, const Polynomial& polynomial);
	friend std::istream& operator >>(std::istream& stream, Polynomial& polynomial);
};
