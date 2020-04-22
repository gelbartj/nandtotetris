#include "CompilationEngine.h"
#include "Shared.h"
#include <iostream>

using namespace std;

// const char* nonTerminal[] = { "class", "classVarDec", "subroutineDec", "parameterList", "subroutineBody", "varDec", "statements", "letStatement", 
// "ifStatement", "whileStatement", "doStatement", "returnStatement", "expression", "term", "expressionList" };
// for reference only

static const char* tokens[] = { "keyword", "symbol", "identifier", "int constant", "string constant", "none" };

const map<Keyword, string> reverseKeywordList = {
	{Keyword::CLASS, "class"},
	{Keyword::METHOD, "method"},
	{Keyword::FUNCTION, "function"},
	{Keyword::CONSTRUCTOR, "constructor" },
	{Keyword::INT, "int" },
	{Keyword::BOOLEAN, "boolean" },
	{Keyword::CHAR, "char" },
	{Keyword::VOID, "void" },
	{Keyword::VAR, "var" },
	{Keyword::STATIC, "static" },
	{Keyword::FIELD, "field" },
	{Keyword::LET, "let" },
	{Keyword::DO, "do" },
	{Keyword::IF, "if" },
	{Keyword::ELSE, "else" },
	{Keyword::WHILE, "while" },
	{Keyword::RETURN, "return" },
	{Keyword::TRUE, "true" },
	{Keyword::FALSE, "false" },
	{Keyword::K_NULL, "null" },
	{Keyword::THIS, "this" }
};

const set<char> ops = { '+', '-', '*', '/', '&', '|', '<', '>', '=' };

CompilationEngine::CompilationEngine(JackTokenizer* tkA, std::string& outputFilename)
{
	outFile.open(outputFilename);
	if (outFile.is_open()) {
		cout << "Opened file " << outputFilename << " for detailed XML output." << endl;
	}
	else {
		cout << "Error opening file " << outputFilename << " for detailed output." << endl;
		failedOpen = true;
	}
	tk = tkA;
	tk->advance();
	if (tk->tokenType() == Token::KEYWORD && tk->keyword() == Keyword::CLASS)
		compileClass();
	else {
		cout << "ERROR! File does not begin with class declaration." << endl;
	}
}

Status CompilationEngine::eat(Token tokenType, bool isOptional) {
	if (tk->tokenType() != tokenType) {
		if (isOptional) return Status::NOT_FOUND;
		cout << "Error at " << tk->currPos() << ", expected " << tokens[static_cast<int>(tokenType)] << ". Got " << tk->stringVal() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eat(char symbol, bool isOptional) {
	if (tk->symbol() != symbol) {
		if (isOptional) return Status::NOT_FOUND;
		cout << "Error at " << tk->currPos() << ", expected " << symbol << ". Got " << tk->stringVal() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eat(Keyword keywordType, bool isOptional) {
	if (tk->keyword() != keywordType) {
		if (isOptional) return Status::NOT_FOUND;
		cout << "Error at " << tk->currPos() << ", expected " << reverseKeywordList.at(keywordType) << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eatType(bool includeVoid, bool isOptional) {
	if ((tk->tokenType() == Token::KEYWORD && 
		(tk->keyword() == Keyword::INT || 
			tk->keyword() == Keyword::BOOLEAN || 
			tk->keyword() == Keyword::CHAR || 
			(includeVoid ? tk->keyword() == Keyword::VOID : false)))
		|| tk->tokenType() == Token::IDENTIFIER) {
		writeTkAndAdvance();
		return Status::OK;
	}
	else {
		if (isOptional) return Status::NOT_FOUND;
		cout << "Error at " << tk->currPos() << ", expected type" << endl;
		return Status::SYNTAX_ERROR;
	}
}

Status CompilationEngine::eatType(bool isOptional) {
	return eatType(false, isOptional);
}

Status CompilationEngine::eatTypeWithVoid(bool isOptional) {
	return eatType(true, isOptional);
}

Status CompilationEngine::eatOp(bool isOptional) {
	auto opIt = tk->tokenType() == Token::SYMBOL ? ops.find(tk->symbol()) : ops.end();
	if (opIt == ops.end()) {
		if (isOptional) return Status::NOT_FOUND;
		cout << "Error at " << tk->currPos() << ", expected operator. Got " << tk->stringVal() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		writeTkAndAdvance();
		return Status::OK;
	}
}


void CompilationEngine::writeTkAndAdvance() {
	tk->writeCurrToken(outFile);
	tk->advance();
}

void CompilationEngine::compileClass()
{
	outFile << "<class>" << endl;
	eat(Keyword::CLASS);
	eat(Token::IDENTIFIER);
	eat('{');
	if (tk->tokenType() == Token::KEYWORD) {
		while (tk->keyword() == Keyword::STATIC || tk->keyword() == Keyword::FIELD) {
			compileClassVarDec();	
		}
		while (tk->keyword() == Keyword::CONSTRUCTOR || tk->keyword() == Keyword::FUNCTION || tk->keyword() == Keyword::METHOD) {
			compileSubroutineDec();
		}
	}
	eat('}');
	outFile << "</class>";
}

void CompilationEngine::compileClassVarDec()
{
	outFile << "<classVarDec>" << endl;
	eat(Token::KEYWORD);
	eatType();
	eat(Token::IDENTIFIER);
	while (eat(',', true) == Status::OK) {
		eat(Token::IDENTIFIER);
	}
	eat(';');
	outFile << "</classVarDec>" << endl;
}

void CompilationEngine::compileSubroutineDec()
{
	outFile << "<subroutineDec>" << endl;
	eat(Token::KEYWORD);
	eatTypeWithVoid();
	eat(Token::IDENTIFIER);
	eat('(');
	compileParameterList();
	eat(')');
	compileSubroutineBody();
	outFile << "</subroutineDec>" << endl;
}

void CompilationEngine::compileParameterList()
{
	outFile << "<parameterList>" << endl;
	if (eatType(true) == Status::OK) {
		eat(Token::IDENTIFIER);
		while (eat(',', true) == Status::OK) {
			eatType();
			eat(Token::IDENTIFIER);
		}
	}
	// list can be empty
	outFile << "</parameterList>" << endl; 
}

void CompilationEngine::compileSubroutineBody()
{
	outFile << "<subroutineBody>" << endl;
	eat('{');
	while (tk->tokenType() == Token::KEYWORD && tk->keyword() == Keyword::VAR) {
		compileVarDec();
	}
	compileStatements();
	eat('}');
	outFile << "</subroutineBody>" << endl;
}

void CompilationEngine::compileVarDec()
{
	outFile << "<varDec>" << endl;
	eat(Keyword::VAR);
	eatType();
	eat(Token::IDENTIFIER); // later, add logic here to insert this variable into a symbol table
	while (eat(',', true) == Status::OK) {
		eat(Token::IDENTIFIER);
	}
	eat(';');
	outFile << "</varDec>" << endl;
}

void CompilationEngine::compileStatements()
{
	outFile << "<statements>" << endl;
	while (tk->tokenType() == Token::KEYWORD) {
		switch (tk->keyword()) {
			case Keyword::LET:
				compileLet();
				break;
			case Keyword::IF:
				compileIf();
				break;
			case Keyword::WHILE:
				compileWhile();
				break;
			case Keyword::DO:
				compileDo();
				break;
			case Keyword::RETURN:
				compileReturn();
				break;
		}
		// cout << "Current token is " << tk->stringVal() << endl;
	}
	outFile << "</statements>" << endl;
}

void CompilationEngine::compileLet()
{
	outFile << "<letStatement>" << endl;
	eat(Keyword::LET);
	eat(Token::IDENTIFIER);
	if (eat('[', true) == Status::OK) {
		compileExpression();
		eat(']');
	}
	eat('=');
	compileExpression();
	eat(';');
	outFile << "</letStatement>" << endl;
}

void CompilationEngine::compileIf()
{
	outFile << "<ifStatement>" << endl;
	eat(Keyword::IF);
	eat('(');
	compileExpression();
	eat(')');
	eat('{');
	compileStatements();
	eat('}');
	if (eat(Keyword::ELSE, true) == Status::OK) {
		eat('{');
		compileStatements();
		eat('}');
	}
	outFile << "</ifStatement>" << endl;
}

void CompilationEngine::compileWhile()
{
	outFile << "<whileStatement>" << endl;
	eat(Keyword::WHILE);
	eat('(');
	compileExpression();
	eat(')');
	eat('{');
	compileStatements();
	eat('}');
	outFile << "</whileStatement>" << endl;
}

void CompilationEngine::compileDo()
{
	outFile << "<doStatement>" << endl;
	eat(Keyword::DO);
	eat(Token::IDENTIFIER);
	if (eat('.', true) == Status::OK) {
		eat(Token::IDENTIFIER);
	}
	eat('(');
	compileExpressionList();
	eat(')');
	eat(';');
	outFile << "</doStatement>" << endl;
}

void CompilationEngine::compileReturn()
{
	outFile << "<returnStatement>" << endl;
	eat(Keyword::RETURN);
	if (eat(';', true) == Status::OK) {
		outFile << "</returnStatement>" << endl;
		return;
	}
	compileExpression();
	eat(';');
	outFile << "</returnStatement>" << endl;
}

void CompilationEngine::compileExpression()
{
	outFile << "<expression>" << endl;
	compileTerm();
	while (eatOp(true) == Status::OK) {
		compileTerm();
	}
	outFile << "</expression>" << endl;
}

bool isKeywordConst(Keyword key) {
	return (key == Keyword::TRUE || key == Keyword::FALSE || key == Keyword::K_NULL || key == Keyword::THIS);
}

void CompilationEngine::compileTerm()
{
	outFile << "<term>" << endl;
	if (eat(Token::IDENTIFIER, true) == Status::OK) {
		if (eat('[', true) == Status::OK) {
			compileExpression();
			eat(']');
		}
		else if (eat('(', true) == Status::OK) {
			compileExpressionList();
			eat(')');
		}
		else if (eat('.', true) == Status::OK) {
			eat(Token::IDENTIFIER);
			eat('(');
			compileExpressionList();
			eat(')');
		}
	}
	else if (eat(Token::INT_CONST, true) == Status::OK) {}
	else if (eat(Token::STRING_CONST, true) == Status::OK) {}
	else if (tk->tokenType() == Token::KEYWORD && isKeywordConst(tk->keyword())) {
		eat(Token::KEYWORD);
	}
	else if (eat('(', true) == Status::OK) {
		compileExpression();
		eat(')');
	}
	else if (eat('-', true) == Status::OK || eat('~', true) == Status::OK) {
		compileTerm();
	}

	outFile << "</term>" << endl;
}

void CompilationEngine::compileExpressionList()
{
	outFile << "<expressionList>" << endl;
	if (tk->tokenType() == Token::SYMBOL && tk->symbol() == ')') {
		outFile << "</expressionList>" << endl;
		return;
	}
	compileExpression();
	while (eat(',', true) == Status::OK) {
		compileExpression();
	}
	outFile << "</expressionList>" << endl;
}

void CompilationEngine::close()
{
	if (outFile.is_open()) {
		cout << "Closing file." << endl;
		outFile.close();
	}
}

bool CompilationEngine::didFailOpen()
{
	return failedOpen;
}

CompilationEngine::~CompilationEngine()
{
}