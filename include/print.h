/**
 * print.h
 *
 * Inspired by Python 3s `print` function, this has very similar syntax.
 *
 * Usage is very simple. Just #include this file, and use the global free function `print`:
 *
 *     print("Hello, world!");  // Prints "Hello, world!\n"
 *
 *     int a = 1;
 *     int b = 4;
 *     print(a, '+', b, "==", a + b);  // Prints "1 + 4 = 5\n"
 *     print(a, b, sep="; ");  // Prints "1; 4\n"
 *
 *     std::stringstream ss;
 *     print(a, file=ss, end="; ");  // `ss.str() == "1; "`
 *     print(b, file=ss);  // `ss.str() == "1; 4\n"`
 *     print(ss.str(), file=std::cerr, flush=true);  // Prints "1; 4\n" to std::cerr and then flushes std::cerr
 *     print(ss.str(), file=std::cerr, flush);  // Same as above
 *
 *     print("a", "", "b", end="", sep="+");  // Prints "a++b"
 *     print("a", print_nothing, "b", end="", sep="+");  // Prints "ab"
 *     print("a", print_nothing, "b", end=print_nothing, sep="+");  // Prints "ab" (But doesn't call `std::cout << ""` for end
 *     print("a", std::boolalpha, true);  // Prints "a  true\n" (Double space)
 *     print("a", print_nothing, std::boolalpha, true);  // Prints "a true\n"
 *     print("a", "b", "c", end=print_nothing, sep=print_nothing);  // Prints "abc"
 *     print("a", "b", "c", end, sep);  // Prints "abc"  (`end` is short for `end=print_nothing`, and the same for `sep`)
 *
 * There is also `raw_print` and `print_no_end`, which are the same as `print`, except `raw_print` has
 * `sep` and `end` set to `print_nothing` by default, and `print_no_end` has `end` set to `print_nothing` by default.
 *
 *     // These two are the same
 *     print(x, y, z, sep, end);
 *     raw_print(x, y, z);
 *
 *     // These two are also the same
 *     print(x, y, z, end);
 *     print_no_end(x, y, z);
 *
 * To avoid bringing these two functions into the global scope, define `PRINTER_NO_RAW` before including this file.
 *
 * `print()`, outputs (through `operator<<`) all the passed arguments to `file`, and outputs `sep` in between
 * each argument. Then, it outputs `end`, and, if `flush` is true, flushes the stream (through std::flush if possible).
 * If any argument encountered is `print_nothing`, the separator is not printed. When this is set as `end` or `sep`,
 * the file's output operator is not called for the separator or end (As opposed to `sep=""` which would call
 * `file << ""` around as many times as there are arguments).
 *
 * `file` defaults to `std::cout`. `sep` defaults to `' '` (space character). `end` defaults to `'\n'` (newline
 * character). `flush` defaults to `false`.
 *
 * To set these 4 arguments, there are 4 static variables, called `file`, `sep`, `end` and `flush`.
 * Their `operator=` will return an object which will set the corresponding argument to the value it was set to.
 *
 *
 * If you do not want these static variables in the global scope, define `PRINTER_NO_GLOBALS` before including
 * this file. These variables are of type `printer::file_t`, `printer::sep_t`, `printer::end_t` and `printer::flush_t`.
 * You can define variables of these types somewhere else, use the static variables in the `printer` namespace (
 * `printer::file`, `printer::sep`, `printer::end` and `printer::flush`) or use rvalues of these types:
 *
 *      constexpr printer::sep_t my_sep;
 *      // All three of these are equivalent
 *      print(a, b, my_sep="; ");
 *      printer::print(a, b, printer::sep="; ");
 *      printer::print(a, b, printer::sep_t{}="; ");
 *
 *      // And equivalently `printer::print_nothing_t` for `print_nothing`
 *      // All of these do nothing
 *      constexpr printer::print_nothing_t my_nothing;
 *      print(end=my_nothing);
 *      printer::print(end=printer::print_nothing);
 *      printer::print(end=printer::print_nothing_t{});
 *
 * Note that arguments are perfect forwarded, and no copies of arguments are done (Unless `operator<<(file, value)`
 * copies something). `noexcept` is also calculated properly, and (not in gcc, but in clang and msvc) `print();`
 * will be `constexpr` if possible.
 *
 * By default, `flush` calls `file.flush();` but only if a `flush=(value)` argument is passed at all (So it is safe
 * to pass something that can't be flushed if you don't pass a `flush=(value)` argument, but you can't pass `flush=false`).
 * To customise this behaviour, pass a class as a template parameter that has an `operator()(MyStreamType) const`:
 *
 *      struct FlushMyStream {
 *          void operator()(MyStreamType& s) const {
 *              // e.g. your stream type has a `flush` member function instead of a free function
 *              s.flush();
 *          }
 *      };
 *
 *      MyStreamType f;
 *      print<FlushMyStream>(1, 2, 3, file=f, flush=true);
 */


#ifndef PRINTER_H_
#define PRINTER_H_

#include <iostream>
#include <utility>
#include <type_traits>

// gcc cannot handle the number of `constexpr` functions called
#if defined(__GNUC__) && !defined(__clang__)
#define PRINTER_CONSTEXPR
#else
#define PRINTER_CONSTEXPR constexpr
#endif

namespace printer {
    namespace detail {
        template<class T, class U>
        struct is_fwd_same : ::std::integral_constant<bool, ::std::is_same<
            typename ::std::remove_cv<typename ::std::remove_reference<T>::type>::type,
            typename ::std::remove_cv<typename ::std::remove_reference<U>::type>::type
        >::value> {};
    }

    struct sep_t {
        template<class T>
        struct value_t {
            T&& value;
        };

        template<class T>
        constexpr value_t<T> operator=(T&& value) const noexcept {
            return value_t<T>{ ::std::forward<T>(value) };
        }
    };

    struct end_t {
        template<class T>
        struct value_t {
            T&& value;
        };

        template<class T>
        constexpr value_t<T> operator=(T&& value) const noexcept {
            return value_t<T>{ ::std::forward<T>(value) };
        }
    };

    struct file_t {
        template<class T>
        struct value_t {
            T&& value;
        };

        template<class T>
        constexpr value_t<T> operator=(T&& value) const noexcept {
            return value_t<T>{ ::std::forward<T>(value) };
        }
    };

    struct flush_t {
        struct value_t {
            bool value;
        };

        constexpr value_t operator=(bool value) const noexcept {
            return value_t{ value };
        }

        template<class T>
        constexpr value_t operator=(T&& value) const noexcept {
            return value_t{ static_cast<bool>(::std::forward<T>(value)) };
        }
    };

    struct print_nothing_t {
        template<class T>
        friend constexpr T&& operator<<(T&& os, print_nothing_t) noexcept {
            return ::std::forward<T>(os);
        }

        template<class T, class U>
        friend constexpr
        typename ::std::enable_if<
            detail::is_fwd_same<T, print_nothing_t>::value ||
            detail::is_fwd_same<U, print_nothing_t>::value,
            ::std::integral_constant<bool,
                detail::is_fwd_same<T, print_nothing_t>::value == detail::is_fwd_same<U, print_nothing_t>::value
            >
        >::type operator==(const T&, const U&) noexcept {
            return ::std::integral_constant<bool,
                detail::is_fwd_same<T, print_nothing_t>::value == detail::is_fwd_same<U, print_nothing_t>::value
            >{};
        }

        template<class T, class U>
        friend constexpr
        typename ::std::enable_if<
            detail::is_fwd_same<T, print_nothing_t>::value ||
            detail::is_fwd_same<U, print_nothing_t>::value,
            ::std::integral_constant<bool,
                detail::is_fwd_same<T, print_nothing_t>::value != detail::is_fwd_same<U, print_nothing_t>::value
            >
        >::type operator!=(const T&, const U&) noexcept {
            return ::std::integral_constant<bool,
                detail::is_fwd_same<T, print_nothing_t>::value != detail::is_fwd_same<U, print_nothing_t>::value
            >{};
        }
    };

    struct print_flusher {
        template<class T>
        constexpr void operator()(T&& os) const noexcept(noexcept(::std::forward<T>(os).flush())) {
            ::std::forward<T>(os).flush();
        }
    };

#ifdef PRINTER_TRY_COMBINE_STATICS
    namespace detail {
        struct static_variables_t : sep_t, end_t, file_t, flush_t, print_nothing_t { };
        static constexpr const static_variables_t static_variables;
    }

    static constexpr const sep_t& sep = detail::static_variables;
    static constexpr const end_t& end = detail::static_variables;
    static constexpr const file_t& file = detail::static_variables;
    static constexpr const flush_t& flush = detail::static_variables;

    static constexpr const print_nothing_t& print_nothing = detail::static_variables;
#else
    static constexpr const sep_t sep;
    static constexpr const end_t end;
    static constexpr const file_t file;
    static constexpr const flush_t flush;

    static constexpr const print_nothing_t print_nothing;
#endif
}


namespace printer { namespace detail {
    enum class print_manipulated {
        none = 0,
        sep = 1,
        end = 2,
        file = 4,
        flush = 8
    };

    constexpr bool operator&(print_manipulated lhs, print_manipulated rhs) noexcept {
        using underlying = typename ::std::underlying_type<print_manipulated>::type;
        return static_cast<bool>(static_cast<underlying>(lhs) & static_cast<underlying>(rhs));
    }
    constexpr print_manipulated operator|(print_manipulated lhs, print_manipulated rhs) noexcept {
        using underlying = typename ::std::underlying_type<print_manipulated>::type;
        return static_cast<print_manipulated>(static_cast<underlying>(lhs) | static_cast<underlying>(rhs));
    }

    template<class T>
    struct dependant_false : ::std::false_type { };

    template<class SepT, class EndT, class FileT, print_manipulated Manipulated = print_manipulated::none>
    struct print_options {
        SepT&& sep;
        EndT&& end;
        FileT&& file;
        const bool flush;


        constexpr print_options(SepT&& sep_, EndT&& end_, FileT&& file_, const bool flush_) noexcept :
            sep(::std::forward<SepT>(sep_)), end(::std::forward<EndT>(end_)), file(::std::forward<FileT>(file_)), flush(flush_) {}

        constexpr print_options(const print_options& other) noexcept : sep(::std::forward<SepT>(other.sep)), end(::std::forward<EndT>(other.end)), file(::std::forward<FileT>(other.file)), flush(other.flush) {}
        // constexpr print_options& operator=(const print_options&) noexcept = default;

        static constexpr const bool set_sep = Manipulated & print_manipulated::sep;
        static constexpr const bool set_end = Manipulated & print_manipulated::end;
        static constexpr const bool set_file = Manipulated & print_manipulated::file;
        static constexpr const bool set_flush = Manipulated & print_manipulated::flush;

        template<class T>
        constexpr print_options<T, EndT, FileT, Manipulated | print_manipulated::sep> operator+(const sep_t::value_t<T>& new_sep) const noexcept {
            static_assert(dependant_false<T>::value || !set_sep, "`sep` keyword argument passed multiple times to print().");
            return { ::std::forward<T>(new_sep.value), ::std::forward<EndT>(end), ::std::forward<FileT>(file), flush };
        }
        template<class T>
        constexpr print_options<SepT, T, FileT, Manipulated | print_manipulated::end> operator+(const end_t::value_t<T>& new_end) const noexcept {
            static_assert(dependant_false<T>::value || !set_end, "`end` keyword argument passed multiple times to print().");
            return { ::std::forward<SepT>(sep), ::std::forward<T>(new_end.value), ::std::forward<FileT>(file), flush };
        }
        template<class T>
        constexpr print_options<SepT, EndT, T, Manipulated | print_manipulated::file> operator+(const file_t::value_t<T>& new_file) const noexcept {
            static_assert(dependant_false<T>::value || !set_file, "`file` keyword argument passed multiple times to print().");
            return { ::std::forward<SepT>(sep), ::std::forward<EndT>(end), ::std::forward<T>(new_file.value), flush };
        }
        template<class T = void>
        constexpr print_options<SepT, EndT, FileT, Manipulated | print_manipulated::flush> operator+(const flush_t::value_t& new_flush) const noexcept {
            static_assert(dependant_false<T>::value || !set_flush, "`flush` keyword argument passed multiple times to print().");
            return { ::std::forward<SepT>(sep), ::std::forward<EndT>(end), ::std::forward<FileT>(file), new_flush.value };
        }
        template<class T = print_nothing_t>
        constexpr print_options<T, EndT, FileT, Manipulated | print_manipulated::sep> operator+(const sep_t&) const noexcept {
            // Just `sep` is an alias for `sep=print_nothing_t`
            return *this + (sep_t()=T());
        }
        template<class T = print_nothing_t>
        constexpr print_options<SepT, T, FileT, Manipulated | print_manipulated::end> operator+(const end_t&) const noexcept {
            // Just `end` is an alias for `end=print_nothing_t`
            return *this + (end_t()=T());
        }
        template<class T = flush_t>
        constexpr print_options<SepT, EndT, FileT, Manipulated | print_manipulated::flush> operator+(const flush_t&) const noexcept {
            // Just `flush` is an alias for `flush=true`
            static_assert(dependant_false<T>::value || !set_flush, "`flush` keyword argument passed multiple times to print().");
            return { ::std::forward<SepT>(sep), ::std::forward<EndT>(end), ::std::forward<FileT>(file), true };
        }

        template<class T> constexpr const print_options& operator+(const T&) const noexcept { return *this; }
    };

    template<class T> struct is_print_opt_value : ::std::false_type {};
    template<class T> struct is_print_opt_value<sep_t::value_t<T>> : ::std::true_type {};
    template<class T> struct is_print_opt_value<end_t::value_t<T>> : ::std::true_type {};
    template<class T> struct is_print_opt_value<file_t::value_t<T>> : ::std::true_type {};
    template<> struct is_print_opt_value<flush_t::value_t> : ::std::true_type {};
    template<> struct is_print_opt_value<sep_t> : ::std::true_type {};
    template<> struct is_print_opt_value<end_t> : ::std::true_type {};
    template<> struct is_print_opt_value<flush_t> : ::std::true_type {};

    template<class T>
    struct is_fwd_print_opt_value : ::std::integral_constant<bool, is_print_opt_value<typename ::std::remove_cv<typename ::std::remove_reference<T>::type>::type>::value> {};

    template<class SepT, class EndT, class FileT, print_manipulated Manipulated>
    constexpr print_options<SepT, EndT, FileT, Manipulated> combine_options(const print_options<SepT, EndT, FileT, Manipulated>& opts) noexcept {
        return opts;
    }

    template<class SepT, class EndT, class FileT, print_manipulated Manipulated, class T, class... U>
    PRINTER_CONSTEXPR auto combine_options(const print_options<SepT, EndT, FileT, Manipulated>& opts, T&& t, U&&... u) noexcept -> decltype(combine_options(opts + t, u...)) {
        return combine_options(opts + t, u...);
    }

    constexpr bool fold_or(bool b) noexcept {
        return b;
    }

    template<class... Bool>
    constexpr bool fold_or(bool a, bool b, Bool... c) noexcept {
        return fold_or(a || b, c...);
    }

    template<class... Args>
    struct print_can_possibly_flush : ::std::integral_constant<bool, fold_or(
        false, (is_fwd_same<Args, flush_t::value_t>::value || is_fwd_same<Args, flush_t>::value)...
    )> { };

    template<class... Args>
    struct print_will_always_flush : ::std::integral_constant<bool, fold_or(
        false, (is_fwd_same<Args, flush_t>::value)...
    )> { };

#if __cplusplus >= 201402L
    template<bool AlwaysFlush, bool CanFlush, class Flusher, class FileT>
    PRINTER_CONSTEXPR
    typename ::std::enable_if<CanFlush && AlwaysFlush>::type print_flush(bool flush, FileT&& f) noexcept(noexcept(Flusher{}(::std::forward<FileT>(f)))) {
        Flusher{}(::std::forward<FileT>(f));
    }

    template<bool AlwaysFlush, bool CanFlush, class Flusher, class FileT>
    PRINTER_CONSTEXPR
    typename ::std::enable_if<CanFlush && !AlwaysFlush>::type print_flush(bool flush, FileT&& f) noexcept(noexcept(Flusher{}(::std::forward<FileT>(f)))) {
        return flush ? static_cast<void>(Flusher{}(::std::forward<FileT>(f))) : static_cast<void>(0);
    }

    template<bool AlwaysFlush, bool CanFlush, class Flusher, class FileT>
    constexpr typename ::std::enable_if<!CanFlush && !AlwaysFlush>::type print_flush(bool, FileT&&) noexcept {}
#else
    template<bool AlwaysFlush, bool CanFlush, class Flusher, class FileT>
    PRINTER_CONSTEXPR
    typename ::std::enable_if<CanFlush && AlwaysFlush, int>::type print_flush(bool flush, FileT&& f) noexcept(noexcept(Flusher{}(::std::forward<FileT>(f)))) {
        return static_cast<void>(Flusher{}(::std::forward<FileT>(f))), 0;
    }

    template<bool AlwaysFlush, bool CanFlush, class Flusher, class FileT>
    PRINTER_CONSTEXPR
    typename ::std::enable_if<CanFlush && !AlwaysFlush, int>::type print_flush(bool flush, FileT&& f) noexcept(noexcept(Flusher{}(::std::forward<FileT>(f)))) {
        return flush ? (static_cast<void>(Flusher{}(::std::forward<FileT>(f))), 0) : 0;
    }

    template<bool AlwaysFlush, bool CanFlush, class Flusher, class FileT>
    constexpr typename ::std::enable_if<!CanFlush && !AlwaysFlush, int>::type print_flush(bool, FileT&&) noexcept { return 0; }
#endif

#if __cplusplus >= 201402L
    using constexpr_return_type = void;
#else
    using constexpr_return_type = int;
#endif

    template<bool, class PrintOptionsT>
    constexpr constexpr_return_type print_impl(const PrintOptionsT&) noexcept { return static_cast<constexpr_return_type>(0); }

    template<bool PrintSep, class PrintOptionsT, class Arg, class... Args>
    constexpr typename ::std::enable_if<is_fwd_print_opt_value<Arg>::value, constexpr_return_type>::type
    print_impl(const PrintOptionsT& opts, Arg&&, Args&&... args) noexcept(noexcept(print_impl<PrintSep>(opts, ::std::forward<Args>(args)...))) {
        // arg is a print option; ignore
        return static_cast<void>(print_impl<PrintSep>(opts, ::std::forward<Args>(args)...)), static_cast<constexpr_return_type>(0);
    }

    template<bool PrintSep, class PrintOptionsT, class Arg, class... Args>
    constexpr
    typename ::std::enable_if<is_fwd_same<Arg, print_nothing_t>::value, constexpr_return_type>::type
    print_impl(const PrintOptionsT& opts, Arg&&, Args&&... args) noexcept(noexcept(print_impl<false>(opts, ::std::forward<Args>(args)...))) {
        // arg is a print_nothing_t; Now don't print sep even if we were going to
        return static_cast<void>(print_impl<false>(opts, ::std::forward<Args>(args)...)), static_cast<constexpr_return_type>(0);
    }

    template<bool PrintSep, class PrintOptionsT, class Arg, class... Args>
    constexpr
    typename ::std::enable_if<!PrintSep && !is_fwd_print_opt_value<Arg>::value && !is_fwd_same<Arg, print_nothing_t>::value, constexpr_return_type>::type
    print_impl(const PrintOptionsT& opts, Arg&& arg, Args&&... args)
    noexcept(noexcept(opts.file << ::std::forward<Arg>(arg)) && noexcept(print_impl<true>(opts, ::std::forward<Args>(args)...)))
    {
        // Print just the value, but print attempt to print the separator next (PrintSep is now `true`)
        return static_cast<void>(opts.file << ::std::forward<Arg>(arg)), static_cast<void>(print_impl<true>(opts, ::std::forward<Args>(args)...)), static_cast<constexpr_return_type>(0);
    }

    template<bool PrintSep, class PrintOptionsT, class Arg, class... Args>
    constexpr
    typename ::std::enable_if<PrintSep && !is_fwd_print_opt_value<Arg>::value && !is_fwd_same<Arg, print_nothing_t>::value && !is_fwd_same<decltype(::std::declval<PrintOptionsT>().sep), print_nothing_t>::value, constexpr_return_type>::type
    print_impl(const PrintOptionsT& opts, Arg&& arg, Args&&... args)
    noexcept(noexcept(opts.file << opts.sep) && noexcept(opts.file << ::std::forward<Arg>(arg)) && noexcept(print_impl<true>(opts, ::std::forward<Args>(args)...)))
    {
        // print the separator before printing the value
        return (
            static_cast<void>(opts.file << opts.sep),
            static_cast<void>(opts.file << ::std::forward<Arg>(arg)),
            static_cast<void>(print_impl<true>(opts, ::std::forward<Args>(args)...)),
            static_cast<constexpr_return_type>(0)
        );
    }

    template<bool PrintSep, class PrintOptionsT, class Arg, class... Args>
    constexpr
    typename ::std::enable_if<PrintSep && !is_fwd_print_opt_value<Arg>::value && !is_fwd_same<Arg, print_nothing_t>::value && is_fwd_same<decltype(::std::declval<PrintOptionsT>().sep), print_nothing_t>::value, constexpr_return_type>::type
    print_impl(const PrintOptionsT& opts, Arg&& arg, Args&&... args)
    noexcept(noexcept(opts.file << opts.sep) && noexcept(opts.file << ::std::forward<Arg>(arg)) && noexcept(print_impl<true>(opts, ::std::forward<Args>(args)...)))
    {
        // Same as above, but sep is `print_nothing`, so don't print it.
        return (
            static_cast<void>(opts.file << ::std::forward<Arg>(arg)),
            static_cast<void>(print_impl<true>(opts, ::std::forward<Args>(args)...)),
            static_cast<constexpr_return_type>(0)
        );
    }

    template<class File, class End>
    PRINTER_CONSTEXPR typename ::std::enable_if<!is_fwd_same<End, print_nothing_t>::value, constexpr_return_type>::type
    print_end_impl(File&& f, End&& end) noexcept(noexcept(f << ::std::forward<End>(end))) {
        return static_cast<void>(f << ::std::forward<End>(end)), static_cast<constexpr_return_type>(0);
    }

    template<class File, class End>
    constexpr typename ::std::enable_if<is_fwd_same<End, print_nothing_t>::value, constexpr_return_type>::type
    print_end_impl(File&&, End&&) noexcept {
        return static_cast<constexpr_return_type>(0);
    }

    template<class Flusher, class Opts, class... Args>
    constexpr constexpr_return_type print_impl_2(const Opts& opts, Args&&... args) noexcept(
        noexcept(print_impl<false>(opts, ::std::forward<Args>(args)...)) &&
        noexcept(print_end_impl(opts.file, ::std::forward<decltype(opts.end)>(opts.end))) &&
        noexcept(print_flush<print_will_always_flush<Args...>::value, print_can_possibly_flush<Args...>::value, Flusher>(opts.flush, opts.file))
    ) {
        return (
            static_cast<void>(print_impl<false>(opts, ::std::forward<Args>(args)...)),
            static_cast<void>(print_end_impl(opts.file, ::std::forward<decltype(opts.end)>(opts.end))),
            static_cast<void>(print_flush<print_will_always_flush<Args...>::value, print_can_possibly_flush<Args...>::value, Flusher>(opts.flush, opts.file)),
            static_cast<constexpr_return_type>(0)
        );
    }

    template<class Flusher, class SepT, class EndT, class... Args>
    constexpr constexpr_return_type print_impl_3(const SepT& default_sep, const EndT& default_end, Args&&... args) noexcept(
        noexcept(print_impl_2<Flusher>(
            combine_options(print_options<const SepT&, const EndT&, ::std::ostream&>(default_sep, default_end, ::std::cout, false), ::std::forward<Args>(args)...),
            ::std::forward<Args>(args)...
        ))
    ) {
        return static_cast<void>(print_impl_2<Flusher>(
            combine_options(print_options<const SepT&, const EndT&, ::std::ostream&>(default_sep, default_end, ::std::cout, false), ::std::forward<Args>(args)...),
            ::std::forward<Args>(args)...
        )), static_cast<constexpr_return_type>(0);
    }
} }

namespace printer {
    template<class Flusher = printer::print_flusher, class... Args>
    constexpr detail::constexpr_return_type print(Args&& ... args) noexcept(noexcept(detail::print_impl_3<Flusher, char, char>(' ', '\n', ::std::forward<Args>(args)...))) {
        return static_cast<void>(detail::print_impl_3<Flusher, char, char>(' ', '\n', ::std::forward<Args>(args)...)), static_cast<detail::constexpr_return_type>(0);
    }

    template<class Flusher = printer::print_flusher, class... Args>
    constexpr detail::constexpr_return_type raw_print(Args&& ... args) noexcept(noexcept(detail::print_impl_3<Flusher, print_nothing_t, print_nothing_t>(print_nothing_t(), print_nothing_t(), ::std::forward<Args>(args)...))) {
        return static_cast<void>(detail::print_impl_3<Flusher, print_nothing_t, print_nothing_t>(print_nothing_t(), print_nothing_t(), ::std::forward<Args>(args)...)), static_cast<detail::constexpr_return_type>(0);
    }

    template<class Flusher = printer::print_flusher, class... Args>
    constexpr detail::constexpr_return_type print_no_end(Args&& ... args) noexcept(noexcept(detail::print_impl_3<Flusher, char, print_nothing_t>(' ', print_nothing_t(), ::std::forward<Args>(args)...))) {
        return static_cast<void>(detail::print_impl_3<Flusher, char, print_nothing_t>(' ', print_nothing_t(), ::std::forward<Args>(args)...)), static_cast<detail::constexpr_return_type>(0);
    }
}

#endif

// Outside of main header guard so multiple includes
// where the first defined `PRINTER_NO_GLOBALS`
// and the second didn't mean that it will have globals.
// Different header guards so just comments are included
// if included multiple times.

#ifndef PRINTER_NO_GLOBALS
#ifndef PRINTER_H_GLOBALS_
#define PRINTER_H_GLOBALS_
using printer::sep;
using printer::end;
using printer::file;
using printer::flush;
using printer::print_nothing;
using printer::print;
#endif
#else
#undef PRINTER_NO_GLOBALS
#endif

#ifndef PRINTER_NO_RAW
#ifndef PRINTER_H_RAW_
#define PRINTER_H_RAW_
using printer::raw_print;
using printer::print_no_end;
#endif
#else
#undef PRINTER_NO_RAW
#endif
