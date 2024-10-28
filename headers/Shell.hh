#ifndef shell_hh
#define shell_hh

#include "ListCommands.hh"
#include "PipeCommand.hh"
#include "IfCommand.hh"
#include "WhileCommand.hh"
#include "ForCommand.hh"
#include <stack>

class Shell {

public:
  int _level; // Only outer level executes.
  bool _enablePrompt;

	std::stack<ListCommands*> listStack;
	std::stack<WhileCommand*> whileStack;
	std::stack<ForCommand*> forStack;

  ListCommands * _listCommands; 
  SimpleCommand *_simpleCommand;
  PipeCommand * _pipeCommand;
  IfCommand * _ifCommand;
	WhileCommand * _whileCommand;
	ForCommand * _forCommand;
	
  Command * _currentCommand;
  static Shell * TheShell;

	std::string* _path;
	std::string _prevCommandLastArg;

	int _lastZombie;
	
	int _lastSimple;

  Shell();
	~Shell();
  void execute();
  void print();
  void clear();
  void prompt();

};

#endif
