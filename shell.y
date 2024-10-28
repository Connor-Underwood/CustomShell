

%code requires {
#include <string>

#if __cplusplus > 199711L
#define register      // Deprecated in C++11 so remove the keyword
#endif
}

%union
{
	char        *string_val;
	// Example of using a c++ type in yacc
	std::string *cpp_string;
}

%token <cpp_string> WORD 
%token NOTOKEN GREAT GREATGREAT GREATAMPERSAND GREATGREATAMPERSAND TWOGREAT 
%token AMPERSAND PIPE LESS NEWLINE IF FI THEN LBRACKET RBRACKET SEMI
%token DO DONE WHILE FOR IN EXIT QUOTEDWORD PRINTENV TWOGREATAMPERSANDONE WILDCARD

%{
	//#define yylex yylex
#include <cstdio>
#include "Shell.hh"
#include <iostream>
#include <unistd.h>
extern char **env = environ;
extern int yylex_destroy();

	void yyerror(const char * s);
	int yylex();
	int whileCount = 0;

	%}

	%%


	goal: command_list;

arg_list:
arg_list
WORD {	

		Shell::TheShell->_simpleCommand->insertArgument( $2 );
}
|
	/*empty string*/
	;

cmd_and_args:
EXIT {
	yylex_destroy();
	exit(1);
} 
| 
WORD { 
	Shell::TheShell->_simpleCommand = new SimpleCommand(); 
	Shell::TheShell->_simpleCommand->insertArgument( $1 );
} 
arg_list
;

pipe_list: 
cmd_and_args 
{ 
	Shell::TheShell->_pipeCommand->insertSimpleCommand(Shell::TheShell->_simpleCommand ); 
	Shell::TheShell->_simpleCommand = new SimpleCommand();
}
| pipe_list PIPE cmd_and_args 
{ 
	Shell::TheShell->_pipeCommand->insertSimpleCommand(Shell::TheShell->_simpleCommand);
	Shell::TheShell->_simpleCommand = new SimpleCommand();
}
;

io_modifier:
GREATGREAT WORD {
	Shell::TheShell->_pipeCommand->_outFile = $2;
	Shell::TheShell->_pipeCommand->_appendOut = true;
}
| GREAT WORD 
{
	if (!Shell::TheShell->_pipeCommand->_outFile) {
		Shell::TheShell->_pipeCommand->_outFile = $2;
	} else {
		Shell::TheShell->_pipeCommand->_multipleOuts = true;
	}
}
| GREATGREATAMPERSAND WORD {
	Shell::TheShell->_pipeCommand->_outFile = $2;
	std::string *copy = $2;
	Shell::TheShell->_pipeCommand->_errFile = copy;
	Shell::TheShell->_pipeCommand->_appendOut = true;
	Shell::TheShell->_pipeCommand->_appendErr = true;
}
| GREATAMPERSAND WORD {

	Shell::TheShell->_pipeCommand->_outFile = $2;
	std::string *copy = $2;
	Shell::TheShell->_pipeCommand->_errFile = copy;
}
| LESS WORD {
	if (!Shell::TheShell->_pipeCommand->_inFile) {
		Shell::TheShell->_pipeCommand->_inFile = $2;
	} else {
		Shell::TheShell->_pipeCommand->_multipleIns = true;
	}
}
| TWOGREAT WORD {
	Shell::TheShell->_pipeCommand->_errFile = $2;
}
| TWOGREATAMPERSANDONE {
	std::string *copyOut = Shell::TheShell->_pipeCommand->_outFile;
	Shell::TheShell->_pipeCommand->_errFile = copyOut;
}
;

io_modifier_list:
io_modifier_list io_modifier
| /*empty*/
;

background_optional: 
AMPERSAND {
	Shell::TheShell->_pipeCommand->_background = true;
}
| /*empty*/
;

SEPARATOR:
NEWLINE
| SEMI
;

command_line:

pipe_list io_modifier_list background_optional SEPARATOR 
{ 
	

	// populate _listCommands here

	Shell::TheShell->_listCommands->insertCommand(Shell::TheShell->_pipeCommand);
	Shell::TheShell->_pipeCommand = new PipeCommand(); 
}

| if_command SEPARATOR 
{
	Shell::TheShell->_listCommands->insertCommand(Shell::TheShell->_ifCommand);
}
| while_command SEPARATOR {
	
	Shell::TheShell->_listCommands->insertCommand(Shell::TheShell->_whileCommand);
}
| for_command SEPARATOR {

	Shell::TheShell->_listCommands->insertCommand(Shell::TheShell->_forCommand);	

	if (Shell::TheShell->forStack.size() > 0) {
		Shell::TheShell->_forCommand = Shell::TheShell->forStack.top();
		Shell::TheShell->forStack.pop();
	} else {
		Shell::TheShell->_forCommand = new ForCommand();
	}
}
| SEPARATOR /*accept empty cmd line*/
| error SEPARATOR {yyerrok; Shell::TheShell->clear(); }
;          /*error recovery*/

command_list :
command_line 
{ 
	Shell::TheShell->execute();
}
| 
command_list command_line 
{
	Shell::TheShell->execute();
}
;  /* command loop*/

if_command:
IF LBRACKET 
{ 
	Shell::TheShell->_level++; 
	Shell::TheShell->_ifCommand = new IfCommand();
} 
arg_list RBRACKET SEMI THEN 
{
	Shell::TheShell->_ifCommand->insertCondition(Shell::TheShell->_simpleCommand);
	Shell::TheShell->_simpleCommand = new SimpleCommand();
}
command_list FI 
{ 
	Shell::TheShell->_level--; 
	Shell::TheShell->_ifCommand->insertListCommands( 
			Shell::TheShell->_listCommands);
	Shell::TheShell->_listCommands = new ListCommands();
}
;


while_command:
WHILE LBRACKET {	
	if (Shell::TheShell->_level > 0) { 
		// If nested, push the previous while onto the stack
		Shell::TheShell->whileStack.push(Shell::TheShell->_whileCommand);	
		Shell::TheShell->listStack.push(Shell::TheShell->_listCommands);
		Shell::TheShell->_listCommands = new ListCommands();
	}


	Shell::TheShell->_level++;
	Shell::TheShell->_whileCommand = new WhileCommand();

}
arg_list RBRACKET SEMI DO
{	
	Shell::TheShell->_whileCommand->insertCondition(Shell::TheShell->_simpleCommand);
	Shell::TheShell->_simpleCommand = new SimpleCommand();
}
command_list DONE {		
	
	if (Shell::TheShell->_level > 1) {
		// Grab the outer while, since it is the last thing pushed
		WhileCommand * prevWhile = Shell::TheShell->whileStack.top();
		ListCommands * prevList = Shell::TheShell->listStack.top();
		
		
		// Append the parsed whileCommand and listCommand to the previous while and list commands	
		Shell::TheShell->_whileCommand->insertListCommands(Shell::TheShell->_listCommands);

		prevList->insertCommand(Shell::TheShell->_whileCommand);

		prevWhile->insertListCommands(prevList);

		
		// Pop them off of their stacks
		Shell::TheShell->whileStack.pop();
		Shell::TheShell->listStack.pop();
		

		// Replace the shells while command with the appended version
		Shell::TheShell->_whileCommand = prevWhile;
		Shell::TheShell->_listCommands = prevList;
	} else { // not nested	
		Shell::TheShell->_whileCommand->insertListCommands(Shell::TheShell->_listCommands);
		Shell::TheShell->_listCommands = new ListCommands();
	}
	Shell::TheShell->_level--;
}
;

for_command:
FOR WORD IN {
	Shell::TheShell->_level++;
	if (Shell::TheShell->_level > 1) { 
		// If nested, push the previous while onto the stack
		Shell::TheShell->forStack.push(Shell::TheShell->_forCommand);	
		Shell::TheShell->listStack.push(Shell::TheShell->_listCommands);
	}

	Shell::TheShell->_listCommands = new ListCommands();
	Shell::TheShell->_forCommand = new ForCommand();
	std::string *variable = $2;
	Shell::TheShell->_forCommand->insertVariable(variable);	

} arg_list SEMI DO  {

	Shell::TheShell->_forCommand->insertIterable(Shell::TheShell->_simpleCommand);
	Shell::TheShell->_simpleCommand = new SimpleCommand();
} command_list DONE {
	
	Shell::TheShell->_level--;
	Shell::TheShell->_forCommand->insertListCommands(Shell::TheShell->_listCommands);

	if (Shell::TheShell->listStack.size() > 0) {
		// Grab the outer while, since it is the last thing pushed

		Shell::TheShell->_listCommands = Shell::TheShell->listStack.top();

		Shell::TheShell->listStack.pop();

	} else { // not nested	
		Shell::TheShell->_listCommands = new ListCommands();
	}

}
;

%%

	void
yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}

#if 0
main()
{
	yyparse();
}
#endif
