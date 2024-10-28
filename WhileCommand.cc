
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

#include "Command.hh"
#include "SimpleCommand.hh"
#include "WhileCommand.hh"
#include "Shell.hh"

WhileCommand::WhileCommand() {
    _condition = NULL;
    _listCommands =  NULL;
}

int getArgsSize(const char ** args) {
	int count = 0;
	while (args[count] != NULL) {
		++count;
	}
	return count;
}

// Run condition with command "test" and return the exit value.
int WhileCommand::runTest(SimpleCommand * condition) {
		
		const char ** args = Shell::TheShell->_pipeCommand->expandEnvVarsAndWildcards(condition);
				
		
		// THIS SIZE DOES NOT INCLUDE THE NULL BYTE 
		int argSize = getArgsSize(args);
		
		std::vector<const char *> argv(argSize+2); // ADD 2 FOR TEST AT THE FRONT AND NULL BYTE AT THE END
		
		std::string test = "test";
		argv[0] = test.c_str();
		
		for (int i = 0; i < argSize; i++) {	
			argv[i+1] = args[i];
		}

		int ret = fork();

		if (ret == 0) {
			execvp(argv[0],(char * const *) argv.data());
			perror("execvp failed");
			exit(1);
		} else {
			int status;
			waitpid(ret, &status, 0);
			if (WIFEXITED(status)) {
				
				int res = WEXITSTATUS(status);
				return res;
			} else {
				perror("WIFEXITED");
				exit(1);
			}
		}

}

void 
WhileCommand::insertCondition( SimpleCommand * condition ) {
    _condition = condition;
}

void WhileCommand::insertListCommands( ListCommands * listCommands) {
    _listCommands = listCommands;
}

void WhileCommand::clear() {
}

void 
WhileCommand::print() {
    printf("WHILE [ \n"); 
    this->_condition->print();
    printf("   ]; do\n");
    this->_listCommands->print();
}
  
void WhileCommand::execute() {
    // Run command if test is 0		
    while (runTest(this->_condition) == 0) {
			_listCommands->execute();
    }
}

