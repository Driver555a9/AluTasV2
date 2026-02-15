#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

#include <ratio>

namespace CoreEngine
{
    namespace Units
    {
        template <typename Derived, typename Category, typename Rep, typename Period>
        struct Basic_Unit
        {
            using category_type = Category;
            using value_type    = Rep;
            using period        = Period;

            constexpr Basic_Unit() noexcept = default;
            constexpr explicit Basic_Unit(Rep v) noexcept : m_value(v) {}

            constexpr Derived& operator=(Rep v) noexcept
            {
                m_value = v;
                return static_cast<Derived&>(*this);
            }

            [[nodiscard]] constexpr Rep& GetReference () noexcept { return m_value; }
            [[nodiscard]] constexpr Rep Get() const noexcept { return m_value; }
            [[nodiscard]] constexpr explicit operator Rep() const noexcept { return m_value; }

            ////////////////////////////////////////////////
            // Logical comparisons
            ////////////////////////////////////////////////
            [[nodiscard]] friend constexpr bool operator==(const Derived& a, const Derived& b) noexcept
            {
                return a.m_value == b.m_value;
            }

            [[nodiscard]] friend constexpr bool operator!=(const Derived& a, const Derived& b) noexcept
            {
                return !(a == b);
            }

            [[nodiscard]] friend constexpr bool operator<(const Derived& a, const Derived& b) noexcept
            {
                return a.m_value < b.m_value;
            }

            [[nodiscard]] friend constexpr bool operator>(const Derived& a, const Derived& b) noexcept
            {
                return b < a;
            }

            [[nodiscard]] friend constexpr bool operator<=(const Derived& a, const Derived& b) noexcept
            {
                return !(b < a);
            }

            [[nodiscard]] friend constexpr bool operator>=(const Derived& a, const Derived& b) noexcept
            {
                return !(a < b);
            }

            ////////////////////////////////////////////////
            // Arithmetic (non-mutating)
            ////////////////////////////////////////////////
            [[nodiscard]] friend constexpr Derived operator+(Derived a, const Derived& b) noexcept
            {
                a += b;
                return a;
            }

            [[nodiscard]] friend constexpr Derived operator-(Derived a, const Derived& b) noexcept
            {
                a -= b;
                return a;
            }

            [[nodiscard]] friend constexpr Derived operator*(Derived a, Rep scalar) noexcept
            {
                a.m_value *= scalar;
                return a;
            }

            [[nodiscard]] friend constexpr Derived operator*(Rep scalar, Derived a) noexcept
            {
                a.m_value *= scalar;
                return a;
            }

            [[nodiscard]] friend constexpr Derived operator/(Derived a, Rep scalar) noexcept
            {
                a.m_value /= scalar;
                return a;
            }

            ////////////////////////////////////////////////
            // Mutating
            ////////////////////////////////////////////////
            constexpr Derived& operator+=(const Derived& b) noexcept
            {
                m_value += b.m_value;
                return static_cast<Derived&>(*this);
            }

            constexpr Derived& operator-=(const Derived& b) noexcept
            {
                m_value -= b.m_value;
                return static_cast<Derived&>(*this);
            }

            constexpr Derived& operator*=(Rep scalar) noexcept
            {
                m_value *= scalar;
                return static_cast<Derived&>(*this);
            }

            constexpr Derived& operator/=(Rep scalar) noexcept
            {
                m_value /= scalar;
                return static_cast<Derived&>(*this);
            }

        protected:
            Rep m_value{};
        };

        namespace UnitCategory
        {
            struct Time     {};
            struct Mass     {};
            struct Distance {};
            struct Velocity {};
            struct Force    {};
            struct Angle    {};
        }

        #define DEFINE_UNIT(name, category, rep, ratio)       \
            struct name : Basic_Unit<name, category, rep, ratio> { \
                using Basic_Unit<name, category, rep, ratio>::Basic_Unit; \
            };

        DEFINE_UNIT(Second,         UnitCategory::Time,       double,         std::ratio<1>)
        DEFINE_UNIT(MicroSecond,    UnitCategory::Time,       std::int64_t,   std::micro   )
        DEFINE_UNIT(MilliSecond,    UnitCategory::Time,       std::int64_t,   std::milli   )

        DEFINE_UNIT(Kilogram,       UnitCategory::Mass,       float,          std::ratio<1>)

        DEFINE_UNIT(Meter,          UnitCategory::Distance,   float,          std::ratio<1>)

        DEFINE_UNIT(Meters_per_sec, UnitCategory::Velocity,   float,          std::ratio<1>)

        DEFINE_UNIT(Newtons,        UnitCategory::Force,      float,          std::ratio<1>)

        DEFINE_UNIT(Degrees,        UnitCategory::Angle,      float,          std::ratio<1>)

        #undef DEFINE_UNIT

        template <typename T>
        concept Is_Unit = requires { typename T::category_type; typename T::value_type; typename T::period; };
        
        template <typename T>
        concept Is_Time_Unit = Is_Unit<T> && std::same_as<typename T::category_type, UnitCategory::Time>;

        template <typename T>
        concept Is_Mass_Unit = Is_Unit<T> && std::same_as<typename T::category_type, UnitCategory::Mass>;

        template <typename T>
        concept Is_Distance_Unit = Is_Unit<T> && std::same_as<typename T::category_type, UnitCategory::Distance>;

        template <typename T>
        concept Is_Velocity_Unit = Is_Unit<T> && std::same_as<typename T::category_type, UnitCategory::Velocity>;

        template <typename T>
        concept Is_Force_Unit = Is_Unit<T> && std::same_as<typename T::category_type, UnitCategory::Force>;

        template <typename T>
        concept Is_Angle_Unit = Is_Unit<T> && std::same_as<typename T::category_type, UnitCategory::Angle>;

        template <typename A, typename B>
        concept Is_Same_Unit_Category = Is_Unit<A> && Is_Unit<B> && std::same_as<typename A::category_type, typename B::category_type>;

        template <typename To, typename From>
        requires ( Is_Same_Unit_Category<To, From> ) 
        [[nodiscard]] constexpr inline To Convert(const From& from) noexcept
        {
            using FromPeriod = typename From::period;
            using ToPeriod   = typename To::period;

            constexpr const double factor = static_cast<double>(FromPeriod::num) / FromPeriod::den * static_cast<double>(ToPeriod::den) / ToPeriod::num;

            return To(static_cast<typename To::value_type>(from.Get() * factor));
        }
    }
}