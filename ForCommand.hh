#ifndef forcommand_hh
#define forcommand_hh

#include "Command.hh"
#include "SimpleCommand.hh"
#include "ListCommands.hh"
#include <iostream>
#include <string>

// Command Data Structure

class ForCommand : public Command {
public:
  SimpleCommand * _iterables;
  ListCommands * _listCommands; 
	const char * _variable;

  ForCommand();
	void insertVariable( std::string * variable);
  void insertListCommands( ListCommands * listCommands);
	void insertIterable( SimpleCommand * iterable );

  void clear();
  void print();
  void execute();

};

#endif
