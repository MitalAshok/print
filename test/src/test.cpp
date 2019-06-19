#include <sstream>
#include <utility>

#include "print.h"
#include "gtest/gtest.h"

class PrintTest : public ::testing::Test {
protected:
    ~PrintTest() override = default;

    ::std::string get_string() {
        return sstream.str();
    }
    virtual void reset() {
        ::std::stringstream().swap(sstream);
    }

    static constexpr const char flush_char = '#';
    ::std::string flush_string() {
        return ::std::string(1u, flush_char);
    }

    ::std::stringstream sstream;
public:
    template<class T>
    void operator<<(T&& other) {
        sstream << ::std::forward<T>(other);
    }

    void operator<<(::printer::print_nothing_t) {
        FAIL() << "Should not call operator<< with print_nothing";
    }

    void flush() {
        sstream << flush_char;
    }
};

class PrintStdout : public PrintTest {
protected:
    PrintStdout() : coutbuf(::std::cout.rdbuf()) {
        ::std::cout.rdbuf(sstream.rdbuf());
    }

    ~PrintStdout() override {
        ::std::cout.rdbuf(coutbuf);
    }

    void reset() override {
        PrintTest::reset();
        ::std::cout.rdbuf(sstream.rdbuf());
    }

private:
    ::std::streambuf* const coutbuf;
};

struct flush_twice {
public:
    template<class T>
    void operator()(T&& t) const {
        t.flush();
        t.flush();
    }
};

TEST_F(PrintStdout, empty_test) {
    ::print();
    ASSERT_EQ(get_string(), "\n");
}

TEST_F(PrintTest, other_tests) {
    using ::print;
    using ::file;
    using ::end;
    using ::sep;
    using ::print_nothing;
    using ::flush;
    using ::raw_print;
    using ::print_no_end;

    print(file=*this);
    ASSERT_EQ(get_string(), "\n");

    reset();
    print(end=print_nothing, file=*this);
    ASSERT_EQ(get_string(), "");

    const char hello[] = "Hello, world!\n";

    reset();
    print("Hello, world!", file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print("Hello,", "world!", file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print(end="Hello, world!\n", file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print("Hello, ", "world!", sep=print_nothing, file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print("Hello, ", "world!", sep, file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print("Hello, ", "world!", sep="", file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print(end="world!\n", "Hello, ", sep="no sep between end and last", file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print("Hello,", print_nothing, " ", print_nothing, "world!", sep="print_nothing didn't work", file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print(end="Hello,", file=*this);
    print("", "world!", file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print(flush=true, file=*this);
    ASSERT_EQ(get_string(), '\n' + flush_string());

    reset();
    print(flush, file=*this);
    ASSERT_EQ(get_string(), '\n' + flush_string());

    reset();
    print(flush=false, file=*this);
    ASSERT_EQ(get_string(), "\n");

    reset();
    print<::flush_twice>(flush=true, file=*this);
    ASSERT_EQ(get_string(), '\n' + flush_string() + flush_string());

    reset();
    print<::flush_twice>(flush, file=*this);
    ASSERT_EQ(get_string(), '\n' + flush_string() + flush_string());

    reset();
    raw_print("Hello,", ' ', "world!", '\n', file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print_no_end("Hello,", "world!\n", file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    raw_print("Hello,", "world!", sep=' ', end='\n', file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    print_no_end("Hello,", "world!", end='\n', file=*this);
    ASSERT_EQ(get_string(), hello);

    reset();
    raw_print(file=*this, 'H', 'e', 'l', 'l', 'o', ',', ' ', 'w', 'o', 'r', 'l', 'd', '!', '\n');
    ASSERT_EQ(get_string(), hello);
}

struct void_stream_t {
    template<class T>
    constexpr void operator<<(T&&) const noexcept { /* Do nothing */ }

    constexpr void flush() const noexcept { }
};

struct not_a_stream_t {};

struct dont_flush {
    template<class T>
    constexpr void operator()(T&&) const noexcept { }
};

TEST(PrintTests, noexcept_constexpr_tests) {
    using ::print;
    using ::file;
    using ::end;
    using ::sep;
    using ::print_nothing;
    using ::flush;
    using ::raw_print;
    using ::print_no_end;

    constexpr ::void_stream_t void_stream;
    constexpr ::not_a_stream_t not_a_stream;

#undef CONSTEXPR_TRUE
#define CONSTEXPR_TRUE(x) ASSERT_TRUE((::std::integral_constant<bool, (x)>::value))

    CONSTEXPR_TRUE(noexcept(print(file=void_stream)));
    CONSTEXPR_TRUE(noexcept(print(file=void_stream, 0, 1, 2, "3")));
    CONSTEXPR_TRUE(noexcept(print(file=not_a_stream, end)));
    CONSTEXPR_TRUE(noexcept(print(file=void_stream, flush=true)));
    CONSTEXPR_TRUE(noexcept(print<::dont_flush>(file=not_a_stream, end, flush=true)));

#define LONG_TEST_CASE_16 LONG_TEST_CASE_14, 0, 0
#define LONG_TEST_CASE_61 LONG_TEST_CASE_16, LONG_TEST_CASE_16, LONG_TEST_CASE_16, LONG_TEST_CASE_14
#define LONG_TEST_CASE_64 LONG_TEST_CASE_16, LONG_TEST_CASE_16, LONG_TEST_CASE_16, LONG_TEST_CASE_16
// 253 because typical implementations will have at least 256 arguments as the maximum number of arguments,
// and we need to pass `file=` and the implementation of `print` prepends two arguments (Need 3 extra)
#define LONG_TEST_CASE_253 LONG_TEST_CASE_64, LONG_TEST_CASE_64, LONG_TEST_CASE_64, LONG_TEST_CASE_61

#if !defined(__GNUC__) || defined(__clang__)
    // These will actually just not compile if it's not a constant expression
    CONSTEXPR_TRUE((::std::integral_constant<bool, (static_cast<void>(print(file=void_stream)), true)>::value));
    CONSTEXPR_TRUE((::std::integral_constant<bool, (static_cast<void>(print(file=void_stream, 0, 1, 2, "3")), true)>::value));
    CONSTEXPR_TRUE((::std::integral_constant<bool, (static_cast<void>(print(file=not_a_stream, end)), true)>::value));
    CONSTEXPR_TRUE((::std::integral_constant<bool, (static_cast<void>(print(file=void_stream, flush=true)), true)>::value));
    CONSTEXPR_TRUE((::std::integral_constant<bool, (static_cast<void>(print<::dont_flush>(file=not_a_stream, end, flush=true)), true)>::value));
#define LONG_TEST_CASE_14 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    CONSTEXPR_TRUE((::std::integral_constant<bool, (static_cast<void>(print(file=void_stream, LONG_TEST_CASE_253)), true)>::value));
#undef LONG_TEST_CASE_14
#endif

// Different types so templates instantiated are different
#define LONG_TEST_CASE_14 nullptr, nullptr, static_cast<void*>(0), "", 0.0, 0L, 0U, 0UL, 0U, 0U, ' ', 0U, 0U, 0U, 0U
    CONSTEXPR_TRUE(noexcept(print(LONG_TEST_CASE_253, file=void_stream)));
}
