#include <gtest/gtest.h>

#include <genoaf/property.hpp>

namespace genoaf::test
{
    namespace
    {
        class DummyContainer
        {
        public:
            template<typename Type,
                     bool kSkipEqTest>
            void markDirty(Property<Type, DummyContainer, kSkipEqTest> const &)
            {
                ++_counter;
            }

            size_t counter() const { return _counter; }

        private:
            size_t _counter = 0;
        };
    }

    TEST(Property, SmokeTest)
    {
        DummyContainer container;
        Property<int, DummyContainer> property(container);

        EXPECT_TRUE(property.dirty());
        EXPECT_EQ(property.get(), 0);

        EXPECT_EQ(container.counter(), 0); // not signaling initially
    }

    TEST(Property, UpdateCycle)
    {
        DummyContainer container;
        Property<int, DummyContainer> property(container);

        ASSERT_TRUE(property.dirty());
        ASSERT_EQ(property.get(), 0);
        ASSERT_EQ(container.counter(), 0);

        property.revalidate();

        ASSERT_FALSE(property.dirty());
        ASSERT_EQ(property.get(), 0);
        ASSERT_EQ(container.counter(), 0);

        property = 42;

        ASSERT_TRUE(property.dirty());
        ASSERT_EQ(property.get(), 42);
        ASSERT_EQ(container.counter(), 1);

        property.revalidate();

        EXPECT_FALSE(property.dirty());
        EXPECT_EQ(property.get(), 42);
        ASSERT_EQ(container.counter(), 1);
    }

    TEST(Property, Comparison)
    {
        DummyContainer container;
        Property<int, DummyContainer> p1(container, 10);
        Property<int, DummyContainer> p2(container, 20);

        EXPECT_TRUE(p1 == 10);
        EXPECT_TRUE(p1 != 20);
        EXPECT_TRUE(p1 < 20);
        EXPECT_TRUE(p1 <= 10);
        EXPECT_TRUE(p2 > 10);
        EXPECT_TRUE(p2 >= 20);

        EXPECT_TRUE(p1 == p1);
        EXPECT_TRUE(p1 != p2);
        EXPECT_TRUE(p1 < p2);
        EXPECT_TRUE(p1 <= p2);
        EXPECT_TRUE(p2 > p1);
        EXPECT_TRUE(p2 >= p1);
    }
}
