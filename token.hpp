#include <string>

#pragma once

enum class TokenType{

		IDENTIFIER,
		EQUAL,
		INTEGER,
		PLUS,
		MINUS,
		SEMICOLON,
		ENDOFFILE,
		ILLEGAL,
		MULTIPLICATION,
		INT,
		BOOL,
		DOUBLE,
		COMPARE,
		RETURN,
		LESS,
		GREATER,
		DIVISION,
		NOT,
		LPARENT,
		RPARENT,
		LBRAC,
		RBRAC,
		IF,
		ELSE,
		COMMA,
		FLOAT
};



struct Token{

		TokenType tokentype;
		std::string text;
		
		Token(TokenType tokentype, std::string text):tokentype(tokentype),text(std::move(text)){};

};



