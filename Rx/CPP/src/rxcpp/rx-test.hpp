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

    virtual void on_subscribe(observer<T>) const =0;
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
    }
    typename test_subject_base<T>::type ts;
    void on_subscribe(observer<T> o) const {
        ts->on_subscribe(std::move(o));
    }
    template<class Observer>
    typename std::enable_if<!std::is_same<typename std::decay<Observer>::type, observer<T>>::value, void>::type
    on_subscribe(Observer o) const {
        auto so = std::make_shared<Observer>(o);
        ts->on_subscribe(make_observer_dynamic<T>(
            so->get_subscription(),
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
            }));
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

template<class T>
class testable_observable
    : public observable<T, typename detail::test_source<T>>
{
    typedef observable<T, typename detail::test_source<T>> observable_base;
    typedef typename detail::test_subject_base<T>::type test_subject;
    test_subject ts;

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

#include "schedulers/rx-test.hpp"

#endif