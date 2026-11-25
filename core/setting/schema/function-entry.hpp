#pragma once

#include <functional>
#include <lua.hpp>
#include <memory>
#include <string>
#include <vector>

#include "common/common.hpp"
#include "core/setting/engine.hpp"
#include "core/setting/schema/table.hpp"

#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
#include "core/setting/reference.hpp"
#include "core/setting/schema/describe.hpp"
#endif

namespace yabe::core::setting::schema {
struct FunctionMetadata {
  std::string              description;
  std::vector<std::string> parameter_names;
};

// to store any signature of function into std::vector in Engine
class FunctionEntryBase {
 public:
  DISABLE_MOVE_AND_COPY(FunctionEntryBase);
  explicit FunctionEntryBase()             = default;
  virtual ~FunctionEntryBase()             = default;
  virtual int invoke(Engine* engine) const = 0;
#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  /// @brief describes this function for the reference
  /// @param name the field name of this function
  /// @param table_name unique name of the table passed to this function
  /// @param tables the table passed to this function is appended to it
  virtual reference::Function describe(const std::string& name,
      const std::string&                                  table_name,
      std::vector<reference::Table>&                      tables) const = 0;
#endif
};

template <typename>
class FunctionEntry : public FunctionEntryBase {};
// template specialization
template <typename ReturnType, typename... Args>
class FunctionEntry<ReturnType(Args...)> : public FunctionEntryBase {
 public:
  using FunctionType = std::function<ReturnType(Args...)>;

  DISABLE_MOVE_AND_COPY(FunctionEntry);
  explicit FunctionEntry(FunctionMetadata metadata, FunctionType func)
      : metadata_(std::move(metadata)), func_(std::move(func)) {
  }
  ~FunctionEntry() override = default;

  static std::unique_ptr<FunctionEntryBase> create(
      FunctionMetadata metadata, FunctionType func) {
    return std::make_unique<FunctionEntry<ReturnType(Args...)>>(
        std::move(metadata), std::move(func));
  }

  int invoke(Engine* engine) const override {
    bool has_optional = false;
    auto get_arg      = [&]<typename T>(std::size_t index) {
      auto arg = engine->get_stack_value<T>(static_cast<int>(index));
      if (!arg) {
        LOG_ERR("function argument {} is invalid", index);
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

#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  reference::Function describe(const std::string& name,
      const std::string& /*table_name*/,
      std::vector<reference::Table>& /*tables*/) const override {
    reference::Function result{
        .name        = name,
        .description = this->metadata_.description,
        .parameters  = {},
        .return_type = describe_type<ReturnType>(),
    };
    auto describe_argument = [this, &result]<typename Argument>() {
      const auto index = result.parameters.size();
      result.parameters.push_back({
          .name = index < this->metadata_.parameter_names.size() ?
                      this->metadata_.parameter_names[index] :
                      "arg" + std::to_string(index + 1),
          .type = describe_type<Argument>(),
      });
    };
    (describe_argument.template operator()<Args>(), ...);
    return result;
  }
#endif

 private:
  FunctionMetadata metadata_;
  FunctionType     func_;
};

template <typename ReturnType, typename... Fields>
class FunctionEntry<ReturnType(Table<Fields...>)> : public FunctionEntryBase {
 public:
  using Schema       = Table<Fields...>;
  using ArgumentType = TableValue<Schema>;
  using FunctionType = std::function<ReturnType(ArgumentType)>;

  DISABLE_MOVE_AND_COPY(FunctionEntry);
  explicit FunctionEntry(
      const Schema& schema, FunctionMetadata metadata, FunctionType func)
      : schema_(schema)
      , metadata_(std::move(metadata))
      , func_(std::move(func)) {
  }
  ~FunctionEntry() override = default;

  static std::unique_ptr<FunctionEntryBase> create(
      const Schema& schema, FunctionMetadata metadata, FunctionType func) {
    return std::make_unique<FunctionEntry<ReturnType(Schema)>>(
        schema, std::move(metadata), std::move(func));
  }

  int invoke(Engine* engine) const override {
    auto argument = engine->get_stack_value(1, this->schema_);
    if (!argument) {
      LOG_ERR("function argument 1 is invalid");
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

#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  reference::Function describe(const std::string& name,
      const std::string&                          table_name,
      std::vector<reference::Table>&              tables) const override {
    describe_table(this->schema_, table_name, "", tables);
    return {
        .name        = name,
        .description = this->metadata_.description,
        .parameters  = {{
            .name = this->metadata_.parameter_names.empty() ?
                        "arg1" :
                        this->metadata_.parameter_names.front(),
            .type = describe_table_type(table_name),
        }},
        .return_type = describe_type<ReturnType>(),
    };
  }
#endif

 private:
  Schema           schema_;
  FunctionMetadata metadata_;
  FunctionType     func_;
};
}  // namespace yabe::core::setting::schema
