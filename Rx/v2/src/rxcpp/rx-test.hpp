#pragma once

#if !defined(RXCPP_RX_TEST_HPP)
#define RXCPP_RX_TEST_HPP

#include "rx-includes.hpp"

namespace rxcpp {

namespace test {

namespace detail {

template<class T>
struct test_subject_base
    : public std::enable_shared_from_this<test_subject_base<T>>
{
    typedef rxn::recorded<typename rxn::notification<T>::type> recorded_type;
    typedef std::shared_ptr<test_subject_base<T>> type;

    virtual void on_subscribe(subscriber<T>) const =0;
    virtual std::vector<recorded_type> messages() const =0;
    virtual std::vector<rxn::subscription> subscriptions() const =0;
};

template<class T>
struct test_source
    : public rxs::source_base<T>
{
    explicit test_source(typename test_subject_base<T>::type ts)
        : ts(std::move(ts))
    {
        if (!this->ts) abort();
    }
    typename test_subject_base<T>::type ts;
    void on_subscribe(subscriber<T> o) const {
        ts->on_subscribe(std::move(o));
    }
    template<class Subscriber>
    typename std::enable_if<!std::is_same<typename std::decay<Subscriber>::type, subscriber<T>>::value, void>::type
    on_subscribe(Subscriber&& o) const {

        static_assert(is_subscriber<Subscriber>::value, "on_subscribe must be passed a subscriber.");

        auto so = std::make_shared<typename std::decay<Subscriber>::type>(std::forward<Subscriber>(o));
        ts->on_subscribe(
            make_subscriber<T>(
                *so,
                make_observer_dynamic<T>(
                // on_next
                    [so](T t){
                        so->on_next(t);
                    },
                // on_error
                    [so](std::exception_ptr e){
                        so->on_error(e);
                    },
                // on_completed
                    [so](){
                        so->on_completed();
                    })));
    }
};

}

template<class T>
class testable_observer
    : public observer<T>
{
    typedef observer<T> observer_base;
    typedef typename detail::test_subject_base<T>::type test_subject;
    test_subject ts;

public:
    typedef typename detail::test_subject_base<T>::recorded_type recorded_type;

    testable_observer(test_subject ts, observer_base ob)
        : observer_base(std::move(ob))
        , ts(std::move(ts))
    {
    }

    std::vector<recorded_type> messages() const {
        return ts->messages();
    }
};

//struct tag_test_observable : public tag_observable {};

template<class T>
class testable_observable
    : public observable<T, typename detail::test_source<T>>
{
    typedef observable<T, typename detail::test_source<T>> observable_base;
    typedef typename detail::test_subject_base<T>::type test_subject;
    test_subject ts;

    //typedef tag_test_observable observable_tag;

public:
    typedef typename detail::test_subject_base<T>::recorded_type recorded_type;

    explicit testable_observable(test_subject ts)
        : observable_base(detail::test_source<T>(ts))
        , ts(ts)
    {
    }

    std::vector<rxn::subscription> subscriptions() const {
        return ts->subscriptions();
    }

    std::vector<recorded_type> messages() const {
        return ts->messages();
    }
};

}
namespace rxt=test;

}

template<class T, class OperatorFactory>
auto operator >> (const rxcpp::test::testable_observable<T>& source, OperatorFactory&& of)
    -> decltype(source.op(std::forward<OperatorFactory>(of))) {
    return      source.op(std::forward<OperatorFactory>(of));
}

#include "schedulers/rx-test.hpp"

#endif
