#include "stdafx.h"
#include "CppUnitTest.h"

#include "util/cache.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace test_suite
{
	TEST_CLASS(CacheTest) {
	public:
        struct test_t {
            test_t(int i, float f) : i(i), f(f) { }

            int i;
            float f;
        };

        typedef bklib::cache_t<test_t> cache_t;
        typedef cache_t::handle_t      handle_t;

        cache_t cache;

        template <typename F>
        void test_1_helper(F&& func) {
            static auto const  e = std::numeric_limits<float>::epsilon();
            static int const   i = 1;
            static float const f = 2.0f;
            ////////////////////////////////////////////////////////////////////
            Assert::IsTrue(cache.size() == 0);
            Assert::IsTrue(cache.free_slots() == 0);

            ////////////////////////////////////////////////////////////////////
            auto const handle = func(i, f);

            Assert::IsTrue(cache.is_valid(handle));

            Assert::IsTrue(cache.size() == 1);
            Assert::IsTrue(cache.free_slots() == 0);
            ////////////////////////////////////////////////////////////////////
            auto& test = cache.get(handle);

            Assert::AreEqual(i, test.i);
            Assert::AreEqual(f, test.f, e);
        }

        //Test constructing one item in place
        TEST_METHOD(TestConstruct_1) {
            test_1_helper([&](int i, float f) {
                return cache.construct(i, f);
            });
        }

        //Test adding one item
        TEST_METHOD(TestAdd_1) {
            test_1_helper([&](int i, float f) {
                return cache.add(std::make_unique<test_t>(i, f));
            });
        }

        template <unsigned N, typename F>
        void test_n_helper(F&& func) {
            static auto const  e = std::numeric_limits<float>::epsilon();
            static float const f = 0.1f;

            ////////////////////////////////////////////////////////////////////
            std::array<handle_t, N> handles;

            Assert::IsTrue(cache.size() == 0);
            Assert::IsTrue(cache.free_slots() == 0);

            ////////////////////////////////////////////////////////////////////
            std::generate(std::begin(handles), std::end(handles), [&]() -> handle_t {
                static int i = 0;

                auto result = func(i, i + f);
                i++;
                return result;
            });

            Assert::IsTrue(cache.size() == N);
            Assert::IsTrue(cache.free_slots() == 0);

            ////////////////////////////////////////////////////////////////////
            std::for_each(std::begin(handles), std::end(handles), [&](handle_t h) {
                static int i = 0;

                Assert::IsTrue(cache.is_valid(h));
                auto const& x = cache.get(h);

                Assert::AreEqual(x.i, i);
                Assert::AreEqual(x.f, i + f, e);

                i++;
            });
        }

        //Test constructing N items in place
        TEST_METHOD(TestConstruct_N) {
            test_n_helper<10>([&](int i, float f) {
                return cache.construct(i, f);
            });
        }

        //Test adding N items
        TEST_METHOD(TestAdd_N) {
            test_n_helper<10>([&](int i, float f) {
                return cache.add(std::make_unique<test_t>(i, f));
            });
        }

        TEST_METHOD(TestRemove) {
            static auto const  e = std::numeric_limits<float>::epsilon();
            static int const   i = 1;
            static float const f = 2.0f;

            ////////////////////////////////////////////////////////////////////
            Assert::IsTrue(cache.size() == 0);
            Assert::IsTrue(cache.free_slots() == 0);

            auto h0 = cache.construct(i, f);
            Assert::IsTrue(cache.is_valid(h0));

            Assert::IsTrue(cache.size() == 1);
            Assert::IsTrue(cache.free_slots() == 0);
            ////////////////////////////////////////////////////////////////////
            auto v0 = cache.remove(h0);
            Assert::IsFalse(cache.is_valid(h0));

            Assert::AreEqual(v0->i, i);
            Assert::AreEqual(v0->f, f, e);

            Assert::IsTrue(cache.size() == 1);
            Assert::IsTrue(cache.free_slots() == 1);
            ////////////////////////////////////////////////////////////////////
            Assert::ExpectException<bklib::cache_exception>([&] {
                cache.remove(h0);
            });

            Assert::ExpectException<bklib::cache_exception>([&] {
                cache.get(h0);
            });
        }

		/*TEST_METHOD(TestWindowHit)
		{
            static const int X = 100;
            static const int Y = 100;
            static const int W = 200;
            static const int H = 200;

            bklib::gui::Window win(X, Y, W, H);

            Assert::IsFalse(win.hit_test(0, 0));
            Assert::IsFalse(win.hit_test(-1, -1));

            Assert::IsFalse(win.hit_test(X + W + 1, Y + H + 1));
            Assert::IsFalse(win.hit_test(X + W, Y + H));
            //Assert::IsTrue(win.hit_test(X + W - 1, Y + H - 1));

            Assert::IsFalse(win.hit_test(X - 1, Y - 1));
            //Assert::IsTrue(win.hit_test(X, Y));
		}

		TEST_METHOD(TestWindowChildren)
		{
            static const int X = 100;
            static const int Y = 100;
            static const int W = 200;
            static const int H = 200;

            bklib::gui::Window win(X, Y, W, H);

            auto const w1 = win.add_child(
                bklib::make_unique<bklib::gui::Window>(10, 10, 10, 10)
            );

            Assert::IsTrue(w1.is_valid());
            Assert::IsTrue(w1.index == 0);
            Assert::IsTrue(w1.count == 0);
            Assert::IsTrue(win.child_count() == 1);

            auto const w2 = win.add_child(
                bklib::make_unique<bklib::gui::Window>(20, 20, 10, 10)
            );

            Assert::IsTrue(w2.is_valid());
            Assert::IsTrue(w2.index == 1);
            Assert::IsTrue(w2.count == 0);
            Assert::IsTrue(win.child_count() == 2);
		}*/

	};
}