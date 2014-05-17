#ifndef CODE_EMITTER_HPP
#define CODE_EMITTER_HPP

#include <iostream>

namespace b2 {

class CodeEmitter
{
public:
    CodeEmitter(std::ostream& output, std::string indentation = "\t") : m_output(output), m_indentationCount(0), m_indentation(indentation) {}

	CodeEmitter& indent() {
		m_indentationCount++;
		return *this;
	}
	CodeEmitter& outdent() {
		m_indentationCount--;
		return *this;
	}

	CodeEmitter& start_line() {
		for (int i=0; i < m_indentationCount; i++) {
			m_output << m_indentation;
		}
		return *this;
	}

	CodeEmitter& end_line() {
		m_output << std::endl;
		return *this;
	}

	CodeEmitter& line(std::string text) {
		this->start_line();
		m_output << text;
		this->end_line();
		return *this;
	}

	CodeEmitter& blankline() {
		m_output << std::endl;
		return *this;
	}

	CodeEmitter& operator<<(const char* text) {
		m_output << text;
		return *this;
	}

    CodeEmitter& operator<<(std::string text) {
		m_output << text;
		return *this;
	}

	// this is a no-op function, used in constructs where a function
	// writes to CodeEmitter immediately and thus shouldn't return anything
	// (see JavascriptVisitor::expression() for an example)
	CodeEmitter& operator<<(CodeEmitter&) {
		return *this;
	}

private:
	std::ostream& m_output;
	int m_indentationCount;
	std::string m_indentation;
};

}

#endif // CODE_EMITTER_HPP
