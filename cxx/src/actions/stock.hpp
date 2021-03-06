/****************************************************************************
 * Copyright (c) 2020-2016 Jimmy M. Gong                                    *
 * All rights reserved.                                                     *
 *                                                                          *
 * Distributed under the terms of the Apache License 2.0 License.           *
 *                                                                          *
 * The full license is in the file LICENSE, distributed with this software. *
 ****************************************************************************/

#pragma once

#include <QDataStream>
#include <QFuture>
#include <QList>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QVector>
#include <QtConcurrent/QtConcurrentRun>
#include <TActionContext>
#include <TfTest/TfTest>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>

#include "DataFrame/DataFrame.h"
#include "abquant/actions/indicator.hpp"
#include "abquant/actions/utils.hpp"

namespace abq
{
using namespace hmdf;
using namespace std;
using MyDataFrame = StdDataFrame<std::string>;

/***************************
 * StockAction declaration *
 ***************************/

template <class SA>
class StockAction;

template <class SA>
class StockAction : public TActionContext
{
public:
    using self_type    = StockAction<SA>;
    using derived_type = SA;

public:
    // derived_type &derived_cast() & noexcept;
    derived_type* derived_cast() & noexcept;
    const derived_type& derived_cast() const& noexcept;
    derived_type derived_cast() && noexcept;

    // TODO: friend class Indicator<S, derived_type, StockAction>
    QStringList getCodes() const;
    Indicator<self_type> makeIndicator();

protected:
    //! Default constructor
    StockAction() = delete;

    StockAction(QStringList);

    //! Copy constructor
    StockAction(const StockAction& other) = default;

    //! Move constructor
    StockAction(StockAction&& other) noexcept = default;

    // ! Destructor
    ~StockAction() noexcept = default;

    //! Copy assignment operator
    StockAction& operator=(const StockAction& other) = default;

    //! Move assignment operator
    StockAction& operator=(StockAction&& other) noexcept = default;

protected:
    template <typename T>
    QVector<T> toSeries(const char*) const noexcept;

    MyDataFrame toFq();
    MyDataFrame toDataFrame();
    template <typename S>
    QList<S> run(const QStringList codes, const char* start, const char* end);
    template <typename S>
    QList<S> run(const QStringList codes, const char* start, const char* end, MIN_FREQ freq);
    template <typename S>
    QList<S> run(const QStringList codes, int category = 1);
    template <typename S>
    QList<S> get_price(const QStringList codes, const char* start, const char* end);
    template <typename S>
    QList<S> get_price(const QStringList codes, const char* start, const char* end, MIN_FREQ freq);
    template <typename S>
    QList<S> get_price(const QStringList codes, int category);

    auto getStocks();

private:
    friend SA;
    friend class Indicator<self_type>;
    QStringList m_codes;
    const char* m_start;
    const char* m_end;
};

/******************************
 * StockAction implementation *
 ******************************/

template <class SA>
StockAction<SA>::StockAction(QStringList codes) : m_codes{codes}
{
}

// since: /usr/include/treefrog/tactioncontext.h:59:20: note: 'TActionContext'
// has been explicitly marked deleted here
// T_DISABLE_COPY(TActionContext)
// workaround here, SA*
// template <class SA>
//     inline auto StockAction<SA>::derived_cast() & noexcept -> derived_type & {
//     return *static_cast<derived_type *>(this);
// }

/**
 * @name Downcast functions
 */
//@{
/**
 * Returns a pointer to the actual derived type of the StockAction.
 */
template <class SA>
    inline auto StockAction<SA>::derived_cast() & noexcept -> derived_type*
{
    return static_cast<derived_type*>(this);
}

/**
 * Returns a constant reference to the actual derived type of the StockAction.
 */
template <class SA>
    inline auto StockAction<SA>::derived_cast() const & noexcept -> const derived_type&
{
    return *static_cast<const derived_type*>(this);
}

/**
 * Returns a constant reference to the actual derived type of the StockAction.
 */
template <class SA>
    inline auto StockAction<SA>::derived_cast() && noexcept -> derived_type
{
    return *static_cast<derived_type*>(this);
}

template <class SA>
template <typename S>
QList<S> StockAction<SA>::run(const QStringList codes, const char* start, const char* end)
{
    QFuture<QList<S>> future = QtConcurrent::run(this, &self_type::get_price<S>, codes, start, end);

    QList<S> stocks = future.result();
    return stocks;
}

template <class SA>
template <typename S>
QList<S> StockAction<SA>::run(const QStringList codes, const char* start, const char* end, MIN_FREQ freq)
{
    QFuture<QList<S>> future = QtConcurrent::run(this, &self_type::get_price<S>, codes, start, end, freq);

    QList<S> stocks = future.result();
    return stocks;
}

template <class SA>
template <typename S>
QList<S> StockAction<SA>::run(const QStringList codes, int category)
{
    QFuture<QList<S>> future = QtConcurrent::run(this, &self_type::get_price<S>, codes, category);

    QList<S> stocks = future.result();
    return stocks;
}

template <class SA>
template <typename T>
QVector<T> StockAction<SA>::toSeries(const char* col) const noexcept
{
    auto sa = derived_cast();
    return sa->toSeries(col);
}

template <class SA>
template <typename S>
QList<S> StockAction<SA>::get_price(const QStringList codes, const char* start, const char* end)
{
    double start_d = QDateTime::fromString(QString::fromUtf8(start), Qt::ISODate).toSecsSinceEpoch();
    double end_d   = QDateTime::fromString(QString::fromUtf8(end), Qt::ISODate).toSecsSinceEpoch();
    auto sa        = derived_cast();
    TDatabaseContext::setCurrentDatabaseContext(sa);
    bool EnableTransactions = true;
    setTransactionEnabled(EnableTransactions);
    QList<S> stocks = S::get_price(codes, start_d, end_d);
    commitTransactions();
    return stocks;
}

template <class SA>
template <typename S>
QList<S> StockAction<SA>::get_price(const QStringList codes, const char* start, const char* end, MIN_FREQ freq)
{
    double start_d = QDateTime::fromString(QString::fromUtf8(start), Qt::ISODate).toSecsSinceEpoch();
    double end_d   = QDateTime::fromString(QString::fromUtf8(end), Qt::ISODate).toSecsSinceEpoch();
    auto sa        = derived_cast();
    TDatabaseContext::setCurrentDatabaseContext(sa);
    bool EnableTransactions = true;
    setTransactionEnabled(EnableTransactions);
    QList<S> stocks = S::get_price(codes, start_d, end_d, freq);
    commitTransactions();
    return stocks;
}

template <class SA>
template <typename S>
QList<S> StockAction<SA>::get_price(const QStringList codes, int category)
{
    auto sa = derived_cast();
    TDatabaseContext::setCurrentDatabaseContext(sa);
    bool EnableTransactions = true;
    setTransactionEnabled(EnableTransactions);
    QList<S> stocks = S::get_price(codes, category);
    commitTransactions();
    return stocks;
}

template <class SA>
QStringList StockAction<SA>::getCodes() const
{
    return m_codes;
}

template <class SA>
inline auto StockAction<SA>::getStocks()
{
    auto sa = derived_cast();
    return sa->getStocks();
}

template <class SA>
inline auto StockAction<SA>::makeIndicator() -> Indicator<self_type>
{
    auto sa = derived_cast();
    return Indicator<self_type>(sa);
}

} // namespace abq
