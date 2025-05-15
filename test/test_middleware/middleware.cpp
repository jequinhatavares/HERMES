#include <unity.h>
#include <cstdio>
#include <string.h>
#include "../lib/middleware/strategies/strategy_inject.h"
#include "table.h"

//pio test -e native -f "test_middleware" -v

void setMetricValue(void* av, void*bv);

typedef struct metricTableEntry{
    int processingCapacity;
}metricTableEntry;

metricTableEntry metrics[TableMaxSize];

void setMetricValue(void* av, void*bv){
    metricTableEntry *a = (metricTableEntry *) av;
    metricTableEntry *b = (metricTableEntry *) bv;

    a->processingCapacity = b->processingCapacity;
}

void printMetricStruct(TableEntry* Table){
    LOG(NETWORK,INFO,"Node[%d.%d.%d.%d] → (Metric: %d) \n",
        ((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3],
        ((metricTableEntry *)Table->value)->processingCapacity);
}

void test_init_middleware(){
    metricTableEntry metric;
    int IP[4]={1,1,1,1};
    initMetricTable(setMetricValue, (void*) metrics,sizeof(metricTableEntry));

    metric.processingCapacity = 1;
    updateMiddlewareMetric(&metric,IP);

    tablePrint(metricTable, printMetricStruct);

    metricTableEntry *metricValue = (metricTableEntry*) tableRead(metricTable,IP);
    TEST_ASSERT(metricValue != nullptr);


}


void setUp(void){
    enableModule(STATE_MACHINE);
    enableModule(MESSAGES);
    enableModule(NETWORK);
    enableModule(DEBUG_SERVER);
    enableModule(CLI);

    lastModule = MESSAGES;
    currentLogLevel = DEBUG;
}

void tearDown(void){}

int main(int argc, char** argv){
    UNITY_BEGIN();
    RUN_TEST(test_init_middleware);
    //RUN_TEST(test_pir_incorrect);

    UNITY_END();
}
