#pragma once

#include "object.hpp"
#include "token.hpp"
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

struct EnvironmentFrame {
    std::unordered_map<std::string, Binding> bindings;
    const std::shared_ptr<EnvironmentFrame> parent;

    explicit EnvironmentFrame(std::shared_ptr<EnvironmentFrame> parent = nullptr)
        : parent(std::move(parent)) {}
};

class Environment {
    friend class Evaluator;

    // Environment is a handle: copies refer to the same lexical scope.
    std::shared_ptr<EnvironmentFrame> frame;

    explicit Environment(std::shared_ptr<EnvironmentFrame> parent)
        : frame(std::make_shared<EnvironmentFrame>(std::move(parent))) {}

public:
    Environment(Environment* parent = nullptr)
        : Environment(parent ? parent->frame : nullptr) {}

    std::optional<Binding> Get(const std::string& name) const {
        for (auto scope = frame; scope; scope = scope->parent) {
            const auto it = scope->bindings.find(name);
            if (it != scope->bindings.end()) {
                return it->second;
            }
        }
        return std::nullopt;
    }

    void define(const std::string& name, const Binding& binding) {
        const auto it = frame->bindings.find(name);
        if (it != frame->bindings.end()) {
            throw std::runtime_error("already exsits a variable with that name");
        }
        frame->bindings[name] = binding;
    }

    void Set(const std::string& name, const Binding& other) {
        const auto it = frame->bindings.find(name);
        if (it == frame->bindings.end()) {
            throw std::runtime_error("no variable with that name bud!!!");
        }
        if (it->second.type != other.type) {
            throw std::runtime_error("Must be matching types");
        }
        it->second.value = other.value;
    }
};
