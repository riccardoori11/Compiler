#pragma once

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

std::string_view type(const auto& value){

		return std::visit([](const auto& item) -> std::string_view{
						using TYPE_NORM = std::remove_cvref_t<decltype(item)>;
				if constexpr (std::is_same_v<TYPE_NORM, int>){
						return "int";
				}
				else if constexpr (std::is_same_v<TYPE_NORM, float>) {
						return "float";
				}
				else if constexpr (std::is_same_v<TYPE_NORM, double>) {
						return "double";
				}
				else{

						return "bool";
				}
						},value);
}


