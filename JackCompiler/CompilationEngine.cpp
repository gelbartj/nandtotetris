#include "CompilationEngine.h"
#include "Shared.h"
#include <iostream>
#include <random>

using namespace std;

static const char* nonTerminal[] = { "class", "classVarDec", "subroutineDec", "parameterList", "subroutineBody", "varDec", "statements", "letStatement",	"ifStatement", "whileStatement", "doStatement", "returnStatement", "expression", "term", "expressionList", "identifier", "category", "idx", "isDefinition" };

static const char* tokens[] = { "keyword", "symbol", "identifier token", "int constant", "string constant", "none" };
// static const char* kinds[] = { "static", "field", "arg", "var" };

static const char* brightError = "\x1B[91mERROR\033[0m";

static const map<Keyword, const string> reverseKeywordList = {
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

static const map<char, const Command> opMap = {
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

CompilationEngine::CompilationEngine(JackTokenizer& tkA, VMWriter& vmA) : tk(tkA), vm(vmA)
{
	tk.advance();
	if (tk.tokenType() == Token::KEYWORD && tk.keyword() == Keyword::CLASS)
		compileClass();
	else {
		cout << brightError << ": File does not begin with class declaration." << endl;
	}
}

Status CompilationEngine::eat(Token tokenType, bool isOptional) {
	if (tk.aborted()) return Status::FAILURE;
	if (tk.tokenType() != tokenType) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk.currLineNum() << ": Expected " << tokens[static_cast<int>(tokenType)] << " and got " << tk.identifier() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		if (tk.tokenType() == Token::INT_CONST)
			vm.writePush(Segment::CONST, tk.intVal());
		else if (tk.tokenType() == Token::STRING_CONST) {
			string stringConst = tk.stringVal();
			size_t length = stringConst.length();
			vm.writePush(Segment::CONST, length);
			vm.writeCall("String.new", 1);
			for (size_t i = 0; i < length; ++i) {
				vm.writePush(Segment::CONST, stringConst.at(i));
				vm.writeCall("String.appendChar", 2);
			}
		}
		else if (tk.tokenType() == Token::KEYWORD) {
			switch (tk.keyword()) {
			case Keyword::THIS: // only case I am aware of is "return this"
				vm.writePush(Segment::POINTER, 0);
				break;
			case Keyword::K_NULL: // fall-through
			case Keyword::FALSE:
				vm.writePush(Segment::CONST, 0);
				break;
			case Keyword::TRUE:
				vm.writePush(Segment::CONST, 1);
				vm.writeArithmetic(Command::NEG);
				break;
			}
		}
		advanceTk();
		return Status::OK;
	}
}

Status CompilationEngine::eat(char symbol, bool isOptional) {
	if (tk.aborted()) return Status::FAILURE;
	if (tk.symbol() != symbol) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk.currLineNum() << ": Expected '" << symbol << "' and got '" << tk.identifier() << "' instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		advanceTk();
		return Status::OK;
	}
}

Status CompilationEngine::eat(Keyword keywordType, bool isOptional) {
	if (tk.aborted()) return Status::FAILURE;
	if (tk.keyword() != keywordType) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk.currLineNum() << ": Expected " << reverseKeywordList.at(keywordType) << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		advanceTk();
		return Status::OK;
	}
}

Status CompilationEngine::eatType(bool includeVoid, bool isOptional) {
	if (tk.aborted()) return Status::FAILURE;
	if ((tk.tokenType() == Token::KEYWORD &&
			(tk.keyword() == Keyword::INT ||
			tk.keyword() == Keyword::BOOLEAN ||
			tk.keyword() == Keyword::CHAR ||
			(includeVoid ? tk.keyword() == Keyword::VOID : false)))
		|| tk.tokenType() == Token::IDENTIFIER) {
		advanceTk();
		return Status::OK;
	}
	else {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk.currLineNum() << ": Expected type" << endl;
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
	if (tk.aborted()) return Status::FAILURE;
	auto opIt = opMap.find(tk.symbol());
	if (opIt == opMap.end()) {
		if (isOptional) return Status::NOT_FOUND;
		cout << brightError << " at line " << tk.currLineNum() << ": Expected operator. Got " << tk.identifier() << " instead." << endl;
		return Status::SYNTAX_ERROR;
	}
	else {
		if (!noAdvance)
			advanceTk();
		return Status::OK;
	}
}


void CompilationEngine::advanceTk() {
	if (!tk.aborted()) {
		tk.advance();
	}
}

void CompilationEngine::compileClass()
{
	classTable.reset();
	eat(Keyword::CLASS);
	className = tk.identifier();
	compileIdentifier(true, false);
	eat('{');
	if (tk.tokenType() == Token::KEYWORD) {
		while (!tk.aborted() && (tk.keyword() == Keyword::STATIC || tk.keyword() == Keyword::FIELD)) {
			compileClassVarDec();
		}
		while (!tk.aborted() && (tk.keyword() == Keyword::CONSTRUCTOR || tk.keyword() == Keyword::FUNCTION || tk.keyword() == Keyword::METHOD)) {
			compileSubroutineDec();
		}
	}
	eat('}');
}

void CompilationEngine::checkVarDec(bool isClass) {
	auto found = (isClass ? classTable : subroutineTable).getTable().find(tk.identifier());
	auto end = (isClass ? classTable : subroutineTable).getTable().end();
	if (found != end) {
		cout << brightError << " at line " << tk.currLineNum() << ": Attempting redefinition of already defined variable \"" << tk.identifier() << "\"" << endl;
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
	Kind kind = keyToKind(tk.keyword());
	eat(Token::KEYWORD);
	string type = tk.identifier();
	eatType();
	checkVarDec(true);
	classTable.define(tk.identifier(), type, kind);
	eat(Token::IDENTIFIER);
	while (eat(',', true) == Status::OK) {
		checkVarDec(true);
		classTable.define(tk.identifier(), type, kind);
		eat(Token::IDENTIFIER);
	}
	eat(';');
}

void CompilationEngine::compileSubroutineDec()
{
	subroutineTable.reset();
	Keyword key = tk.keyword();
	eat(Token::KEYWORD);
	string classNameOrType = tk.identifier();
	eatTypeWithVoid();
	string funName = tk.identifier();
	eat(Token::IDENTIFIER);

	eat('(');
	compileParameterList(key == Keyword::METHOD);
	eat(')');
	compileSubroutineBody(funName, classNameOrType, key); // push pointer 0 at end is already captured in "return this" statement
													// for constructors
}

void CompilationEngine::compileParameterList(bool isMethod)
{
	string type = tk.identifier();
	int numArgs = 0;
	if (isMethod) {
		subroutineTable.define("this", className, Kind::ARG);
		++numArgs;
	}
	if (eatType(true) == Status::OK) {
		subroutineTable.define(tk.identifier(), type, Kind::ARG);
		eat(Token::IDENTIFIER);
		++numArgs;
		while (eat(',', true) == Status::OK) {
			type = tk.identifier();
			eatType();
			subroutineTable.define(tk.identifier(), type, Kind::ARG);
			eat(Token::IDENTIFIER);
			++numArgs;
		}
	}
	// else do nothing. list can be empty
}

void CompilationEngine::compileSubroutineBody(string& funName, string& type, Keyword key)
{
	eat('{');
	while (!tk.aborted() && tk.tokenType() == Token::KEYWORD && tk.keyword() == Keyword::VAR) {
		compileVarDec();
	}
	vm.writeFunction(className + "." + funName, subroutineTable.getKindCount(Kind::VAR));
	if (key == Keyword::CONSTRUCTOR) {
		vm.writePush(Segment::CONST, static_cast<int>(classTable.getKindCount(Kind::FIELD))); // don't include static vars
		vm.writeCall("Memory.alloc", 1);
		vm.writePop(Segment::POINTER, 0);
	}
	else if (key == Keyword::METHOD) {
		vm.writePush(Segment::ARG, 0);
		vm.writePop(Segment::POINTER, 0);
	}
	compileStatements(type);
	eat('}');
}

void CompilationEngine::compileVarDec()
{
	eat(Keyword::VAR);
	string type = tk.identifier();
	eatType();
	checkVarDec(false);
	subroutineTable.define(tk.identifier(), type, Kind::VAR);
	
	eat(Token::IDENTIFIER);
	while (eat(',', true) == Status::OK) {
		checkVarDec(false);
		subroutineTable.define(tk.identifier(), type, Kind::VAR);
		
		eat(Token::IDENTIFIER);
	}
	eat(';');
}

void CompilationEngine::compileStatements(const string& type)
{
	bool endFlag = false;
	while (!tk.aborted() && tk.tokenType() == Token::KEYWORD && !endFlag) {
		switch (tk.keyword()) {
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
			compileReturn(type);
			break;
		default:
			endFlag = true;
			break;
		}
	}
}

ItWithResult CompilationEngine::getVarIt(const string& name) {
	auto it = subroutineTable.getTable().find(name);
	if (it != subroutineTable.getTable().end()) {
		return std::make_tuple(it, true);
	}
	it = classTable.getTable().find(name);
	if (it != classTable.getTable().end()) {
		return std::make_tuple(it, true);
	}
	else return std::make_tuple(it, false);
}

bool CompilationEngine::prepareMethod(string& token, bool sureMethod)
{
	if (sureMethod) {
		vm.writePush(Segment::POINTER, 0);
		token = className + "." + token;
		return true;
	}
	std::map<string, const STEntry>::iterator it;
	bool itResult;
	std::tie(it, itResult) = getVarIt(token);
	if (!itResult) {
		token = token + "." + tk.identifier();
	}
	else {
		token = (it->second.type) + "." + tk.identifier();
		vm.writePush(kindToSegment(it->second.kind), it->second.idx);
	}
	return itResult;
}

bool CompilationEngine::isDefined(std::string var) {
	std::map<string, const STEntry>::iterator it;
	bool itResult;
	std::tie(it, itResult) = getVarIt(var);
	return itResult;
}

void CompilationEngine::compileLet()
{
	bool isArray = false;
	eat(Keyword::LET);
	string varName = tk.identifier();

	std::map<string, const STEntry>::iterator it;
	bool itResult;
	std::tie(it, itResult) = getVarIt(varName);
	
	if (!itResult) {
		cout << brightError << " at line " << tk.currLineNum() << ": Attempting to assign value to undeclared variable \"" << tk.identifier() << "\"" << endl;
		return;
	}
	
	eat(Token::IDENTIFIER);	

	if (eat('[', true) == Status::OK) {
		isArray = true;
		vm.writePush(kindToSegment(it->second.kind), it->second.idx);
		compileExpression();
		vm.writeArithmetic(Command::ADD);
		eat(']');
	}
	eat('=');
	compileExpression();
	// after returning from compileExpression, result of expression will be at the top of the stack
	if (isArray) {
		vm.writePop(Segment::TEMP, 0);
		vm.writePop(Segment::POINTER, 1);
		vm.writePush(Segment::TEMP, 0);
		vm.writePop(Segment::THAT, 0);
	}
	else {
		vm.writePop(kindToSegment(it->second.kind), it->second.idx);
	}
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
	string elseBlock = random_string(20);
	string afterIf = random_string(20);
	eat(Keyword::IF);
	eat('(');
	compileExpression();
	eat(')');
	// true or false value will be on the stack
	vm.writeArithmetic(Command::NOT);
	vm.writeIf(elseBlock);
	eat('{');
	compileStatements();
	eat('}');
	vm.writeGoto(afterIf);
	vm.writeLabel(elseBlock);
	if (eat(Keyword::ELSE, true) == Status::OK) {
		eat('{');
		compileStatements();
		eat('}');
	}
	vm.writeLabel(afterIf);
}

void CompilationEngine::compileWhile()
{
	string whileBlock = random_string(20);
	string afterWhile = random_string(20);
	eat(Keyword::WHILE);
	vm.writeLabel(whileBlock);
	eat('(');
	compileExpression();
	eat(')');
	vm.writeArithmetic(Command::NOT);
	vm.writeIf(afterWhile);
	eat('{');
	compileStatements();
	eat('}');
	vm.writeGoto(whileBlock);
	vm.writeLabel(afterWhile);
}

void CompilationEngine::compileDo()
{
	eat(Keyword::DO);
	string subOrObjName = tk.identifier();
	tk.advance();
	int numArgs = 0;
	bool hasClassAccess = tk.tokenType() == Token::SYMBOL && tk.symbol() == '.';
	if (eat('.', true) == Status::OK) {
		bool isMethod = prepareMethod(subOrObjName);
		if (isMethod) ++numArgs;
		tk.advance();
	}
	else {
		prepareMethod(subOrObjName, true);
		++numArgs;
	}
	eat('(');
	numArgs += compileExpressionList();
	eat(')');
	compileIdentifier(false, true, subOrObjName, numArgs);
	eat(';');
	vm.writePop(Segment::TEMP, 0); // A "do" call means the caller doesn't care about
									// the return value. Pop into temp to throw it away
}

void CompilationEngine::compileReturn(const string& type)
{
	eat(Keyword::RETURN);
	if (eat(';', true) == Status::OK) {
		if (type == "void") {
			vm.writePush(Segment::CONST, 0);
		}
		vm.writeReturn();
		return;
	}
	compileExpression();
	eat(';');
	vm.writeReturn();
}

void CompilationEngine::compileExpression()
{
	compileTerm();
	while (eatOp(true, true) == Status::OK) {
		Command op = opMap.find(tk.symbol())->second;
		tk.advance();
		compileTerm();
		vm.writeArithmetic(op);
	}
}

static bool isKeywordConst(Keyword key) {
	return (key == Keyword::TRUE || key == Keyword::FALSE || key == Keyword::K_NULL || key == Keyword::THIS);
}

void CompilationEngine::compileTerm()
{
	if (tk.tokenType() == Token::IDENTIFIER) {
		compileTermIdentifier();
	}
	else if (eat(Token::INT_CONST, true) == Status::OK) {}
	else if (eat(Token::STRING_CONST, true) == Status::OK) {}
	else if (tk.tokenType() == Token::KEYWORD && isKeywordConst(tk.keyword())) {
		eat(Token::KEYWORD);
	}
	else if (eat('(', true) == Status::OK) {
		compileExpression();
		eat(')');
	}
	else if (eat('-', true) == Status::OK) {
		compileTerm();
		vm.writeArithmetic(Command::NEG);
	}
	else if (eat('~', true) == Status::OK) {
		compileTerm();
		vm.writeArithmetic(Command::NOT);
	}
}

int CompilationEngine::compileExpressionList()
{
	int argCount = 0;
	if (tk.tokenType() == Token::SYMBOL && tk.symbol() == ')') {
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

void CompilationEngine::compileIdentifier(bool beingDefined, bool isSubroutine, const string& savedToken, int argCount)
{
	string token = (savedToken.length() > 0) ? savedToken : tk.identifier();

	if (isSubroutine && !beingDefined) {
		vm.writeCall(token, argCount);
		if (savedToken.length() == 0) tk.advance();
		return;
	}

	std::map<string, const STEntry>::iterator it;
	bool itResult;
	std::tie(it, itResult) = getVarIt(token);

	if (itResult && !beingDefined) {
		vm.writePush(kindToSegment(it->second.kind), it->second.idx);
	}	
	else { 
		// is class
	}
	if (savedToken.length() == 0) tk.advance();
}

void CompilationEngine::compileTermIdentifier() {
	string token = tk.identifier();
	tk.advance();
	int args = 0;
	bool usedSymbolFlag = false;
	if (tk.tokenType() == Token::SYMBOL) {
		switch (tk.symbol()) {
		case '[':
			compileIdentifier(false, false, token); // pushes base address onto stack
			eat('[');
			compileExpression();
			eat(']');
			vm.writeArithmetic(Command::ADD);
			vm.writePop(Segment::POINTER, 1);
			vm.writePush(Segment::THAT, 0);
			usedSymbolFlag = true;
			break;

		case '(':
			eat('(');
			prepareMethod(token, true);
			args = compileExpressionList() + 1;
			eat(')');
			compileIdentifier(false, true, token, args);
			usedSymbolFlag = true;
			break;

		case '.':
			eat('.');
			bool isMethod = prepareMethod(token);
			eat(Token::IDENTIFIER);
			eat('(');
			args = compileExpressionList() + (isMethod ? 1 : 0); 
			eat(')');
			compileIdentifier(false, true, token, args);	
			usedSymbolFlag = true;
			break;
		}
	}
	if (!usedSymbolFlag) {
		compileIdentifier(false, false, token);
	}
}

CompilationEngine::~CompilationEngine()
{
}
