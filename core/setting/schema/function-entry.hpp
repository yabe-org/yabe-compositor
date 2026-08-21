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
#include "core/setting/schema/metafile.hpp"

namespace yabe::core::setting::schema {
struct FunctionMetafileText {
  std::string type;
  std::string definitions;
  std::string description;
};
}  // namespace yabe::core::setting::schema
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
  virtual FunctionMetafileText generate_metafile_text(
      std::string_view class_name) const = 0;
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

#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  FunctionMetafileText generate_metafile_text(
      std::string_view /*class_name*/) const override {
    std::stringstream type;
    type << "fun(";
    std::size_t index          = 0;
    auto        write_argument = [this, &type, &index]<typename Argument>() {
      if (index != 0) {
        type << ", ";
      }
      const auto name = index < this->metadata_.parameter_names.size() ?
                            this->metadata_.parameter_names[index] :
                            "arg" + std::to_string(index + 1);
      type << name << ": " << metafile_type_name<Argument>();
      ++index;
    };
    (write_argument.template operator()<Args>(), ...);
    type << "): " << metafile_type_name<ReturnType>();
    return {
        .type        = type.str(),
        .definitions = {},
        .description = this->metadata_.description,
    };
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

#ifdef YABE_SETTING_ENABLE_META_FILE_GENERATION
  FunctionMetafileText generate_metafile_text(
      std::string_view class_name) const override {
    const auto argument_name = this->metadata_.parameter_names.empty() ?
                                   "arg1" :
                                   this->metadata_.parameter_names.front();
    return {
        .type        = std::format("fun({}: {}): {}", argument_name, class_name,
            metafile_type_name<ReturnType>()),
        .definitions = generate_table_metafile_text(
            this->schema_, std::string(class_name)),
        .description = this->metadata_.description,
    };
  }
#endif

 private:
  Schema           schema_;
  FunctionMetadata metadata_;
  FunctionType     func_;
};
}  // namespace yabe::core::setting::schema
