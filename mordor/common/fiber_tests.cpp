#include <cassert>
#include <stdio.h>

#include "fiber.h"

static Fiber *g_mainFiber, *g_a;

static void fiberProc()
{
    assert(g_mainFiber->state() == Fiber::EXEC);
    assert(g_a->state() == Fiber::EXEC);
    printf("In fiber\n");
    Fiber::yield();
    assert(g_mainFiber->state() == Fiber::EXEC);
    assert(g_a->state() == Fiber::EXEC);
    printf("In fiber again\n");
}

static void fiberProc2()
{
    assert(g_mainFiber->state() == Fiber::HOLD);
    assert(g_a->state() == Fiber::EXEC);
    printf("In fiber\n");
    g_mainFiber->yieldTo();
    assert(g_mainFiber->state() == Fiber::HOLD);
    assert(g_a->state() == Fiber::EXEC);
    printf("In fiber again\n");
}

int main()
{
    Fiber mainFiber;
    Fiber a(&fiberProc, 8192);
    g_mainFiber = &mainFiber;
    g_a = &a;
    assert(g_mainFiber->state() == Fiber::EXEC);
    assert(g_a->state() == Fiber::HOLD);
    printf("In main\n");
    a.call();
    assert(g_mainFiber->state() == Fiber::EXEC);
    assert(g_a->state() == Fiber::HOLD);
    printf("in main again\n");
    a.call();
    assert(g_mainFiber->state() == Fiber::EXEC);
    assert(g_a->state() == Fiber::TERM);
    printf("finished\n");
    a.reset(&fiberProc2);
    assert(g_mainFiber->state() == Fiber::EXEC);
    assert(g_a->state() == Fiber::HOLD);
    a.yieldTo();
    assert(g_mainFiber->state() == Fiber::EXEC);
    assert(g_a->state() == Fiber::HOLD);
    printf("in main again\n");
    a.yieldTo();
    assert(g_mainFiber->state() == Fiber::EXEC);
    assert(g_a->state() == Fiber::TERM);
    printf("finished\n");
    return 0;
}
