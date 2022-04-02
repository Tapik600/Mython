#include "statement.h"

#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
} // namespace

ObjectHolder VariableValue::Execute(Closure &closure, Context & /*context*/) {
    if (dotted_ids_.empty()) {
        throw std::runtime_error("Dotted ids cannot by empty"s);
    }

    if (const auto it = closure.find(dotted_ids_[0]); it != closure.end()) {
        ObjectHolder obj = it->second;

        if (dotted_ids_.size() > 1) {
            for (size_t i = 1; i < dotted_ids_.size() - 1; ++i) {
                if (auto *class_ptr = obj.TryAs<runtime::ClassInstance>()) {
                    if (const auto item = class_ptr->Fields().find(dotted_ids_[i]);
                        item != class_ptr->Fields().end()) {
                        obj = item->second;
                        continue;
                    }
                }
                throw std::runtime_error("Cannot find class"s);
            }
            const auto &fields = obj.TryAs<runtime::ClassInstance>()->Fields();
            if (const auto it = fields.find(dotted_ids_.back()); it != fields.end()) {
                return it->second;
            }

        } else {
            return obj;
        }
    }
    throw std::runtime_error("Cannot find class"s);
}

ObjectHolder Assignment::Execute(Closure &closure, Context &context) {
    closure[var_] = rv_->Execute(closure, context);
    return closure.at(var_);
}

ObjectHolder Print::Execute(Closure &closure, Context &context) {
    std::string delim{};
    for (const auto &arg : args_) {
        const ObjectHolder value = arg->Execute(closure, context);
        context.GetOutputStream() << delim;

        if (value) {
            value->Print(context.GetOutputStream(), context);
        } else {
            context.GetOutputStream() << "None"s;
        }
        delim = " "s;
    }
    context.GetOutputStream() << '\n';
    return ObjectHolder::None();
}

ObjectHolder MethodCall::Execute(Closure &closure, Context &context) {
    std::vector<runtime::ObjectHolder> object_args;
    for (const auto &arg : args_) {
        object_args.push_back(arg->Execute(closure, context));
    }

    auto *cls = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();
    if (!cls) {
        throw std::runtime_error("Cannot find class"s);
    }

    return cls->Call(method_, object_args, context);
}

ObjectHolder Stringify::Execute(Closure &closure, Context &context) {
    std::ostringstream out;
    const ObjectHolder obj = arg_->Execute(closure, context);

    if (obj) {
        obj->Print(out, context);
    } else {
        out << "None"s;
    }
    return ObjectHolder::Own(runtime::String(out.str()));
}

ObjectHolder Add::Execute(Closure &closure, Context &context) {
    auto obj_lhs = lhs_->Execute(closure, context);
    auto obj_rhs = rhs_->Execute(closure, context);

    if (auto lhs_inst = obj_lhs.TryAs<runtime::ClassInstance>()) {
        return lhs_inst->Call(ADD_METHOD, {obj_rhs}, context);
    }

    if (obj_lhs.TryAs<runtime::Number>() && obj_rhs.TryAs<runtime::Number>()) {
        return ObjectHolder::Own(
            runtime::Number(obj_lhs.TryAs<runtime::Number>()->GetValue() +
                            obj_rhs.TryAs<runtime::Number>()->GetValue()));
    }

    if (obj_lhs.TryAs<runtime::String>() && obj_rhs.TryAs<runtime::String>()) {
        return ObjectHolder::Own(
            runtime::String(obj_lhs.TryAs<runtime::String>()->GetValue() +
                            obj_rhs.TryAs<runtime::String>()->GetValue()));
    }

    throw std::runtime_error("Cannot sum objects"s);
}

ObjectHolder Sub::Execute(Closure &closure, Context &context) {
    auto obj_lhs = lhs_->Execute(closure, context);
    auto obj_rhs = rhs_->Execute(closure, context);

    if (obj_lhs.TryAs<runtime::Number>() && obj_rhs.TryAs<runtime::Number>()) {
        return ObjectHolder::Own(
            runtime::Number(obj_lhs.TryAs<runtime::Number>()->GetValue() -
                            obj_rhs.TryAs<runtime::Number>()->GetValue()));
    }

    throw std::runtime_error("Cannot sub objects"s);
}

ObjectHolder Mult::Execute(Closure &closure, Context &context) {
    auto obj_lhs = lhs_->Execute(closure, context);
    auto obj_rhs = rhs_->Execute(closure, context);

    if (obj_lhs.TryAs<runtime::Number>() && obj_rhs.TryAs<runtime::Number>()) {
        return ObjectHolder::Own(
            runtime::Number(obj_lhs.TryAs<runtime::Number>()->GetValue() *
                            obj_rhs.TryAs<runtime::Number>()->GetValue()));
    }

    throw std::runtime_error("Cannot multiply objects"s);
}

ObjectHolder Div::Execute(Closure &closure, Context &context) {
    auto obj_lhs = lhs_->Execute(closure, context);
    auto obj_rhs = rhs_->Execute(closure, context);

    if (obj_lhs.TryAs<runtime::Number>() && obj_rhs.TryAs<runtime::Number>()) {
        if (obj_rhs.TryAs<runtime::Number>()->GetValue() == 0) {
            throw std::runtime_error("division by zero"s);
        }
        return ObjectHolder::Own(
            runtime::Number(obj_lhs.TryAs<runtime::Number>()->GetValue() /
                            obj_rhs.TryAs<runtime::Number>()->GetValue()));
    }

    throw std::runtime_error("Cannot division objects"s);
}

ObjectHolder Compound::Execute(Closure &closure, Context &context) {
    for (const auto &statement : statements_) {
        statement->Execute(closure, context);
    }
    return {};
}

ObjectHolder Return::Execute(Closure &closure, Context &context) {
    throw statement_->Execute(closure, context);
}

ObjectHolder ClassDefinition::Execute(Closure &closure, Context & /*context*/) {
    closure[class_.TryAs<runtime::Class>()->GetName()] = class_;
    return class_;
}

ObjectHolder FieldAssignment::Execute(Closure &closure, Context &context) {
    auto *cls = object_.Execute(closure, context).TryAs<runtime::ClassInstance>();
    if (!cls) {
        throw std::runtime_error("Cannot find class"s);
    }

    cls->Fields()[field_name_] = rv_->Execute(closure, context);
    return cls->Fields()[field_name_];
}

ObjectHolder IfElse::Execute(Closure &closure, Context &context) {
    if (runtime::IsTrue(condition_->Execute(closure, context))) {
        return if_body_->Execute(closure, context);
    }
    if (else_body_) {
        return else_body_->Execute(closure, context);
    }

    return {};
}

ObjectHolder Or::Execute(Closure &closure, Context &context) {
    return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(lhs_->Execute(closure, context)) ||
                                           runtime::IsTrue(rhs_->Execute(closure, context))));
}

ObjectHolder And::Execute(Closure &closure, Context &context) {
    return ObjectHolder::Own(runtime::Bool(runtime::IsTrue(lhs_->Execute(closure, context)) &&
                                           runtime::IsTrue(rhs_->Execute(closure, context))));
}

ObjectHolder Not::Execute(Closure &closure, Context &context) {
    return ObjectHolder::Own(runtime::Bool(!runtime::IsTrue(arg_->Execute(closure, context))));
}

ObjectHolder Comparison::Execute(Closure &closure, Context &context) {
    return ObjectHolder::Own(runtime::Bool(
        cmp_(lhs_->Execute(closure, context), rhs_->Execute(closure, context), context)));
}

ObjectHolder NewInstance::Execute(Closure &closure, Context &context) {
    ObjectHolder obj = ObjectHolder::Own(runtime::ClassInstance(class_));
    auto new_instance = obj.TryAs<runtime::ClassInstance>();
    if (new_instance && new_instance->HasMethod(INIT_METHOD, args_.size())) {
        std::vector<runtime::ObjectHolder> new_args;
        for (const auto &arg : args_) {
            new_args.push_back(arg->Execute(closure, context));
        }
        new_instance->Call(INIT_METHOD, new_args, context);
    }
    return obj;
}

ObjectHolder MethodBody::Execute(Closure &closure, Context &context) {
    ObjectHolder result = ObjectHolder::None();

    try {
        result = body_->Execute(closure, context);
    } catch (ObjectHolder &obj) { result = std::move(obj); }

    return result;
}

} // namespace ast