
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vector>
#include <string>
#include <iostream>

#include "Command.hh"
#include "SimpleCommand.hh"
#include "IfCommand.hh"

IfCommand::IfCommand() {
    _condition = NULL;
    _listCommands =  NULL;
			std::string test = "test";
}


// Run condition with command "test" and return the exit value.
int
IfCommand::runTest(SimpleCommand * condition) {
			

		
		std::string test = "test";
		
		std::vector<char *> argv;

		argv.push_back( (char *) test.c_str());

		for (std::string* arg : condition->_arguments) {
			argv.push_back( (char*) arg->c_str());	
		}

		argv.push_back(nullptr);	

		int ret = fork();

		if (!ret) {
			execvp(argv[0], argv.data());
			perror("execvp failed");
			exit(1);
		} else {
			int status;
			waitpid(ret, &status, 0);
			if (WIFEXITED(status)) {
				//condition->print();
				return WEXITSTATUS(status);
			} else {
				perror("WIFEXITED");
				exit(1);
			}
		}

}

void 
IfCommand::insertCondition( SimpleCommand * condition ) {
    _condition = condition;
}

void 
IfCommand::insertListCommands( ListCommands * listCommands) {
    _listCommands = listCommands;
}

void 
IfCommand::clear() {
}

void 
IfCommand::print() {
    printf("IF [ \n"); 
    this->_condition->print();
    printf("   ]; then\n");
    this->_listCommands->print();
}
  
void 
IfCommand::execute() {
    // Run command if test is 0
    if (runTest(this->_condition) == 0) {
	_listCommands->execute();
    }
}

