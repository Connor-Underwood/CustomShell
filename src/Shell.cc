
#include <unistd.h>
#include <cstdio>
#include <limits.h>
#include "Command.hh"
#include "Shell.hh"
#include <signal.h>
#include <iostream>
#include <cstring>
#include <string>
#include <algorithm>

int yyparse(void);

Shell * Shell::TheShell;

extern "C" void handle( int sig ) { 
	if (sig == SIGINT) {
		fprintf(stderr, "\n");
		Shell::TheShell->prompt();
		return;
	}
}

Shell::Shell() {
	this->_level = 0;
	this->_enablePrompt = true;
	this->_listCommands = new ListCommands(); 
	this->_simpleCommand = new SimpleCommand();
	this->_pipeCommand = new PipeCommand();
	this->_currentCommand = this->_pipeCommand;
	
	

	if ( !isatty(0)) {
		this->_enablePrompt = false;
	}
	this->_path = NULL;
	this->_lastZombie = -1;
	this->_lastSimple = -1;
	this->_prevCommandLastArg = "";
}

Shell::~Shell() = default;

void Shell::prompt() {
	if (_enablePrompt) {
		printf("myshell>");
		fflush(stdout);
	}
}

void Shell::print() {
	printf("\n--------------- Command Table ---------------\n");
	this->_listCommands->print();
}

void Shell::clear() {
	this->_listCommands->clear();
	this->_simpleCommand->clear();
	this->_pipeCommand->clear();
	this->_currentCommand->clear();
	this->_level = 0;
}

void Shell::execute() {
	if (this->_level == 0 ) {
		//this->print();
		this->_listCommands->execute();
		this->_listCommands->clear();
		this->prompt();
	}
}

void yyset_in (FILE *  in_str );

int 
main(int argc, char **argv) {




	char * input_file = NULL;
	if ( argc > 1 ) {
		for (int i = 1; i < argc; i++) {
			setenv(std::to_string(i-1).c_str(), argv[i], 1);
		}
		setenv("#", std::to_string(argc-2).c_str(), 1);
		input_file = argv[1];
		FILE * f = fopen(input_file, "r");
		if (f==NULL) {
			fprintf(stderr, "Cannot open file %s\n", input_file);
			perror("fopen");
			exit(1);
		}
		yyset_in(f);
	}  


	Shell::TheShell = new Shell();
	
	char buff[10000];
	Shell::TheShell->_path = new std::string(realpath(argv[0], buff));


	struct sigaction sa;
	sa.sa_handler = handle;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if(sigaction(SIGINT, &sa, NULL)){
		perror("sigaction");
		exit(2);
	}

	if (input_file != NULL) {
		// No prompt if running a script
		Shell::TheShell->_enablePrompt = false;
	}

	else {
		Shell::TheShell->prompt();
	}
	yyparse();
	Shell::TheShell->~Shell();



}


