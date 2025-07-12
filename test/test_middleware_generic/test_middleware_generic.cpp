#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_inject/strategy_inject.h"
#include "middleware.h"
#include "table.h"

//pio test -e native -f "test_middleware_generic" -v
MetricTableEntry metrics[TABLE_MAX_SIZE];

void setUp(void){
}

void tearDown(void){}

void test_select_strategy(){
    middlewareSelectStrategy(STRATEGY_INJECT);
    initStrategyInject((void*) metrics,sizeof(MetricTableEntry),setMetricValue,encodeMetricEntry,decodeMetricEntry);

    InjectContext *api = (InjectContext*) middlewareGetStrategyContext();
}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_select_strategy);
    UNITY_END();
}
