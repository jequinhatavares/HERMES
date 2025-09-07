//#include "wifi_interface.h"
#include "wifi_common.h"

TableEntry lTable[TABLE_MAX_SIZE];
TableInfo LTable = {
        .numberOfItems = 0,
        .isEqual = isMACEqual,
        .table = lTable,
        .setKey = setMAC,
        .setValue = setConnectionStatus,
};
TableInfo* lostChildrenTable = &LTable;

int MAC[TABLE_MAX_SIZE][6];
unsigned long lastChildDisconnectionTime[TABLE_MAX_SIZE];

bool isMACEqual(void* a, void* b){
    int* aMAC = (int*) a;
    int* bMAC = (int*) b;
    //printf("In Function is MACEqual\n");
    if(aMAC[0] == bMAC[0] && aMAC[1] == bMAC[1] && aMAC[2] == bMAC[2] && aMAC[3] == bMAC[3] && aMAC[4] == bMAC[4] && aMAC[5] == bMAC[5]){
        return true;
    }
    return false;
}


void setMAC(void* av, void* bv){
    int* a = (int*) av;
    int* b = (int*) bv;
    //Serial.printf("Key.Setting old value: %i.%i.%i.%i to new value:  %i.%i.%i.%i\n", a[0],a[1],a[2],a[3], b[0],b[1],b[2],b[3]);
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
    a[3] = b[3];
    a[4] = b[4];
    a[5] = b[5];
}

void setConnectionStatus(void* av, void* bv){
    childConnectionStatus *a = (childConnectionStatus *) av;
    childConnectionStatus *b = (childConnectionStatus *) bv;

    //Serial.printf("Values.Setting old value: %i.%i.%i.%i to new value:  %i.%i.%i.%i\n", a->nextHopIP[0],a->nextHopIP[1],a->nextHopIP[2],a->nextHopIP[3], b->nextHopIP[0],b->nextHopIP[1],b->nextHopIP[2],b->nextHopIP[3]);
    //*a = *b;
    a->childDisconnectionTime = b->childDisconnectionTime;
    a->childTimedOut = b->childTimedOut;
}

void initAuxTables(){
    //Serial.printf("SizeOf int[4]: %i struct: %i", sizeof(int[4]), sizeof(RoutingTableEntry));
    tableInit(lostChildrenTable,MAC, lastChildDisconnectionTime, sizeof(int[6]),sizeof(unsigned long));
}

void setTableEntry(TableInfo *table, void* key, void* value){
    if( tableFind(table, key) == -1){//The node is not present in the table
        tableAdd(table, key, value);
    }else{//The node is already present in the table
        tableUpdate(table, key, value);
    }
}

void printLostChildrenHeader(){
    LOG(NETWORK,INFO,"==================================| Lost Children Table |=================================\n");
}
void printLostChild(TableEntry *Table){
    LOG(NETWORK,INFO,"NodeMAC[%hhu.%hhu.%hhu.%hhu.%hhu.%hhu] → isDisconnected[%d] | (Time: %lu) | (Sequence Number: %d)\n",
        ((uint8_t *)Table->key)[0],((uint8_t *)Table->key)[1],((uint8_t *)Table->key)[2]
        ,((uint8_t *)Table->key)[3],((uint8_t *)Table->key)[4],((uint8_t *)Table->key)[5],
        ((childConnectionStatus *)Table->value)->childTimedOut,((childConnectionStatus *)Table->value)->childDisconnectionTime);
}