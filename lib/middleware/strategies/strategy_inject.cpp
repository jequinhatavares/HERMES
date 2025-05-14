#include "strategy_inject.h"

TableEntry mTable[TableMaxSize];
TableInfo MTable = {
        .numberOfItems = 0,
        .isEqual = isIPEqual,
        .table = mTable,
        .setKey = setKey,
        .setValue = setValue,
};
TableInfo* metricTable = &MTable;

int IP[TableMaxSize][4];
routingTableEntry routingTableEntries[TableMaxSize];

void initMetricTable(){

}