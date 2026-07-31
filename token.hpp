#include <iostream>
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
};



struct Token{

		TokenType tokentype;
		std::string text;
		
		Token(TokenType tokentype, std::string text):tokentype(tokentype),text(std::move(text)){};

};



