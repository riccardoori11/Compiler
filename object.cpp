#include "object.hpp"
#include <format>
#include <string_view>
#include <type_traits>
#include <variant>

std::string Inspect(const Value& value){

		return std::visit([](const auto& item){

						return std::format("{}",item);

						},value);
}

std::string_view type(const Value& value){

		return std::visit([](const auto& item) -> std::string_view{
				if constexpr (std::is_same_v<decltype(item), int>){
						return "int";
				}
				else{

						return "bool";
				}
						},value);
}


