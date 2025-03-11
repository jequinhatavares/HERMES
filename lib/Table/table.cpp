#include <stdio.h>
#include "table.h"

//int NumberOfItems = 0;

//bool (*isEqual)(void* a, void* b) = nullptr;

/**
 * tableCreate
 * Creates a static table and initializes it with a comparison function.
 *
 * @param pFunction - A pointer to a function that compares two keys.
 * @return TableInfo* - A pointer to the created table.
 */
TableInfo* tableCreate(bool (*pFunction)(void* a, void* b)){
    static TableEntry table[TableMaxSize];
    static TableInfo Table = {
            .numberOfItems = 0,
            .isEqual = pFunction,
            .table = table
    };

    //isEqual = pFunction;
    return &Table;
}

/**
 * tableFind
 * Searches for a key in the table and returns its index.
 *
 * @param T - A pointer to the table.
 * @param Key - The key to search for.
 * @return int - The index of the key if found, otherwise -1.
 */
int tableFind(TableInfo* T, void* Key){
    int i;
    for(i=0; i<T->numberOfItems; i++){
        //if(Table[i].key == Key)
        bool result = T->isEqual(T->table[i].key, Key);
        if(result){
            //printf("tableFind Returned: %i\n",i);
            return i;
        }
    }
    return -1;
}

/**
 * tableRead
 * Retrieves the value associated with a given key in the table.
 *
 * @param T - A pointer to the table.
 * @param key - The key to look up.
 * @return void* - A pointer to the associated value, or nullptr if the key is not found.
 */
void* tableRead(TableInfo* T, void* key){
    int i = tableFind(T, key);
    if (i==-1) return nullptr;
    //printf("tableRead value: %i\n",((int*)T->table[i].value)[0]);
    return T->table[i].value;
}

/**
 * tableAdd
 * Adds a new key-value pair to the table.
 *
 * @param T - A pointer to the table.
 * @param key - The key to add.
 * @param value - The value associated with the key.
 * @return void
 */
void tableAdd(TableInfo* T, void* key, void* value){
    T->table[T->numberOfItems].key=key;
    T->table[T->numberOfItems].value=value;
    T->numberOfItems++;
}

/**
 * tableUpdate
 * Updates the value of an existing key in the table.
 *
 * @param T - A pointer to the table.
 * @param key - The key to update.
 * @param value - The new value to associate with the key.
 * @return void
 */
void tableUpdate(TableInfo* T, void* key, void* value){
    int index = tableFind(T, key);
    if(index == -1) return;
    T->table[index].value = value;
}

/**
 * tableRemove
 * Removes a key-value pair from the table by shifting entries.
 *
 * @param T - A pointer to the table.
 * @param key - The key to remove.
 * @return void
 */
void tableRemove(TableInfo* T, void* key){
    int i, index = tableFind(T, key);
    if(index == -1) return;
    T->table[index].value = nullptr;
    for (int i = index; i < T->numberOfItems-1; i++) {
        T->table[i].key= T->table[i+1].key;
        T->table[i].value= T->table[i+1].value;
    }
    T->numberOfItems --;
}

/**
 * tablePrint
 * Prints the contents of the table using a user-defined print function.
 *
 * @param T - A pointer to the table.
 * @param print - A function pointer to print a table entry.
 * @return void
 */
void tablePrint(TableInfo* T, void (*print)(TableEntry*)){
    int i;
    for(i=0; i<T->numberOfItems; i++){
        print(&T->table[i]);
    }
}

/**
 * tableClean
 * Clears all entries in the table by setting keys and values to nullptr and resetting the item count.
 *
 * @param T - A pointer to the table.
 * @return void
 */
void tableClean(TableInfo* T){
    int i;
    for (int i = 0; i < T->numberOfItems; i++) {
        T->table[i].key = nullptr;
        T->table[i].value = nullptr;
    }
    T->numberOfItems = 0;
}