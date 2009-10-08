// Copyright (c) 2009 - Decho Corp.

#include "mordor/common/pch.h"

#include <boost/bind.hpp>

#include "mordor/common/timer.h"
#include "mordor/test/test.h"

using namespace Mordor;
using namespace Mordor::Test;

void
singleTimer(int &sequence, int &expected)
{
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, expected);
}

MORDOR_UNITTEST(Timer, single)
{
    int sequence = 0;
    TimerManager manager;
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    manager.registerTimer(0, boost::bind(&singleTimer, boost::ref(sequence), 1));
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), 0u);
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
    manager.processTimers();
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, 2);
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
}

MORDOR_UNITTEST(Timer, multiple)
{
    int sequence = 0;
    TimerManager manager;
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    manager.registerTimer(0, boost::bind(&singleTimer, boost::ref(sequence),
        boost::ref(sequence)));
    manager.registerTimer(0, boost::bind(&singleTimer, boost::ref(sequence),
        boost::ref(sequence)));
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), 0u);
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
    manager.processTimers();
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, 3);
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
}

MORDOR_UNITTEST(Timer, cancel)
{
    int sequence = 0;
    TimerManager manager;
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    Timer::ptr timer =
        manager.registerTimer(0, boost::bind(&singleTimer, boost::ref(sequence), 1));
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), 0u);
    timer->cancel();
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    manager.processTimers();
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
}

MORDOR_UNITTEST(Timer, idempotentCancel)
{
    int sequence = 0;
    TimerManager manager;
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    Timer::ptr timer =
        manager.registerTimer(0, boost::bind(&singleTimer, boost::ref(sequence), 1));
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), 0u);
    timer->cancel();
    timer->cancel();
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    manager.processTimers();
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
}

MORDOR_UNITTEST(Timer, idempotentCancelAfterSuccess)
{
    int sequence = 0;
    TimerManager manager;
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    Timer::ptr timer =
        manager.registerTimer(0, boost::bind(&singleTimer, boost::ref(sequence), 1));
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), 0u);
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
    manager.processTimers();
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, 2);
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    timer->cancel();
    timer->cancel();
}


MORDOR_UNITTEST(Timer, recurring)
{
    int sequence = 0;
    int expected;
    TimerManager manager;
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    Timer::ptr timer =
        manager.registerTimer(0, boost::bind(&singleTimer, boost::ref(sequence),
        boost::ref(expected)), true);
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), 0u);
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
    expected = 1;
    manager.processTimers();
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, 2);
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), 0u);
    expected = 3;
    manager.processTimers();
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, 4);
    timer->cancel();
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
}

MORDOR_UNITTEST(Timer, later)
{
    int sequence = 0;
    TimerManager manager;
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
    Timer::ptr timer = manager.registerTimer(1000 * 1000 * 1000,
        boost::bind(&singleTimer, boost::ref(sequence), 1));
    MORDOR_TEST_ASSERT_ABOUT_EQUAL(manager.nextTimer(),
        1000 * 1000 * 1000u, 100 * 1000 * 1000u);
    MORDOR_TEST_ASSERT_EQUAL(sequence, 0);
    manager.processTimers();
    ++sequence;
    MORDOR_TEST_ASSERT_EQUAL(sequence, 1);
    timer->cancel();
    MORDOR_TEST_ASSERT_EQUAL(manager.nextTimer(), ~0ull);
}
