#pragma once
#include "JackTokenizer.h"
#include "SymbolTable.h"
#include "VMWriter.h"
#include <string>
#include <fstream>
#include <set>
#include <map>
#include <tuple>

// enum class ItReturnType { LOCAL = 0, CLASS, NONE };
typedef std::tuple<std::map<std::string, const STEntry>::iterator, bool> ItWithResult;

class CompilationEngine
{
public:
	CompilationEngine(JackTokenizer *tkA, VMWriter *vmA); // must call compileClass
	void compileClass();
	void compileClassVarDec();
	void compileSubroutineDec();
	void compileParameterList(bool isMethod = false); // does not handle enclosing parens
	void compileSubroutineBody(std::string& funName, std::string& type, Keyword key);
	void compileVarDec();
	void compileStatements(const std::string& type = std::string()); // does not handle enclosing braces
	void compileLet();
	void compileIf();
	void compileWhile();
	void compileDo();
	void compileReturn(const std::string& type);
	void compileExpression();
	void compileTerm(); // need extra lookahead to check for [, ( or .
	int compileExpressionList();
	void compileIdentifier(bool beingDefined, bool isSubroutine, const std::string& savedToken = std::string(), int argCount = 0);
	void compileTermIdentifier();
	~CompilationEngine();
private:
	std::string className;
	bool failedOpen = false;
	JackTokenizer* tk;
	VMWriter* vm;
	void writeTkAndAdvance();
	void checkVarDec(bool isClass);
	ItWithResult getVarIt(const std::string& name);
	bool prepareMethod(std::string& token, bool sureMethod = false);
	bool isDefined(std::string var);
	Status eat(Keyword keywordType, bool isOptional = false);
	Status eat(Token tokenType, bool isOptional = false);
	Status eat(char symbol, bool isOptional = false);
	Status eatType(bool includeVoid, bool isOptional);
	Status eatType(bool isOptional = false);
	Status eatTypeWithVoid(bool isOptional = false);
	Status eatOp(bool isOptional = false, bool noAdvance = false);
	SymbolTable classTable;
	SymbolTable subroutineTable;
	std::set<std::string> methodList;
};

