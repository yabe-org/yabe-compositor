#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace yabe::core::setting::schema {
template <std::size_t N>
struct FixedString {
  std::array<char, N> value;

  // NOLINTNEXTLINE
  constexpr FixedString(const char (&source)[N]) {
    std::copy_n(source, N, value.data());
  }
  constexpr explicit operator std::string_view() const {
    return {value.data(), N - 1};
  }
};

template <FixedString Name, typename Type>
struct Field {
  using ValueType = Type;

  static constexpr auto kName = Name;
  std::string_view      description;
  Type                  default_value;

  constexpr Field(std::string_view description, Type&& default_value)
      : description(description), default_value(std::move(default_value)) {
  }
};

template <FixedString Name>
struct Field<Name, std::string> {
  using ValueType = std::string;

  static constexpr auto kName = Name;
  std::string_view      description;
  std::string_view      default_value;

  constexpr Field(std::string_view description, std::string_view default_value)
      : description(description), default_value(default_value) {
  }
};

template <typename... Fields>
struct Table {
  std::tuple<Fields...> fields;

  constexpr explicit Table(Fields... fields) : fields(std::move(fields)...) {
  }

  template <FixedString Name, FixedString... Path>
  constexpr decltype(auto) get() const {
    // count the number of fields
    // that have the same name with given Name (a head of Path)
    constexpr std::array kMatches{
        (static_cast<std::string_view>(Fields::kName) ==
            static_cast<std::string_view>(Name))...};
    constexpr auto kNumMatches =
        std::count(kMatches.begin(), kMatches.end(), true);
    static_assert(kNumMatches != 0, "Unknown table field");
    static_assert(kNumMatches == 1, "Duplicate table field");
    // field index is the distance between begin() and the first true value
    constexpr auto kFieldIndex = std::distance(
        kMatches.begin(), std::find(kMatches.begin(), kMatches.end(), true));

    const auto& field = std::get<kFieldIndex>(this->fields);
    if constexpr (sizeof...(Path) == 0) {
      // There is no remaining path, so this is the goal
      return field;
    } else if constexpr (requires { field.type; }) {
      // search recursively
      return field.type.template get<Path...>();
    } else {
      // `field` does not have `type` member
      static_assert(sizeof...(Path) == 0, "Cannot traverse a non-table field");
    }
  }
};

template <FixedString Name, typename... Fields>
struct Field<Name, Table<Fields...>> {
  using ValueType = Table<Fields...>;

  static constexpr auto kName = Name;
  std::string_view      description;
  ValueType             type;

  constexpr Field(std::string_view description, ValueType type)
      : description(description), type(std::move(type)) {
  }
};

template <typename Schema>
class TableValue;

template <typename FieldType>
class FieldValue;

template <FixedString Name, typename Type>
class FieldValue<Field<Name, Type>> {
 public:
  using FieldType = Field<Name, Type>;
  using DefaultValueType =
      std::remove_cvref_t<decltype(std::declval<FieldType>().default_value)>;

  constexpr explicit FieldValue(const FieldType& field)
      : default_value_(field.default_value) {
  }

  [[nodiscard]] constexpr Type get() const {
    if (this->value_) {
      return *this->value_;
    }
    return Type{this->default_value_};
  }

  constexpr void set(Type value) {
    this->value_ = std::move(value);
  }

 private:
  DefaultValueType    default_value_;
  std::optional<Type> value_;
};

template <FixedString Name, typename... Fields>
class FieldValue<Field<Name, Table<Fields...>>>;

template <typename... Fields>
class TableValue<Table<Fields...>> {
 public:
  using Schema = Table<Fields...>;

  constexpr explicit TableValue(const Schema& schema)
      : values_(std::apply(
            [](const Fields&... fields) {
              return std::tuple<FieldValue<Fields>...>{
                  FieldValue<Fields>{fields}...};
            },
            schema.fields)) {
  }

  template <FixedString Name, FixedString... Path>
  [[nodiscard]] constexpr decltype(auto) get() const {
    constexpr auto kFieldIndex = TableValue::find_field_index<Name>();
    const auto&    field       = std::get<kFieldIndex>(this->values_);

    if constexpr (sizeof...(Path) == 0) {
      return field.get();
    } else if constexpr (requires { field.get().template get<Path...>(); }) {
      return field.get().template get<Path...>();
    } else {
      static_assert(sizeof...(Path) == 0, "Cannot traverse a non-table value");
    }
  }

  template <FixedString Name, FixedString... Path, typename Type>
  constexpr void set(Type&& value) {
    constexpr auto kFieldIndex = TableValue::find_field_index<Name>();
    auto&          field       = std::get<kFieldIndex>(this->values_);

    if constexpr (sizeof...(Path) == 0) {
      field.set(std::forward<Type>(value));
    } else if constexpr (requires {
                           field.get().template set<Path...>(
                               std::forward<Type>(value));
                         }) {
      field.get().template set<Path...>(std::forward<Type>(value));
    } else {
      static_assert(sizeof...(Path) == 0, "Cannot traverse a non-table value");
    }
  }

 private:
  template <FixedString Name>
  static consteval std::size_t find_field_index() {
    constexpr auto kMatches = (std::size_t{0} + ... +
                               (static_cast<std::string_view>(Fields::kName) ==
                                           static_cast<std::string_view>(Name) ?
                                       std::size_t{1} :
                                       std::size_t{0}));
    static_assert(kMatches != 0, "Unknown table field");
    static_assert(kMatches == 1, "Duplicate table field");

    std::size_t current = 0;
    std::size_t result  = 0;
    (((static_cast<std::string_view>(Fields::kName) ==
                  static_cast<std::string_view>(Name) ?
              result = current :
              result),
         ++current),
        ...);
    return result;
  }

  std::tuple<FieldValue<Fields>...> values_;
};

template <FixedString Name, typename... Fields>
class FieldValue<Field<Name, Table<Fields...>>> {
 public:
  using FieldType = Field<Name, Table<Fields...>>;
  using ValueType = TableValue<Table<Fields...>>;

  constexpr explicit FieldValue(const FieldType& field) : value_(field.type) {
  }

  [[nodiscard]] constexpr const ValueType& get() const {
    return this->value_;
  }
  [[nodiscard]] constexpr ValueType& get() {
    return this->value_;
  }

  constexpr void set(ValueType value) {
    this->value_ = std::move(value);
  }

 private:
  ValueType value_;
};

// template <typename... Fields>
// TableValue(const Table<Fields...>&) -> TableValue<Table<Fields...>>;

// helper functions
template <typename... Fields>
constexpr auto table(Fields&&... fields) {
  return Table<std::decay_t<Fields>...>{std::forward<Fields>(fields)...};
}

template <FixedString Name, typename Type, typename DefaultValue>
constexpr auto field(
    std::string_view description, DefaultValue&& default_value) {
  return Field<Name, Type>{
      description,
      std::forward<DefaultValue>(default_value),
  };
}

template <FixedString Name, typename... Fields>
constexpr auto field(Table<Fields...> table) {
  return Field<Name, Table<Fields...>>{
      "",
      std::move(table),
  };
}
}  // namespace yabe::core::setting::schema
