#pragma once

#include <genoaf/misc.hpp>

#include <type_traits>

namespace genoaf
{
    template<typename Type,
             typename Container,
             bool kSkipEqTest = false>
    class Property
    {
        template<typename T>
        constexpr static bool kIsProperty = std::is_same_v<std::decay_t<T>,
                                                           std::decay_t<Property>>;

    public:
        explicit Property(Container & container)
            : _container(container)
            , _value()
        {}

        explicit Property(Container & container, Type && value)
            : _container(container)
            , _value(std::move(value))
        {}

        explicit Property(Container & container, Type const & value)
            : _container(container)
            , _value(value)
        {}

        template<typename... Args>
        explicit Property(Container & container, Args && ... args)
            : _container(container)
            , _value(std::forward<Args>(args)...)
        {}

        explicit Property(Property const & other)
            : _container(other._container)
            , _value(other._value)
        {}

        explicit Property(Property && other)
            : _container(other._container)
            , _value(std::move(other._value))
        {}

    public:
        void invalidate()
        {
            if (!_dirty)
            {
                _dirty = true;
                _container.markDirty(*this);
            }
        }

        bool dirty() const { return _dirty; }

        void revalidate() { _dirty = false; }

    public:
        Type const & get() const { return _value; }
        Type const & operator*() const { return get(); }
        Type const * operator->() const { return &get(); }

    public:
        template<typename T, typename = std::enable_if_t<!kIsProperty<T>>>
        void set(T && value)
        {
            bool apply = false;
            if constexpr(kSkipEqTest)
            {
                apply = true;
            }
            else
            {
                apply = value != _value;
            }

            if (apply)
            {
                _value = std::forward<T>(value);
                invalidate();
            }
        }

        void set(Property const & newValue)
        {
            set(newValue._value);
        }

        void set(Property && newValue)
        {
            set(std::move(newValue._value));
        }

        Type & operator*() { invalidate(); return _value; }

        Type * operator->() { return &(operator *()); }

    public:
        template<typename T, typename = std::enable_if_t<!kIsProperty<T>>>
        Property & operator=(T && newValue)
        {
            set(std::forward<T>(newValue));
            return *this;
        }

        Property & operator=(Property const & newValue)
        {
            set(newValue);
            return *this;
        }

        Property & operator=(Property && newValue)
        {
            set(std::move(newValue));
            return *this;
        }

        template<typename... Args>
        void emplace(Args && ... args)
        {
            set(Type(std::forward<Args>(args)...));
        }

    public:
        template<typename T, typename = std::enable_if_t<!kIsProperty<T>>>
        bool operator==(T const & other) const
        {
            return _value == other;
        }

        bool operator==(Property const & other) const
        {
            return _value == other._value;
        }

        template<typename T, typename = std::enable_if_t<!kIsProperty<T>>>
        auto operator<=>(T const & other) const
        {
            return _value <=> other;
        }

        auto operator<=>(Property const & other) const
        {
            return _value <=> other._value;
        }

    private:
        Container & _container;
        Type        _value;
        bool        _dirty = true;
    };
}
