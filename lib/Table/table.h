#ifndef TABLE_H
#define TABLE_H

#define MaxSize 10

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
} TableInfo;
//extern TableEntry Table[MaxSize];

TableInfo* tableCreate(bool (*)(void*, void*));
int tableFind(TableInfo* Table, void* key);
void* tableRead(TableInfo*, void*);
void tableAdd(TableInfo*, void* key, void* value);
void tableUpdate(TableInfo*, void* key, void* value);
void tableRemove(TableInfo*, void* key);
void tablePrint(TableInfo* T, void (*print)(TableEntry*));

#endif //TABLE_H
