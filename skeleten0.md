**Issue 3: keeping a retained function's defining environment alive.**

**Input →** A function remains available after the C++ scope containing its defining environment ends.

**Mechanism →** In [object.hpp](/home/riccardo/compiler/object.hpp:30), `FunctionObj::env` is a raw pointer. Assigning `func.env = &env` stores an address without extending the environment's lifetime.

The REPL's `Environment e{}` is destroyed after each iteration. A function call's `Environment call_env(...)` is destroyed when that call finishes. If a function defined in either environment survives elsewhere, its `env` pointer becomes dangling.

**Output →** A later lookup through that pointer accesses a destroyed environment: undefined behavior, possibly a crash. The shared function body does not preserve the environment.

**Possible ownership design →** Let a runtime own heap-allocated environments, for example through `unique_ptr<Environment>`. Functions borrow their stable addresses. The runtime also retains any parent environments needed for name lookup.

**Limits →**

- The runtime must outlive every retained function that uses its environments.
- Retaining all environments until runtime shutdown also retains unused memory.
- Making `FunctionObj::env` a `shared_ptr` instead creates an ownership cycle when the environment owns that function. A `weak_ptr` breaks the cycle but does not keep the environment alive.

```text
Proposed ownership:

Runtime --owns--> Environment --shared_ptr owns--> FunctionObj
                      ↑                               |
                      └──────── raw env pointer ───────┘

Runtime retains environment → borrowed env pointer remains valid
```

1. What currently controls a call environment's lifetime?
2. Must retained functions survive destruction of the runtime?
3. When could an unused environment be safely destroyed?
