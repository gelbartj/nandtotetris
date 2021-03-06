#pragma once
#include <string>
#include <fstream>
#include <set>
#include "JackTokenizer.h"

class CompilationEngine
{
public:
	CompilationEngine(JackTokenizer *tkA, std::string& outputFilename, bool jsonMode = false); // must call compileClass
	void compileClass();
	void compileClassVarDec();
	void compileSubroutineDec();
	void compileParameterList(); // does not handle enclosing parens
	void compileSubroutineBody();
	void compileVarDec();
	void compileStatements(); // does not handle enclosing braces
	void compileLet();
	void compileIf();
	void compileWhile();
	void compileDo();
	void compileReturn();
	void compileExpression();
	void compileTerm(); // need extra lookahead to check for [, ( or .
	void compileExpressionList();
	void close();
	bool didFailOpen();
	~CompilationEngine();
private:
	std::ofstream outFile;
	bool failedOpen = false;
	JackTokenizer* tk;
	void writeTkAndAdvance();
	std::set<std::string> definedVars;
	void checkVarDec();
	// std::set<std::string> definedFuns;
	bool jsonMode;
	std::string makeOpenTag(NonTerminal, bool isList = false);
	std::string makeCloseTag(NonTerminal, bool isList = false);
	Status eat(Keyword keywordType, bool isOptional = false);
	Status eat(Token tokenType, bool isOptional = false);
	Status eat(char symbol, bool isOptional = false);
	Status eatType(bool includeVoid, bool isOptional);
	Status eatType(bool isOptional = false);
	Status eatTypeWithVoid(bool isOptional = false);
	Status eatOp(bool isOptional = false);
};

