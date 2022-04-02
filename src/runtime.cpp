#include "runtime.h"

#include <cassert>
#include <optional>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data) : data_(std::move(data)) {}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object &object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(
        std::shared_ptr<Object>(&object, [](auto * /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object &ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object *ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object *ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder &object) {
    if (const auto *ptr = object.TryAs<Number>()) {
        return ptr->GetValue() != 0;
    }
    if (const auto *ptr = object.TryAs<String>()) {
        return !ptr->GetValue().empty();
    }
    if (const auto *ptr = object.TryAs<Bool>()) {
        return ptr->GetValue();
    }
    return false;
}

void ClassInstance::Print(std::ostream &os, Context &context) {
    if (HasMethod("__str__"s, 0)) {
        Call("__str__"s, {}, context).Get()->Print(os, context);
    } else {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string &method, size_t argument_count) const {
    if (const Method *method_ptr = class_.GetMethod(method)) {
        return method_ptr->formal_params.size() == argument_count;
    }
    return false;
}

ObjectHolder ClassInstance::Call(const std::string &method,
                                 const std::vector<ObjectHolder> &actual_args,
                                 Context &context) {
    if (HasMethod(method, actual_args.size())) {
        Closure args;
        args["self"s] = ObjectHolder::Share(*this);

        const Method *method_ptr = class_.GetMethod(method);

        for (size_t i = 0; i < actual_args.size(); ++i) {
            args[method_ptr->formal_params[i]] = actual_args[i];
        }
        return method_ptr->body->Execute(args, context);
    }

    throw std::runtime_error("Method "s + method + " not found"s);
}

const Method *Class::GetMethod(const std::string &name) const {
    for (const auto &method : methods_) {
        if (method.name == name) {
            return &method;
        }
    }
    if (parent_) {
        for (const auto &method : parent_->methods_) {
            if (method.name == name) {
                return &method;
            }
        }
    }
    return nullptr;
}

void Class::Print(ostream &os, [[maybe_unused]] Context &context) {
    os << "Class "sv << name_;
}

void Bool::Print(std::ostream &os, [[maybe_unused]] Context &context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
        return lhs.TryAs<Number>()->GetValue() == rhs.TryAs<Number>()->GetValue();
    }
    if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
        return lhs.TryAs<String>()->GetValue() == rhs.TryAs<String>()->GetValue();
    }
    if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
        return lhs.TryAs<Bool>()->GetValue() == rhs.TryAs<Bool>()->GetValue();
    }
    if (!lhs && !rhs) {
        return true;
    }
    if (lhs.TryAs<ClassInstance>() && lhs.TryAs<ClassInstance>()->HasMethod("__eq__"s, 1)) {
        return lhs.TryAs<ClassInstance>()
            ->Call("__eq__"s, {rhs}, context)
            .TryAs<Bool>()
            ->GetValue();
    }
    throw std::runtime_error("Cannot compare objects for equality"s);
}

bool Less(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    if (lhs.TryAs<Number>() && rhs.TryAs<Number>()) {
        return lhs.TryAs<Number>()->GetValue() < rhs.TryAs<Number>()->GetValue();
    }
    if (lhs.TryAs<String>() && rhs.TryAs<String>()) {
        return lhs.TryAs<String>()->GetValue() < rhs.TryAs<String>()->GetValue();
    }
    if (lhs.TryAs<Bool>() && rhs.TryAs<Bool>()) {
        return lhs.TryAs<Bool>()->GetValue() < rhs.TryAs<Bool>()->GetValue();
    }
    if (lhs.TryAs<ClassInstance>() && lhs.TryAs<ClassInstance>()->HasMethod("__lt__"s, 1)) {
        return lhs.TryAs<ClassInstance>()
            ->Call("__lt__"s, {rhs}, context)
            .TryAs<Bool>()
            ->GetValue();
    }
    throw std::runtime_error("Cannot compare objects for less"s);
}

bool NotEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    return !Equal(lhs, rhs, context);
}

bool Greater(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
}

bool LessOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    return !Greater(lhs, rhs, context);
}

bool GreaterOrEqual(const ObjectHolder &lhs, const ObjectHolder &rhs, Context &context) {
    return !Less(lhs, rhs, context);
}

} // namespace runtime