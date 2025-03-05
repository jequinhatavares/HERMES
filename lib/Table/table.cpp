#include "table.h"

//int NumberOfItems = 0;

//bool (*isEqual)(void* a, void* b) = nullptr;

TableInfo* tableCreate(bool (*pFunction)(void* a, void* b)){
    static TableEntry table[MaxSize];
    static TableInfo Table = {
            .numberOfItems = 0,
            .isEqual = pFunction,
            .table = table
    };

    //isEqual = pFunction;
    return &Table;
}

int tableFind(TableInfo* T, void* Key){
    int i;
    for(i=0; i<T->numberOfItems; i++){
        //if(Table[i].key == Key)
        if(T->isEqual(T->table[i].key, Key))
            return i;
    }
    return -1;
}

void* tableRead(TableInfo* T, void* key){
    int i = tableFind(T, key);
    if (i==-1) return nullptr;
    return T->table[i].value;
}

void tableAdd(TableInfo* T, void* key, void* value){
    T->table[T->numberOfItems].key=key;
    T->table[T->numberOfItems].value=value;
    T->numberOfItems++;
}

void tableUpdate(TableInfo* T, void* key, void* value){
    int index = tableFind(T, key);
    if(index == -1) return;
    T->table[index].value = value;
}

void tableRemove(TableInfo* T, void* key){
    int i, index = tableFind(T, key);
    if(index == -1) return;
    T->table[index].value = nullptr;
    for (int i = index; i < MaxSize-1; i++) {
        T->table[i].key= T->table[i+1].key;
        T->table[i].value= T->table[i+1].value;
    }
    T->numberOfItems --;
}