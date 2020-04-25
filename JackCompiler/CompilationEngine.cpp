#include "CompilationEngine.h"
#include "Shared.h"
#include <iostream>
#include <random>
#include <optional>

using namespace std;

static const char* nonTerminal[] = { "class", "classVarDec", "subroutineDec", "parameterList", "subroutineBody", "varDec", "statements", "letStatement",	"ifStatement", "whileStatement", "doStatement", "returnStatement", "expression", "term", "expressionList", "identifier", "category", "idx", "isDefinition" };

static const char* tokens[] = { "keyword", "symbol", "identifier token", "int constant", "string constant", "none" };
static const char* kinds[] = { "static", "field", "arg", "var" };

static const char* brightError = "\x1B[91mERROR\033[0m";

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

// const set<char> ops = { '+', '-', '*', '/', '&', '|', '<', '>', '=' };
const map<char, Command> opMap = {
	{ '+', Command::ADD },
	{ '-', Command::SUB },
	{ '*', Command::MULT },
	{ '/', Command::DIV },
	{ '&', Command::AND },
	{ '|', Command::OR },
	{ '<', Command::LT },
	{ '>', Command::GT },
	{ '=', Command::EQ }
};

CompilationEngine::CompilationEngine(JackTokenizer* tkA, VMWriter* vmA) : tk(tkA), vm(vmA)
{
	tk->advance();
	if (tk->tokenType() == Token::KEYWORD && tk->keyword() == Keyword::CLASS)
		compileClass();
	else {
		cout << brightError << ": File does not begin with class declaration." << endl;
	}
}

Status CompilationEngine::eat(Token tokenType, bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
	if (tk->tokenType() != tokenType) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected " << tokens[static_cast<int>(tokenType)] << " and got " << tk->stringVal() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		if (tk->tokenType() == Token::INT_CONST)
			vm->writePush(Segment::CONST, tk->intVal());
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eat(char symbol, bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
	if (tk->symbol() != symbol) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected '" << symbol << "' and got '" << tk->stringVal() << "' instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eat(Keyword keywordType, bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
	if (tk->keyword() != keywordType) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected " << reverseKeywordList.at(keywordType) << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		switch (keywordType) {
		case Keyword::THIS: // only case I am aware of is "return this"
			vm->writePush(Segment::POINTER, 0);
			break;
		case Keyword::K_NULL: // fall-through
		case Keyword::FALSE:
			vm->writePush(Segment::CONST, 0);
			break;
		case Keyword::TRUE:
			vm->writePush(Segment::CONST, 1);
			vm->writeArithmetic(Command::NEG);
			break;
		}
		writeTkAndAdvance();
		return Status::OK;
	}
}

Status CompilationEngine::eatType(bool includeVoid, bool isOptional) {
	if (tk->aborted()) return Status::FAILURE;
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
		cout << brightError << " at line " << tk->currPos() << ": Expected type" << endl;
		return Status::SYNTAX_ERROR;
	}
}

Status CompilationEngine::eatType(bool isOptional) {
	return eatType(false, isOptional);
}

Status CompilationEngine::eatTypeWithVoid(bool isOptional) {
	return eatType(true, isOptional);
}

Status CompilationEngine::eatOp(bool isOptional, bool noAdvance) {
	if (tk->aborted()) return Status::FAILURE;
	auto opIt = opMap.find(tk->symbol());
	if (opIt == opMap.end()) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk->currPos() << ": Expected operator. Got " << tk->stringVal() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		if (!noAdvance)
			writeTkAndAdvance();
		return Status::OK;
	}
}


void CompilationEngine::writeTkAndAdvance() {
	if (!tk->aborted()) {
		// tk->writeCurrToken(outFile, jsonMode);
		tk->advance();
	}
}

/*
string CompilationEngine::makeOpenTag(NonTerminal nt, bool isList) {
	string tagName = nonTerminal[static_cast<int>(nt)];
	return (jsonMode ? "\"" + tagName + "\": " + (isList ? "[" : "{") : "<" + tagName + ">") + "\n";
}

string CompilationEngine::makeCloseTag(NonTerminal nt, bool isList) {
	string tagName = nonTerminal[static_cast<int>(nt)];
	return (jsonMode ? (isList ? "]," : "},") : "</" + tagName + ">") + "\n";
}
*/

void CompilationEngine::compileClass()
{
	// outFile << makeOpenTag(NonTerminal::CLASS);
	classTable.reset();
	eat(Keyword::CLASS);
	className = tk->stringVal();
	compileIdentifier(true, false);
	eat('{');
	if (tk->tokenType() == Token::KEYWORD) {
		while (!tk->aborted() && (tk->keyword() == Keyword::STATIC || tk->keyword() == Keyword::FIELD)) {
			compileClassVarDec();
		}
		while (!tk->aborted() && (tk->keyword() == Keyword::CONSTRUCTOR || tk->keyword() == Keyword::FUNCTION || tk->keyword() == Keyword::METHOD)) {
			compileSubroutineDec();
		}
	}
	eat('}');
	// outFile << makeCloseTag(NonTerminal::CLASS);
}

void CompilationEngine::checkVarDec(bool isClass) {
	auto found = (isClass ? classTable : subroutineTable).getTable().find(tk->stringVal());
	auto end = (isClass ? classTable : subroutineTable).getTable().end();
	if (found != end) {
		cout << brightError << " at line " << tk->currPos() << ": Attempting redefinition of already defined variable \"" << tk->stringVal() << "\"" << endl;
	}
}

Kind keyToKind(Keyword key) {
	switch (key) {
	case Keyword::STATIC:
		return Kind::STATIC;
	case Keyword::VAR:
		return Kind::VAR;
	case Keyword::FIELD:
		return Kind::FIELD;
	default:
		return Kind::ARG; // parameter in subroutine declaration
	}
}

Segment kindToSegment(Kind kind) {
	switch (kind) {
	case Kind::ARG:
		return Segment::ARG;
	case Kind::FIELD:
		return Segment::THIS;
	case Kind::STATIC:
		return Segment::STATIC;
	case Kind::VAR:
		return Segment::LOCAL;
	default:
		return Segment::NONE;
	}
}

void CompilationEngine::compileClassVarDec()
{
	// outFile << makeOpenTag(NonTerminal::CLASS_VAR_DEC);
	Kind kind = keyToKind(tk->keyword());
	eat(Token::KEYWORD);
	string type = tk->stringVal();
	eatType();
	checkVarDec(true);
	classTable.define(tk->stringVal(), type, kind);
	compileIdentifier(true, false);
	while (eat(',', true) == Status::OK) {
		checkVarDec(true);
		classTable.define(tk->stringVal(), type, kind);
		compileIdentifier(true, false);
	}
	eat(';');
	// outFile << makeCloseTag(NonTerminal::CLASS_VAR_DEC);
}

void CompilationEngine::compileSubroutineDec()
{
	// outFile << makeOpenTag(NonTerminal::SUBROUTINE_DEC);
	subroutineTable.reset();
	Keyword key = tk->keyword();
	eat(Token::KEYWORD);
	eatTypeWithVoid();
	string funName = tk->stringVal();
	compileIdentifier(true, true);
	eat('(');
	compileParameterList(key == Keyword::METHOD);
	eat(')');
	compileSubroutineBody(funName);
	// outFile << makeCloseTag(NonTerminal::SUBROUTINE_DEC);
}

void CompilationEngine::compileParameterList(bool isMethod)
{
	// outFile << makeOpenTag(NonTerminal::PARAMETER_LIST, true);
	if (isMethod) {
		string thisStr = "this"; // define requires string reference
		subroutineTable.define(thisStr, className, Kind::ARG);
	}
	string type = tk->stringVal();
	if (eatType(true) == Status::OK) {
		subroutineTable.define(tk->stringVal(), type, Kind::ARG);
		compileIdentifier(true, false); // parameter list args are ARG variable declarations
		while (eat(',', true) == Status::OK) {
			type = tk->stringVal();
			eatType();
			subroutineTable.define(tk->stringVal(), type, Kind::ARG);
			compileIdentifier(true, false);
		}
	}
	// else do nothing. list can be empty
	// outFile << makeCloseTag(NonTerminal::PARAMETER_LIST, true);
}

void CompilationEngine::compileSubroutineBody(string& funName)
{
	// outFile << makeOpenTag(NonTerminal::SUBROUTINE_BODY);
	int localCount = 0;
	eat('{');
	while (!tk->aborted() && tk->tokenType() == Token::KEYWORD && tk->keyword() == Keyword::VAR) {
		++localCount;
		compileVarDec();
	}
	vm->writeFunction(className + "." + funName, localCount);
	compileStatements();
	eat('}');
	// outFile << makeCloseTag(NonTerminal::SUBROUTINE_BODY);
}

void CompilationEngine::compileVarDec()
{
	// outFile << makeOpenTag(NonTerminal::VAR_DEC);
	eat(Keyword::VAR);
	string type = tk->stringVal();
	eatType();
	checkVarDec(false);
	classTable.define(tk->stringVal(), type, Kind::VAR);
	compileIdentifier(true, false);
	while (eat(',', true) == Status::OK) {
		checkVarDec(false);
		classTable.define(tk->stringVal(), type, Kind::VAR);
		compileIdentifier(true, false);
	}
	eat(';');
	// outFile << makeCloseTag(NonTerminal::VAR_DEC);
}

void CompilationEngine::compileStatements()
{
	// outFile << makeOpenTag(NonTerminal::STATEMENTS);
	bool endFlag = false;
	while (!tk->aborted() && tk->tokenType() == Token::KEYWORD && !endFlag) {
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
		default:
			endFlag = true;
			break;
		}
		// cout << "Current token is " << tk->stringVal() << endl;
	}
	// outFile << makeCloseTag(NonTerminal::STATEMENTS);
}

void CompilationEngine::compileLet()
{
	eat(Keyword::LET);
	auto classIt = classTable.getTable().find(tk->stringVal());
	auto subIt = subroutineTable.getTable().find(tk->stringVal());
	bool isSubVar = subIt != subroutineTable.getTable().end();
	if (classIt == classTable.getTable().end() && !isSubVar) {
		cout << brightError << " at line " << tk->currPos() << ": Attempting to assign value to undeclared variable \"" << tk->stringVal() << "\"" << endl;
		return;
	}
	compileIdentifier(false, false);
	if (eat('[', true) == Status::OK) {
		compileExpression();
		eat(']');
	}
	eat('=');
	compileExpression();
	// after returning from compileExpression, result of expression will be at the top of the stack
	vm->writePop(kindToSegment((isSubVar ? subIt : classIt)->second.kind), (isSubVar ? subIt : classIt)->second.idx);
	eat(';');
}

std::string random_string(std::size_t length)
{
	const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	std::random_device random_device; // not the best seed but fine for our purposes
	std::mt19937 generator(random_device());
	std::uniform_int_distribution<> distribution(0, CHARACTERS.size() - 1);

	std::string random_string;

	for (std::size_t i = 0; i < length; ++i)
	{
		random_string += CHARACTERS[distribution(generator)];
	}

	return random_string;
}

void CompilationEngine::compileIf()
{
	string rand = random_string(20);
	eat(Keyword::IF);
	eat('(');
	compileExpression();
	eat(')');
	// true or false value will be on the stack
	vm->writeArithmetic(Command::NOT);
	vm->writeIf(rand);
	eat('{');
	compileStatements();
	eat('}');
	vm->writeLabel(rand);
	if (eat(Keyword::ELSE, true) == Status::OK) {
		eat('{');
		compileStatements();
		eat('}');
	}
	// outFile << makeCloseTag(NonTerminal::IF);
}

void CompilationEngine::compileWhile()
{
	// outFile << makeOpenTag(NonTerminal::WHILE);
	eat(Keyword::WHILE);
	eat('(');
	compileExpression();
	eat(')');
	eat('{');
	compileStatements();
	eat('}');
	// outFile << makeCloseTag(NonTerminal::WHILE);
}

void CompilationEngine::compileDo()
{
	eat(Keyword::DO);
	string subroutineName = tk->stringVal();
	tk->advance();
	if (eat('.', true) == Status::OK) {
		subroutineName += "." + tk->stringVal();
		tk->advance();
	}
	eat('(');
	int args = compileExpressionList();
	eat(')');
	compileIdentifier(false, true, subroutineName, args);
	eat(';');
}

void CompilationEngine::compileReturn()
{
	// outFile << makeOpenTag(NonTerminal::RETURN);
	eat(Keyword::RETURN);
	if (eat(';', true) == Status::OK) {
		vm->writeReturn();
		return;
	}
	compileExpression();
	eat(';');
	vm->writeReturn();
}

void CompilationEngine::compileExpression()
{
	compileTerm(); // push onto stack
	while (eatOp(true, true) == Status::OK) {
		Command op = opMap.find(tk->symbol())->second;
		tk->advance();
		compileTerm(); // push onto stack
		vm->writeArithmetic(op);
	}
}

static bool isKeywordConst(Keyword key) {
	return (key == Keyword::TRUE || key == Keyword::FALSE || key == Keyword::K_NULL || key == Keyword::THIS);
}

void CompilationEngine::compileTerm()
{
	if (tk->tokenType() == Token::IDENTIFIER) {
		compileTermIdentifier();
	}
	else if (eat(Token::INT_CONST, true) == Status::OK) {} // done
	else if (eat(Token::STRING_CONST, true) == Status::OK) {}
	else if (tk->tokenType() == Token::KEYWORD && isKeywordConst(tk->keyword())) {
		eat(Token::KEYWORD);
	}
	else if (eat('(', true) == Status::OK) { // done
		compileExpression();
		eat(')');
	}
	else if (eat('-', true) == Status::OK) { // done
		compileTerm();
		vm->writeArithmetic(Command::NEG);
	}
	else if (eat('~', true) == Status::OK) { // done
		compileTerm();
		vm->writeArithmetic(Command::NOT);
	}

	// outFile << makeCloseTag(NonTerminal::TERM);
}

int CompilationEngine::compileExpressionList()
{
	int argCount = 0;
	if (tk->tokenType() == Token::SYMBOL && tk->symbol() == ')') {
		return argCount;
	}
	compileExpression();
	++argCount;
	while (eat(',', true) == Status::OK) {
		compileExpression();
		++argCount;
	}
	return argCount;
}

/*
void CompilationEngine::compileIdentifier(bool beingDefined, bool isSubroutine) {
	compileIdentifier(beingDefined, isSubroutine, std::nullopt);
}
*/

void CompilationEngine::compileIdentifier(bool beingDefined, bool isSubroutine, const string& savedToken, int argCount)
{
	string token = (savedToken.length() > 0) ? savedToken : tk->stringVal();

	if (isSubroutine && !beingDefined) {
		// cout << "Writing call with token " << token << endl;
		vm->writeCall(token, argCount);
		if (savedToken.length() == 0) tk->advance();
		return;
	}

	auto localVar = subroutineTable.getTable().find(token);
	auto classVar = classTable.getTable().find(token);
	bool isLocal = localVar != subroutineTable.getTable().end();
	if (isLocal || classVar != classTable.getTable().end()) {
		if (beingDefined) {
			vm->writePop(kindToSegment((isLocal ? localVar : classVar)->second.kind), (isLocal ? localVar : classVar)->second.idx);
		}
		else vm->writePush(kindToSegment((isLocal ? localVar : classVar)->second.kind), (isLocal ? localVar : classVar)->second.idx);
	}	
	else { // is class
		// cout << "Is class" << endl;
		// outFile << "class";
	}
	if (savedToken.length() == 0) tk->advance();
}

void CompilationEngine::compileTermIdentifier() {
	// tk->writeCurrToken(outFile);
	string token = tk->stringVal();
	tk->advance();
	if (tk->tokenType() == Token::SYMBOL && tk->symbol() == '[') {
		compileIdentifier(false, false, token);
		eat('[');
		compileExpression();
		eat(']');
	}
	else if (tk->tokenType() == Token::SYMBOL && tk->symbol() == '(') {
		eat('(');
		int args = compileExpressionList(); // this will push needed arguments for subroutine call onto the stack and return count
		eat(')');
		compileIdentifier(false, true, token, args);
	}
	else if (tk->tokenType() == Token::SYMBOL && tk->symbol() == '.') {
		cout << "Class method call in termidentifier" << endl;
		eat('.');
		token = token + "." + tk->stringVal();
		eat(Token::IDENTIFIER);
		eat('(');
		int args = compileExpressionList();
		eat(')');
		compileIdentifier(false, false, token, args);	// needs to be able to handle Foo.bar() call	
	}
	else {
		compileIdentifier(false, false, token); // regular variable
	}
}

CompilationEngine::~CompilationEngine()
{
}