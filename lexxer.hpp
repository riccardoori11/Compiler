#include <ios>
#include <utility>
#include <iostream>
#include "token.hpp"
#include <unordered_map>
#pragma once



		class lexxer{

private:
		std::string input = "";
		std::size_t pos{};

		inline static const std::unordered_map<std::string, TokenType> keywords{
				{"int", TokenType::INT},
				{"bool", TokenType::BOOL},
				{"double", TokenType::DOUBLE},

		};
		
		
		static constexpr bool is_digit(char c){

				return c >= '0' && c <= '9';
		}
		static constexpr bool is_letter(char c){
				return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
		}

		static constexpr bool is_identifier(char c){
				return is_letter(c) || is_digit(c);
		}

		void skip_whiteSpace(){

				while (input[pos] == ' '){

						++pos;
				}
		}

		Token readIdentifier(){

				std::size_t start = pos;
				++pos;
				while (pos< input.size() && is_identifier(input[pos])){
						++pos;
				}
				
				std::string id{input.substr(start,pos-start)};

				auto it = keywords.find(id);
				if (it != keywords.end()){
						return Token(it->second,id);
				}
				
				return Token(TokenType::IDENTIFIER,std::move(id));
		}

		Token readInteger(){

				std::size_t start = pos;
				while(pos < input.size() && is_digit(input[pos])){

						++pos;
				}

				return Token(TokenType::INTEGER, input.substr(start,pos-start));
		}
public:

constexpr lexxer() = default;

constexpr lexxer(std::string input):input(std::move(input)){}

// following rule of five
// copy constructor
constexpr lexxer(const lexxer& other):input(other.input){}


// copy assignment
constexpr lexxer& operator= (const lexxer& other){

		if (this != &other){
				input = other.input;
				pos = other.pos;
		}
		
		return *this;

}

// move constructor
constexpr lexxer(lexxer&& other):input(std::exchange(input,other.input)){}

// move assignment
constexpr lexxer& operator= (lexxer&& other){

		std::swap(input,other.input);
		return *this;
}

Token nextToken(){
				
		while(pos< input.size()){

				skip_whiteSpace();

				if (is_letter(input[pos])){
						return readIdentifier();
						
				}
				else if (is_digit(input[pos])){
						return readInteger();
				}
				std::size_t start = pos;
				pos++;
				
				switch (input[start]){
						case '=':
								if (input[pos] == '='){
										pos++;
										return {Token(TokenType::COMPARE, input.substr(start,2))}; 
								}
								else{
										return {Token(TokenType::EQUAL, input.substr(start,1))};
								}
								
						case '+':
								return {Token(TokenType::PLUS, input.substr(start,1))};

						case '-':
								return {Token(TokenType::MINUS, input.substr(start,1))};

						case ';':
								return {Token(TokenType::SEMICOLON, input.substr(start,1))};
						case '*':
								return {Token(TokenType::MULTIPLICATION, input.substr(start,1))};
						case '/':
								return {Token(TokenType::MULTIPLICATION, input.substr(start,1))};
						default:
								return {Token(TokenType::ILLEGAL, input.substr(start,1))};
						
				}

		}

		return Token(TokenType::ENDOFFILE,{});
		}
		};



