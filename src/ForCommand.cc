#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <cctype>
#include <regex>
#include <dirent.h>
#include <regex.h>
#include <cstdio>
#include <cstdlib>


#include "Command.hh"
#include "SimpleCommand.hh"
#include "ForCommand.hh"
#include "Shell.hh"

#define MAXFILENAME 1024


ForCommand::ForCommand() {
	_variable = "";
	_iterables = NULL;
	_listCommands =  new ListCommands();
}

void printArgs(const char ** args) {
	int count = 0;
	while (args[count] != NULL) {
		printf("args[%d] = %s\n", count, args[count]);
		count++;
	}
}


void ForCommand::insertVariable(std::string * variable) {
	_variable = variable->c_str();
}

void ForCommand::insertIterable( SimpleCommand * iterable ) {
	_iterables = iterable;
}


void ForCommand::insertListCommands( ListCommands * listCommands) {
	_listCommands = listCommands;
}

void ForCommand::clear() {
}

void 
ForCommand::print() {
	printf("\n\n\n");
	printf("FOR %s IN\n", _variable); 
	this->_iterables->print();
	printf("   ; do\n");
	printf("start of list\n");
	this->_listCommands->print();
	printf("end of list\n");
	printf("\n\n\n");
}




void ForCommand::execute() {
	
	const char ** argv = Shell::TheShell->_pipeCommand->expandEnvVarsAndWildcards(_iterables);
	
		
	int count = 0;
	while (argv[count] != NULL) {

		setenv(_variable, argv[count++], 1);
		_listCommands->execute();

	}
	
}

