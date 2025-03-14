#ifndef TABLE_H
#define TABLE_H

#define TableMaxSize 10

/*
bool defaultEqual(void* a, void* b){
    return a == b;
}

{.numberOfItems = 0, .isEqual = defaultEqual, .table = {}};
*/

typedef struct TableEntry{
    void* key;
    void* value;
} TableEntry;


typedef struct TableInfo{
    int numberOfItems;
    bool (*isEqual)(void* a, void* b);
    TableEntry* table;
//#ifdef PREALLOCATE_TABLE
    void (*setKey)(void* , void*);
    void (*setValue)(void* , void*);
//#endif
} TableInfo;
//extern TableEntry Table[MaxSize];

TableInfo* tableCreate(bool (*)(void*, void*));  // DEPRECATED
void tableInit(TableInfo* T, void** keys, void** values);
int tableFind(TableInfo* Table, void* key);
void* tableRead(TableInfo*, void*);
void tableAdd(TableInfo*, void* key, void* value);
void tableUpdate(TableInfo*, void* key, void* value);
void tableRemove(TableInfo*, void* key);
void tablePrint(TableInfo* T, void (*print)(TableEntry*));
void tableClean(TableInfo* T);

#endif //TABLE_H
