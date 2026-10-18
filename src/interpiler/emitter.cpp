
#include "emitter.hpp"
#include "../alma.hpp"
#include "../debug.hpp"
#include "../object/cons.hpp"
#include "../object/environment.hpp"

void Emitter::handle(ObjectRef<Object> self,
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment,
    std::stringstream& output)
{
    this->emit_top_level(self, arg_list, environment, output);
}

void EmitterTopLevel::emit_top_level(ObjectRef<Object> self,
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment,
    std::stringstream& output)
{
    this->emit_impl(self, arg_list, environment, output);
}

void EmitterTopLevel::emit_non_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list [[maybe_unused]], ObjectRef<Environment> environment [[maybe_unused]],
    std::stringstream& output [[maybe_unused]])
{
    athrow("This emitter can only be called at the top level");
}

void EmitterNonTopLevel::emit_top_level(ObjectRef<Object> self [[maybe_unused]],
    const std::vector<ObjectRef<Object>>& arg_list [[maybe_unused]], ObjectRef<Environment> environment [[maybe_unused]],
    std::stringstream& output [[maybe_unused]])
{
    athrow("This emitter cannot be called at the top level");
}

void EmitterNonTopLevel::emit_non_top_level(ObjectRef<Object> self,
    const std::vector<ObjectRef<Object>>& arg_list, ObjectRef<Environment> environment,
    std::stringstream& output)
{
    this->emit_impl(self, arg_list, environment, output);
}

void EmitterNonTopLevel::emit(const std::vector<ObjectRef<Object>>& arguments, Declarations& declarations)
{
    emit_impl(arguments, declarations);
}
