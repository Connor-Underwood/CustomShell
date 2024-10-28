

%{

#include <cstring>
#include "y.tab.hh"
#include <iostream>

static  void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

%}

%option noyywrap

%%

"#".*\n {
  /* Discard comments starting with # */
}

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"<" {
  return LESS;
}


"|" {
  return PIPE;
}

">>" {
  return GREATGREAT;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"&" {
  return AMPERSAND;
}

"2>" {
	return TWOGREAT;
}

"2>&1" {
	return TWOGREATAMPERSANDONE;
}

"[" {
  return LBRACKET;
}

"]" {
  return RBRACKET;
}

"if" {
  return IF;
}

"then" {
  return THEN;
}

"fi" {
  return FI;
}

"while" {
  return WHILE;
}

"do" {
  return DO; 
}

"done" {
  return DONE;
}

"for" {
  return FOR;
}

"in" {
  return IN;
}

";" {
  return SEMI;
}

"exit" {
	return EXIT;
}




"source "[^ \t\n>|;]* {
	
	std::string phrase = std::string(yytext);
	
	FILE * source_file = fopen(phrase.substr(7, phrase.length() - 1).c_str(), "r");

	if (source_file != NULL) {

		YY_BUFFER_STATE buf = yy_create_buffer(source_file, YY_BUF_SIZE);

		yypush_buffer_state(buf);

		yyparse();

		yypop_buffer_state();
	}

}


\$\([^\n\)]+\) {
  yylval.cpp_string = new std::string(yytext);
	return WORD;	
}


\`[^\n\`]+\` {
	
  yylval.cpp_string = new std::string(yytext);
	return WORD;	
}


[^ \t\n>|;]*\\[^ \t\n>|;]* {
	int i = 0;

	char * ret = new char[strlen(yytext) * 2]; // account if every character is a backslash
	char * raw_text = yytext;

	while ( *raw_text) {
		if (*raw_text == '\\') {
			raw_text++;
			ret[i++] = *raw_text;
		} else {
			ret[i++] = *raw_text;
		}
		raw_text++;
	}

	ret[i] = '\0'; // set null byte

	yylval.cpp_string = new std::string(ret);
	return WORD;
}

\".*\" {
	/* Quotes */
	yylval.cpp_string = new std::string(yytext);
	yylval.cpp_string = new std::string(yylval.cpp_string->substr(1, yylval.cpp_string->length() -2 ));	
	return WORD;
}


[^ \t\n>&|;][^ \t\n>&|;]*  {
	/* Regular word check */
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
} 






