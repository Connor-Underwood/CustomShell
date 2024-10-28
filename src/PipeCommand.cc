#include <stdio.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <regex>
#include <algorithm>
#include <dirent.h>
#include <cctype>
#include <regex.h>

#define A_LOT 10000
#define MAXFILENAME 1024

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "PipeCommand.hh"
#include "Shell.hh"



struct sigaction sa;

void sig_child_handler(int sig) {
	if (sig == SIGCHLD) {
		while(waitpid(-1, NULL, WNOHANG) > 0);
	}
}

void setup_sigchld_handler() {
	sa.sa_handler = sig_child_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}
}

PipeCommand::PipeCommand() {
	// Initialize a new vector of Simple PipeCommands
	_simpleCommands = std::vector<SimpleCommand *>();

	_outFile = NULL;
	_inFile = NULL;
	_errFile = NULL;
	_background = false;
	_appendOut=false;
	_appendErr=false;

	_multipleIns=false;
	_multipleOuts=false;
	_multipleErrors=false;
}

void PipeCommand::insertSimpleCommand( SimpleCommand * simplePipeCommand ) {
	// add the simple command to the vector
	_simpleCommands.push_back(simplePipeCommand);
}

void PipeCommand::clear() {
	// deallocate all the simple commands in the command vector
	for (auto simplePipeCommand : _simpleCommands) {
		delete simplePipeCommand;
	}

	// remove all references to the simple commands we've deallocated
	// (basically just sets the size to 0)
	_simpleCommands.clear();



	if ( _outFile && _outFile != _errFile) {
		delete _outFile;

	}

	_outFile = NULL;

	if ( _inFile )  {
		delete _inFile;
	}

	_inFile = NULL;


	if (_errFile) {
		delete _errFile;
	}

	_errFile = NULL;

	_background = false;
	_appendErr = false;
	_appendOut = false;
	_multipleIns = false;
	_multipleOuts = false;
	_multipleErrors = false;

}

void PipeCommand::print() {
	/*
	printf("\n\n");
	//printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple PipeCommands\n");
	printf("  --- ----------------------------------------------------------\n");
	*/
	int i = 0;
	// iterate over the simple commands and print them nicely
	for ( auto & simplePipeCommand : _simpleCommands ) {
		printf("  %-3d ", i++ );
		simplePipeCommand->print();
	}
	/*

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n",
			_outFile?_outFile->c_str():"default",
			_inFile?_inFile->c_str():"default",
			_errFile?_errFile->c_str():"default",
			_background?"YES":"NO");
	printf( "\n\n" );
	*/
}

void PipeCommand::execute() {
	// Don't do anything if there are no simple commands
	
	
	if ( _simpleCommands.size() == 0 ) {
		Shell::TheShell->prompt();
		return;
	}


	// Print contents of PipeCommand data structure
	//print();

	// Add execution here
	// For every simple command fork a new process
	// Setup i/o redirection
	// and call exec
	int tmpin = dup(0);
	int tmpout = dup(1);
	int errout = dup(2);

	int fdin;

	if (_inFile) {


		if (_multipleIns) {
			fprintf(stderr, "Ambiguous input redirect.\n");
			close(tmpin);
			close(tmpout);
			close(errout);
			return;
		}
		fdin = open(_inFile->c_str(), O_RDONLY);
	} else {
		fdin = dup(tmpin);
	}

	int ret;
	int fdout;
	int fderr;

	long unsigned int i;


	for (i = 0; i < _simpleCommands.size(); i++) {


		dup2(fdin,0);
		close(fdin);

		if (i == _simpleCommands.size() - 1) {

			if (_outFile) {

				if (_multipleOuts) {
					fprintf(stderr, "Ambiguous output redirect.\n");
					close(tmpin);
					close(tmpout);
					close(errout);
					return;
				}

				if (_appendOut) {
					fdout = open(_outFile->c_str(), O_RDWR | O_APPEND, S_IRWXU);
				} else {
					fdout = open(_outFile->c_str(), O_RDWR | O_TRUNC | O_CREAT, S_IRWXU);
				}
			} else {

				fdout = dup(tmpout);

			}

			if (_errFile) {

				if (_appendErr) {

					fderr = open(_errFile->c_str(), O_RDWR | O_APPEND, S_IRWXU);

				} else {
					fderr = open(_errFile->c_str(), O_RDWR | O_TRUNC | O_CREAT, S_IRWXU);

				}
			} else {

				fderr = dup(errout);

			}

		} else {

			int fdpipe[2];
			pipe(fdpipe);
			fdout=fdpipe[1];
			fdin=fdpipe[0];

		}

		dup2(fdout, 1);
		close(fdout);
		dup2(fderr, 2);
		close(fderr);


		SimpleCommand * s = _simpleCommands[i];

		const char ** argv = expandEnvVarsAndWildcards(s);


		Shell::TheShell->_prevCommandLastArg = std::string(argv[s->_arguments.size() - 1]);





		if (*s->_arguments[0] == "cd") {
			if (s->_arguments.size() == 1) {
				// CD HOME
				char * home = getenv("HOME");
				chdir(home);
			} else {
				int flag = chdir(argv[1]);

				if (flag != 0) {
					// CD FAILS
					fprintf(stderr, "cd: can't cd to %s", s->_arguments[1]->c_str());
					return;
				}
			}
		} else if (*s->_arguments[0] == "setenv") {

			setenv(argv[1], argv[2], 1);

		} else if (*s->_arguments[0] == "unsetenv"){

			unsetenv(argv[1]);

		} else {
			// CALL COMMAND
			setup_sigchld_handler();
			ret = fork();
			if (ret == 0) {

				if (strcmp(s->_arguments[0]->c_str(), "printenv") == 0) {
					char **vars = environ;

					while (*vars) {
						printf("%s\n", *vars);
						vars++;
					}
					exit(0);
				}

				execvp(argv[0], (char * const*) argv);
				perror("execvp");
				exit(1);

			} else if (ret > 0 && _background == true) {
				Shell::TheShell->_lastZombie = ret;
			} else if (ret > 0) {

				int status; 

				waitpid(ret, &status, 0);
				if (WIFEXITED(status)) {
					int exit_status = WEXITSTATUS(status);
					Shell::TheShell->_lastSimple = exit_status;
				}

			}
		}
	} // for loop

	// RETURN I/O VALS
	dup2(tmpin,0);
	dup2(tmpout,1);
	dup2(errout,2);
	close(tmpin);
	close(tmpout);
	close(errout);

	if (!_background) {
		waitpid(ret, NULL, 0);
	} 

	// Clear to prepare for next command
	//clear();
}


std::vector<std::string*> subshell(std::vector<std::string*> arguments) {
	std::vector<std::string*> res;

	for (size_t i = 0; i < arguments.size(); i++) {

		const std::string& input = *arguments[i]; // Dereference pointer to work with the string directly

		// Define the regex pattern with non-greedy matches and check the entire input
		std::regex subShellPattern(R"(\$\((.*?)\)|\`(.*?)\`)");
		std::smatch subShellMatches;


		// Check if the entire input matches the pattern
		if (std::regex_match(input, subShellMatches, subShellPattern)) {

			std::string match = subShellMatches[1].matched ? subShellMatches[1].str() : subShellMatches[2].str();


			int pin[2];
			int pout[2];

			pipe(pin);
			pipe(pout);

			write(pin[1], match.c_str(), match.size());

			write(pin[1], "\n", 1);

			write(pin[1], "exit", 4);

			write(pin[1], "\n", 1);

			int ret = fork();
			// im cooked after this
			if (ret == 0) {	

				dup2(pin[0], 0);	//stdin is pin[0]
				close(pin[0]);

				dup2(pout[1], 1); // stdout is pout[1]
				close(pout[1]);

				close(pin[1]); // close pin[1]
				close(pout[0]); // close pout[0]


				const char *arr[] = {"/proc/self/exe", NULL};
				const char **args = arr; 
				execvp(arr[0], (char * const *) args);


				exit(1);
			}


			close(pin[1]);

			close(pout[1]);

			// read in from pout[0] 
			char *buf = new char[A_LOT];

			char c; 

			int x = 0;
			while (read(pout[0], &c, 1)) {
				if (c == '\n') {
					buf[x++] = ' ';
				} else {
					buf[x++] = c;
				}
			}
			buf[x] = '\0'; // da null byte



			// split buf spaces

			std::string sub_output(buf);

			std::string s; 


			for (size_t j = 0; j < sub_output.size(); j++) {
				if (sub_output[j] != ' ' && sub_output[j] != '\n') {
					s.push_back(sub_output[j]);
				} else if (j == sub_output.size() -1 ) {
					continue;
				} else if (!s.empty()) {
					res.push_back(new std::string(s));
					s.clear();
				}
			}

			if (!s.empty()) {
				res.push_back(new std::string(s));
			}




		} else {
			res.push_back(arguments[i]);
		}
	}

	return res;
}

std::string expandTilde(const std::string& input) {
	// regex by itself
	std::regex selfRegex("^~(/|$)");

	// with a user
	std::regex userRegex("^~([a-zA-Z0-9_]+)(/.*)?$");

	if (std::regex_search(input, selfRegex)) {
		// Ensure HOME environment variable is valid
		const char* homePath = getenv("HOME");
		if (homePath != nullptr) {

			return std::regex_replace(input, selfRegex, std::string(homePath) + "/");
		} else {
			// Fallback if HOME is not set
			return input;
		}
	}

	// Attempt to match and replace the pattern for other user's home directory
	std::smatch matches;
	if (std::regex_search(input, matches, userRegex)) {
		std::string username = matches[1].str();
		std::string rest = matches[2].str();


		// You might need a more robust way to construct other users' home directory paths
		return "/homes/" + username + rest;
	}
	// If no patterns matched, return the input unmodified
	return input;
}

std::string expandEnv(const std::string* argument) {
	// Fuck this function

	const std::string& input = *argument; // Dereference pointer to work with the string directly

	std::string result;
	std::regex envPattern(R"(\$\{([^\}][^\}]*)\})");

	std::smatch matches;
	auto searchStart(input.cbegin());

	while (std::regex_search(searchStart, input.cend(), matches, envPattern)) {
		result += matches.prefix().str(); // Add text before the match

		std::string variable = matches[1].str(); // Extract variable name

		const char* value;

		if (variable == "SHELL") {
			value = Shell::TheShell->_path->c_str();
		} else if (variable == "$") {
			pid_t pid = getpid();
			std::string pidStr = std::to_string(pid);
			value = pidStr.c_str();
		} else if (variable == "!") {
			value = std::to_string(Shell::TheShell->_lastZombie).c_str();
		} else if (variable == "?") {
			value = std::to_string(Shell::TheShell->_lastSimple).c_str();
		} else if (variable == "_") {
			value = Shell::TheShell->_prevCommandLastArg.c_str();
		} else {
			value = getenv(variable.c_str());
		}
		result += value;

		searchStart = matches.suffix().first; // Move our cursor to end of env_var
	}
	result += std::string(searchStart, input.cend()); // Append the rest of the string

	return std::string(result); // Return new string *
}



char * expandRegex(char * component) {
	char *reg = (char *) malloc((2 * sizeof(component)) + 10);
	char *r = reg;

	char *start = component;


	*r = '^'; // add carrot to signify start of string
	r++;

	
	while (*start) {
		if (*start == '*') {
			*r = '.';
			r++;
			*r = '*';
			r++;
		} else if (*start == '?') {
			*r = '.';
			r++;
		} else if (*start == '.') {
			*r = '\\';
			r++;
			*r = '.';
			r++;
		} else {
			*r = *start;
			r++;
		}
		start++;
	}

	*r = '$';
	r++;
	*r = 0;

	return reg;
}

void expandWildcardArg(char * prefix, char * suffix, std::vector<std::string*>& args) {
	
	if (suffix[0] == '\0') {
		args.push_back(new std::string(strdup(prefix)));
		return;
	}

	

	
	bool startSlash = false;
	if (suffix[0] == '/') {
		suffix++;
		prefix[0] = '/';
		startSlash = true;
	}


	char *s = strchr(suffix, '/');
	
	char component[MAXFILENAME] = { 0 };

	if (s!= NULL) {
		strncpy(component, suffix, s-suffix);
		// append null byte to component
		component[s-suffix] = '\0';
		suffix = s+1;
	} else {
		strcpy(component, suffix);
		component[strlen(suffix)] = '\0';	
		suffix += strlen(suffix);
	}

	char newPrefix[MAXFILENAME];

	if (std::string(component).find('*') == std::string::npos && std::string(component).find('?') == std::string::npos) {
		// component does not have wildcards
		if (strcmp(prefix, "") == 0) {
			sprintf(newPrefix, "%s", component);
		} else {
			if (startSlash) {
				sprintf(newPrefix, "%s%s", prefix, component);
			} else {
				sprintf(newPrefix, "%s/%s", prefix, component);
			}
		}
		expandWildcardArg(newPrefix, suffix, args);
		return;
	} 
	

	char * reg = expandRegex(component);

	
	regex_t regex;	


	int ret = regcomp(&regex, reg, REG_EXTENDED|REG_NOSUB);
	
	if (ret) {
		perror("regcomp");
		exit(1);
	}
	
	const char *dir;
	
	if (strcmp(prefix, "") == 0) {
		dir = ".";	
	} else {
		dir = prefix;	
	}
	
	
	bool includeDot = component[0] == '.';

	DIR* d;
	d = opendir(dir);



	if (d == NULL) {
		return;
	}

	struct dirent* entry;

	while ((entry = readdir(d)) != NULL) {
		if (regexec(&regex, entry->d_name, 0, NULL, 0) == 0) {
			if (entry->d_name[0] == '.') {
				if (includeDot) {
					if (strcmp(prefix, "") == 0) {
						sprintf(newPrefix, "%s", entry->d_name);
					} else {
						sprintf(newPrefix, "%s/%s", prefix, entry->d_name);
					}
					expandWildcardArg(newPrefix, suffix, args);
				} 
			} else {
				if (strcmp(prefix, "") == 0) {
					sprintf(newPrefix, "%s", entry->d_name);
				} else {
					if (strcmp(prefix, "/") == 0) {
						sprintf(newPrefix, "%s%s", prefix, entry->d_name);
					} else {
						sprintf(newPrefix, "%s/%s", prefix, entry->d_name);
					}
				}
				expandWildcardArg(newPrefix, suffix, args);

				
			}
		}
	}
	
	closedir(d);
}

bool compareStringPointers(const std::string *a, const std::string *b) {
	return *a < *b;
}


// Expands environment vars and wildcards of a SimpleCommand and
// returns the arguments to pass to execvp.
const char ** PipeCommand::expandEnvVarsAndWildcards(SimpleCommand * s)
{

	std::vector<std::string*> args = subshell(s->_arguments);

	for (size_t i = 0; i < args.size(); i++) {				
		std::string env = expandEnv(args[i]);	
		std::string tilde = expandTilde(env);
		args[i] = new std::string(tilde);
	} // for

	std::vector<std::string*> actualArgs; 
	for (size_t i = 0; i < args.size(); i++) {	
		std::vector<std::string*> temp; 	
		char prefix[] = "";
		char *suffix = (char *) args[i]->c_str();
		// regex
		expandWildcardArg(prefix, suffix, temp);


		if (temp.size() > 1) {
			std::sort(temp.begin(), temp.end(), compareStringPointers);
		} else if (temp.size() == 0)  {// had a wildcard but no matches
			actualArgs.push_back(new std::string(suffix));
		}
		
		for (size_t j = 0; j < temp.size(); j++) {
			actualArgs.push_back(temp[j]);
		}		
	}	
	

	// Clear simple command args and replace them with the expanded
	/*
	s->_arguments.clear();

	for (std::string * arg : actualArgs) {
		s->_arguments.push_back(arg);
	}
	*/

	const char ** argv = (const char **) malloc((actualArgs.size() + 1) * sizeof(char *));
	for (size_t i =0; i < actualArgs.size(); i++) {
		argv[i] = actualArgs[i]->c_str();
	}


	//argv[args.size()] = NULL;
	argv[actualArgs.size()] = NULL;
	return argv;
}


