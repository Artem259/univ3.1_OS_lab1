#ifndef _OS_LAB1_PROBEFUNCS_H
#define _OS_LAB1_PROBEFUNCS_H

#include "compfuncs.hpp"
#include <variant>
#include <iostream>

#include <chrono>
#include <thread>
#include <optional>
#include <condition_variable>
#include <cmath>


# ifdef __GNUG__
# define INLINE_CONST	constexpr
# else
# define INLINE_CONST	inline const
# endif

namespace os::lab1::compfuncs {

    using namespace std::chrono_literals;
    using duration = std::chrono::seconds;
    using std::pair;
    using std::optional;

    template<typename T>
    using case_attribs = pair<duration, optional<T>>;

/*

    //template<typename T> comp_result<T> gen_func(optional<pair<duration, T>> attribs) {
    template<typename T> comp_result<T> gen_func(optional<case_attribs<T>> attribs) {
	if (attribs) {
	    std::this_thread::sleep_for(std::get<duration>(attribs.value()));
	    return std::get<1>(attribs.value()) ? comp_result<T>(std::get<1>(attribs.value()).value()) : comp_result<T>(hard_fail());
	}
	else {
	    std::condition_variable cv;
	    std::mutex m;
	    std::unique_lock<std::mutex> lock(m);
	    cv.wait(lock, []{return false;});
	    return {};
	}
    }

    template<op_group O>
    struct op_group_trial_traits;

    template<typename T> 
    struct op_group_trial_type_traits : op_group_type_traits<T> {
	typedef case_attribs<T> attr_type;
	typedef struct { optional<attr_type> f_attrs, g_attrs; } case_type;
    };

    template<> 
    struct op_group_trial_traits<INT_SUM> : op_group_trial_type_traits<int> {
    	INLINE_CONST static case_type cases[] = {
	    { .f_attrs = pair(3s, 3), .g_attrs = pair(1s, 5) },
	    { .f_attrs =          {}, .g_attrs = attr_type(3s, {}) },
	};
    };

    template<> 
    struct op_group_trial_traits<DOUBLE_MULT> : op_group_trial_type_traits<double> {
    	INLINE_CONST static case_type cases[] = {
	    { .f_attrs = pair(3s, 5.), .g_attrs = pair(1s, 3.) },
	    { .f_attrs = {}, .g_attrs = attr_type(3s, {}) },
	};
    };

    template<> 
    struct op_group_trial_traits<AND> : op_group_trial_type_traits<bool> {
    	INLINE_CONST static case_type cases[] = {
	    { .f_attrs =  pair(1s, false), .g_attrs = pair(3s, true) },
	    { .f_attrs =              {}, .g_attrs = attr_type(3s, {}) },
	};
    };

    template<> 
    struct op_group_trial_traits<OR> : op_group_trial_type_traits<bool> {
    	INLINE_CONST static case_type cases[] = {
	    { .f_attrs = pair(3s, true), .g_attrs = pair(1s, false) },
	    { .f_attrs = attr_type(3s, {}), .g_attrs = {} },
	};
    };

    template<op_group O> 
    typename op_group_traits<O>::result_type trial_f(int case_nr) {
	return gen_func<typename op_group_traits<O>::value_type>(op_group_trial_traits<O>::cases[case_nr].f_attrs);
    }

    template<op_group O> 
    typename op_group_traits<O>::result_type trial_g(int case_nr) {
	return gen_func<typename op_group_traits<O>::value_type>(op_group_trial_traits<O>::cases[case_nr].g_attrs);
    }

*/

    int getDigitOfNumber(int num, int d) {
        int p = static_cast<int>(pow(10,d));
        num = num - (num % p);
        num = num / p;
        num = num % 10;
        return num;
    }

    template<op_group O>
    typename op_group_traits<O>::result_type functionsBehaviour(int v, int t) {
        v = v % 4;
        std::this_thread::sleep_for(std::chrono::seconds(t));
        switch (v) {
            case 0: return t;
            case 1: return soft_fail{};
            case 2: return hard_fail{};
            default: {
                std::condition_variable cv;
                std::mutex m;
                std::unique_lock<std::mutex> lock(m);
                cv.wait(lock, []{return false;});
                return {};
            }
        }
    }

    // v*t*
    template<op_group O>
    typename op_group_traits<O>::result_type trial_f(int x) {
        int v = getDigitOfNumber(x, 3);
        int t = getDigitOfNumber(x, 1);
        return functionsBehaviour<O>(v,t);
    }

    // *v*t
    template<op_group O>
    typename op_group_traits<O>::result_type trial_g(int x) {
        int v = getDigitOfNumber(x, 2);
        int t = getDigitOfNumber(x, 0);
        return functionsBehaviour<O>(v,t);
    }

}

#endif // _OS_LAB1_PROBEFUNCS_H
