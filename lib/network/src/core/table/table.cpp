#include <cstdio>
#include "table.h"


#define PREALLOCATE_TABLE

/**
 * tableCreate
 * Creates a static table and initializes it with a comparison function.
 *
 * @param pFunction - A pointer to a function that compares two keys.
 * @return TableInfo* - A pointer to the created table.
 */
//TableInfo* tableCreate(bool (*pFunction)(void* a, void* b)){
//    static TableEntry table[TABLE_MAX_SIZE];
//    static TableInfo Table = {
//            .numberOfItems = 0,
//            .isEqual = pFunction,
//            .table = table
//    };
//
//    //isEqual = pFunction;
//    return &Table;
//}

/**
 * tableInit
 * This function initializes the table by assigning the keys and values pointers to the address of the global allocated
 * entries of the table.
 *
 * @param T Pointer to the table structure to be initialized.
 * @param keys Pointer to the array of keys.
 * @param values Pointer to the array of values.
 * @param key_size Size of each key in bytes.
 * @param value_size Size of each value in bytes.
 */
void tableInit(TableInfo * T, void* keys, void* values, size_t key_size, size_t value_size){
    for (int i = 0; i < T->maxNumberOfItems; i++) {
        T->table[i].key = (char*)keys + (i * key_size);
        T->table[i].value = (char*)values + (i * value_size);
    }
}

void* tableKey(TableInfo * T, int index){
    // Safeguard: ensure the provided index is within the bounds of the table
    if (index>=T->numberOfItems || index < 0){return nullptr;}
    return T->table[index].key;
}

void* tableValueAtIndex(TableInfo * T, int index){
    // Safeguard: ensure the provided index is within the bounds of the table
    if (index>=T->numberOfItems || index < 0){return nullptr;}
    return T->table[index].value;
}
/**
 * tableFind
 * Searches for a key in the table and returns its index.
 *
 * @param T - A pointer to the table.
 * @param Key - The key to search for.
 * @return int - The index of the key if found, otherwise -1.
 */
int tableFind(TableInfo* T, void* key){
    int i;

    // Safeguard: ensure the provided key pointer is not null before proceeding
    if (key == nullptr) return -1;

    for(i=0; i<T->numberOfItems; i++){
        bool result = T->isEqual(T->table[i].key, key);
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

    // Safeguard: ensure the provided key pointer is not null before proceeding
    if (key == nullptr) return nullptr;

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

    if(T->numberOfItems == T->maxNumberOfItems)return;

#ifndef PREALLOCATE_TABLE
    T->table[T->numberOfItems].key=key;
    T->table[T->numberOfItems].value=value;
#endif
#ifdef PREALLOCATE_TABLE
    //Serial.printf("number of items: %i\n", T->numberOfItems);
    //Serial.printf("values of keys being initialized: %i.%i.%i.%i\n", ((int*)T->table[T->numberOfItems].key)[0],((int*)T->table[T->numberOfItems].key)[1],((int*)T->table[T->numberOfItems].key)[2],((int*)T->table[T->numberOfItems].key)[3]);
    //Serial.printf("value of values being initialized: %i.%i.%i.%i %i\n", ((int*)T->table[T->numberOfItems].value)[0],((RoutingTableEntry*)T->table[T->numberOfItems].value)->nextHopIP[1],((RoutingTableEntry*)T->table[T->numberOfItems].value)->nextHopIP[2],((RoutingTableEntry*)T->table[T->numberOfItems].value)->nextHopIP[3],((RoutingTableEntry*)T->table[T->numberOfItems].value)->hopDistance);
    T->setKey(T->table[T->numberOfItems].key, key);
    T->setValue(T->table[T->numberOfItems].value, value);
#endif
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

#ifndef PREALLOCATE_TABLE
    T->table[index].value=value;
#endif
#ifdef PREALLOCATE_TABLE
    T->setValue(T->table[index].value, value);
#endif
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
    int index = tableFind(T, key);
    if(index == -1) return;
    //T->table[index].value = nullptr;
    for (int i = index; i < T->numberOfItems-1; i++) {

    #ifndef PREALLOCATE_TABLE
        T->table[i].key= T->table[i+1].key;
        T->table[i].value= T->table[i+1].value;
    #endif
    #ifdef PREALLOCATE_TABLE
        T->setKey(T->table[i].key, T->table[i+1].key);
        T->setValue(T->table[i].value, T->table[i+1].value);
    #endif

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
void tablePrint(TableInfo* T, void (*printHeader)(), void (*printEntry)(TableEntry*)){
    printHeader();
    for(int i=0; i<T->numberOfItems; i++){
        printEntry(&T->table[i]);
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
    #ifndef PREALLOCATE_TABLE
    for (int i = 0; i < T->numberOfItems; i++) {
        T->table[i].key = nullptr;
        T->table[i].value = nullptr;
    }
    #endif
    T->numberOfItems = 0;
}