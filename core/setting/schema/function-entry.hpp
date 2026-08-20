#pragma once

#include <functional>
#include <lua.hpp>
#include <memory>

#include "common/common.hpp"
#include "core/setting/engine.hpp"
#include "core/setting/schema/table.hpp"

namespace yabe::core::setting::schema {
// to store any signature of function into std::vector in Engine
class FunctionEntryBase {
 public:
  DISABLE_MOVE_AND_COPY(FunctionEntryBase);
  explicit FunctionEntryBase()             = default;
  virtual ~FunctionEntryBase()             = default;
  virtual int invoke(Engine* engine) const = 0;
};

template <typename>
class FunctionEntry : public FunctionEntryBase {};
// template specialization
template <typename ReturnType, typename... Args>
class FunctionEntry<ReturnType(Args...)> : public FunctionEntryBase {
 public:
  using FunctionType = std::function<ReturnType(Args...)>;

  DISABLE_MOVE_AND_COPY(FunctionEntry);
  explicit FunctionEntry(FunctionType func) : func_(std::move(func)) {
  }
  ~FunctionEntry() override = default;

  static std::unique_ptr<FunctionEntryBase> create(FunctionType func) {
    return std::make_unique<FunctionEntry<ReturnType(Args...)>>(
        std::move(func));
  }

  int invoke(Engine* engine) const override {
    bool has_optional = false;
    auto get_arg      = [&]<typename T>(std::size_t index) {
      auto arg = engine->get_stack_value<T>(static_cast<int>(index));
      if (!arg) {
        engine->add_error_message(
            std::format("function argument {} is invalid", index));
        has_optional = true;
      }
      return arg;
    };

    auto args = [&]<std::size_t... I>(std::index_sequence<I...> /*seq*/) {
      return std::tuple{get_arg.template operator()<Args>(I + 1)...};
    }(std::index_sequence_for<Args...>{});
    if (has_optional) {
      return 0;
    }
    const auto args_valid = std::apply(
        [](auto&&... args) {
          return std::tuple{*args...};
        },
        std::move(args));

    if constexpr (std::is_same_v<ReturnType, void>) {
      std::apply(this->func_, std::move(args_valid));
      return 0;
    } else {
      ReturnType result = std::apply(this->func_, std::move(args_valid));
      engine->push_stack_value(result);
      return 1;
    }
  }

 private:
  FunctionType func_;
};

template <typename ReturnType, typename... Fields>
class FunctionEntry<ReturnType(Table<Fields...>)> : public FunctionEntryBase {
 public:
  using Schema       = Table<Fields...>;
  using ArgumentType = TableValue<Schema>;
  using FunctionType = std::function<ReturnType(ArgumentType)>;

  DISABLE_MOVE_AND_COPY(FunctionEntry);
  explicit FunctionEntry(const Schema& schema, FunctionType func)
      : schema_(schema), func_(std::move(func)) {
  }
  ~FunctionEntry() override = default;

  static std::unique_ptr<FunctionEntryBase> create(
      const Schema& schema, FunctionType func) {
    return std::make_unique<FunctionEntry<ReturnType(Schema)>>(
        schema, std::move(func));
  }

  int invoke(Engine* engine) const override {
    auto argument = engine->get_stack_value(1, this->schema_);
    if (!argument) {
      engine->add_error_message("function argument 1 is invalid");
      return 0;
    }

    if constexpr (std::is_same_v<ReturnType, void>) {
      this->func_(std::move(*argument));
      return 0;
    } else {
      ReturnType result = this->func_(std::move(*argument));
      engine->push_stack_value(result);
      return 1;
    }
  }

 private:
  Schema       schema_;
  FunctionType func_;
};
}  // namespace yabe::core::setting::schema
