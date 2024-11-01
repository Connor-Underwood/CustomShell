#include <cstdio>
#include <cstdlib>
#include <regex>
#include <iostream>
#include <regex.h>

#include "SimpleCommand.hh"
#include "Shell.hh"
#include <unistd.h>

SimpleCommand::SimpleCommand() {
	_arguments = std::vector<std::string *>();
}

SimpleCommand::~SimpleCommand() {
	// iterate over all the arguments and delete them
	for (auto & arg : _arguments) {
		delete arg;
	}
}



void SimpleCommand::insertArgument( std::string * argument ) {
	_arguments.push_back(argument);


}

// Print out the simple command
void SimpleCommand::print() {
	for (auto arg : _arguments) {
		std::cout << "!"<< *arg << "!\t";
	}
	// effectively the same as printf("\n\n");
	std::cout << std::endl;
}

void SimpleCommand::clear() {
	for (auto & arg : _arguments) {
		delete arg;
	}
	_arguments.clear();

}
