#pragma once

#include "object.hpp"

#include "token.hpp"
#include <optional>
#include <stdexcept>
#include <unordered_map>

class Environment{


private:
		std::unordered_map<std::string, Binding> map;

public:

Environment() = default;

std::optional<Binding> Get(const std::string& name) const{

		auto it = map.find(name);

		if (it == map.end()){
				return std::nullopt;
		}

		return it->second;
}

void define(const std::string& name, const Binding& binding){

		auto it = map.find(name);

		if (it == map.end()){

				map[name] = binding;
		}
		else{
				throw std::runtime_error("already exsits a variable with that name");
		}
}

void Set(const std::string& name,const Binding& other){

		auto it = map.find(name);

		if (it != map.end()){
				if (it->second.type == other.type){
						it->second.value = other.value;
				}
				else{

						throw std::runtime_error("Must be matching types");
				}
				
		}
		else{

				throw std::runtime_error("no variable with that name bud!!!");
		}

}


};
