//
// Created by kinit on 2021-10-12.
//

#ifndef NCIHOSTD_LPCARGLISTEXTRACTOR_H
#define NCIHOSTD_LPCARGLISTEXTRACTOR_H

#include <tuple>
#include <functional>

#include "ArgList.h"
#include "LpcResult.h"

namespace ipcprotocol {

template<class Subject>
class LpcArgListExtractor {
private:
    template<int ...>
    struct seq {
    };

    template<int N, int ...S>
    struct gens : gens<N - 1, N - 1, S...> {
    };

    template<int ...S>
    struct gens<0, S...> {
        typedef seq<S...> type;
    };

    template<class R, int ...S, class ...Ts>
    static inline R invoke_forward(const std::function<R(Subject *, Ts...)> &func, Subject *that,
                                   seq<S...>, std::tuple<Ts...> &vs) {
        return func(that, std::get<S>(vs) ...);
    }

    template<int N, class ...Args>
    static inline bool extract_arg(const ArgList &args, std::tuple<Args...> &vs) {
        return args.get(&std::get<N>(vs), N);
    }

    template<int ...S, class ...Ts>
    static inline bool extract_arg_list(seq<S...>, const ArgList &args, std::tuple<Ts...> &vs) {
        if ((true && ... && extract_arg<S, Ts...>(args, vs))) {
            return true;
        }
        return false;
    }

public:
    template<class R, class ...Args>
    static inline std::function<TypedLpcResult<R>(Subject * , Args...)> is
            (TypedLpcResult<R> (*pf)(Subject *, Args...)) {
        return std::function<TypedLpcResult<R>(Subject *, Args...)>(pf);
    }

    template<class R>
    static LpcResult inline invoke(Subject *that, [[maybe_unused]] const ArgList &args,
                                   const std::function<TypedLpcResult<R>(Subject * )> &func) {
        return LpcResult(func(that));
    }

    template<class R, class ...Ts>
    static LpcResult invoke(Subject *that, const ArgList &args,
                            const std::function<TypedLpcResult<R>(Subject * , Ts...)> &func) {
        std::tuple<Ts...> vs;
        if (extract_arg_list(typename gens<sizeof...(Ts)>::type(), args, vs)) {
            return LpcResult(invoke_forward(func, that, typename gens<sizeof...(Ts)>::type(), vs));
        } else {
            LpcResult result;
            result.setError(LpcErrorCode::ERR_INVALID_ARGUMENT);
            return result;
        }
    }
};

}

#endif //NCIHOSTD_LPCARGLISTEXTRACTOR_H
