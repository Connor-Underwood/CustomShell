#ifndef whilecommand_hh
#define whilecommand_hh

#include "Command.hh"
#include "SimpleCommand.hh"
#include "ListCommands.hh"

// Command Data Structure

class WhileCommand : public Command {
public:
  SimpleCommand * _condition;
  ListCommands * _listCommands; 

  WhileCommand();
  void insertCondition( SimpleCommand * condition );
  void insertListCommands( ListCommands * listCommands);
  static int runTest(SimpleCommand * condition);

  void clear();
  void print();
  void execute();

};

#endif
