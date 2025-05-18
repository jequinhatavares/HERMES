#include "strategy_pubsub.h"


/***
 * Middleware Publish Subscribe table
 *
 * mTable[TableMaxSize] - An array where each element is a struct containing two pointers:
 *                         one to the key (used for indexing the metrics table) and another to the value (the corresponding entry).
 *
 * TTable - A struct that holds metadata for the metrics table, including:
 * * * .numberOfItems - The current number of entries in the metrics table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
* * * .table - A pointer to the mTable.
 *
 * childrenTable - A pointer to TTable, used for accessing the children table.
 *
 * valuesPubSub[TableMaxSize] - Preallocated memory for storing the published and subscribed values of each node
 ***/
TableEntry psTable[TableMaxSize];
TableInfo PSTable = {
        .numberOfItems = 0,
        .isEqual = isIPEqual,
        .table = psTable,
        .setKey = setKey,
        .setValue = nullptr,
};
TableInfo* publishSubscribeTable = &PSTable;


int nodes[TableMaxSize][4];



PubSubInfo valuesPubSub[TableMaxSize];

void initMiddleware(void* ){

}

void encodeMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize) {

}

void handleMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize) {
    char infoPubSub[20];
    int IP[4],nodeIP[4],topic,i,k;
    PubSubMessageType type;
    PubSubInfo pbNewInfo,*pbCurrentRecord;
    bool isTableUpdated = false;
    //MESSAGE_TYPE [sender IP] [nodeIP] metric
    sscanf(messageBuffer,"%*i %i.%i.%i.%i %s",&IP[0],&IP[1],&IP[2],&IP[3],infoPubSub);

    sscanf(infoPubSub,"%i",&type);
    switch (type) {

        case PUBSUB_PUBLISH:
            //13 [destination IP] 0 [Publisher IP] [Published Topic]
            sscanf(infoPubSub,"%*i %i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);
            break;
        case PUBSUB_SUBSCRIBE:
            //Message sent when a one subscribes to a certain topic
            //13 [destination IP] 1 [Subscriber IP] [Topic]
            sscanf(infoPubSub,"%*i %i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);
            pbCurrentRecord = (PubSubInfo*) tableRead(publishSubscribeTable,nodeIP);
            if(pbCurrentRecord != nullptr){
                for (i = 0; i < MAX_TOPICS; i++) {
                    // Save all topics published by the node as-is.
                    pbNewInfo.publishedTopics[i] = pbCurrentRecord->publishedTopics[i];
                    // Skip update if the node is already marked as subscribed to this topic.
                    if(pbCurrentRecord->subscribedTopics[i] == topic){
                        isTableUpdated = true;
                        break;
                    }
                    //Save the new subscribed topic in the first available slot (indicated by -1)
                    if(pbCurrentRecord->subscribedTopics[i] == -1){
                        pbNewInfo.subscribedTopics[i] = topic;
                    }else{// Preserve all other subscriptions as they are
                        pbNewInfo.subscribedTopics[i] = pbCurrentRecord->subscribedTopics[i];
                    }
                }
                if(!isTableUpdated)tableUpdate(publishSubscribeTable,nodeIP,&pbNewInfo);
            }else{
                //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1,
                // except for the announced subscribed topic
                for (int i = 0; i < MAX_TOPICS; i++){
                    pbNewInfo.publishedTopics[i] = -1;
                    if(i == 0){pbNewInfo.subscribedTopics[i] = topic;}
                    else{pbNewInfo.subscribedTopics[i] = -1;}
                }
                tableAdd(publishSubscribeTable,nodeIP,&pbNewInfo);
            }
            break;

        case PUBSUB_UNSUBSCRIBE:
            //Message sent when a one unsubscribes to a certain topic
            //13 [destination IP] 2 [Unsubscriber IP] [Topic]
            sscanf(infoPubSub,"%*i %i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

            pbCurrentRecord = (PubSubInfo*) tableRead(publishSubscribeTable,nodeIP);
            if(pbCurrentRecord != nullptr){
                for (i = 0; i < MAX_TOPICS; i++) {
                    // Save all topics published by the node as-is.
                    pbNewInfo.publishedTopics[i] = pbCurrentRecord->publishedTopics[i];

                    //If the topic is found in the subscribed topics list, unsubscribe
                    if(pbCurrentRecord->subscribedTopics[i] == topic){
                        // Remove the subscription by shifting all subsequent entries one position forward to overwrite the target
                        for (k = i; k < MAX_TOPICS-1; k++) {
                            pbNewInfo.subscribedTopics[k] = pbCurrentRecord->subscribedTopics[k+1];
                        }
                        //TODO put the last position to -1
                        pbNewInfo.subscribedTopics[MAX_TOPICS-1] = -1;

                        isTableUpdated = true;
                        break;
                    }

                }
                if(!isTableUpdated)tableUpdate(publishSubscribeTable,nodeIP,&pbNewInfo);
            }else{
                //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1
                for (int i = 0; i < MAX_TOPICS; i++){
                    pbNewInfo.publishedTopics[i] = -1;
                    pbNewInfo.subscribedTopics[i] = -1;
                }
                tableAdd(publishSubscribeTable,nodeIP,&pbNewInfo);
            }

            break;
        case PUBSUB_ADVERTISE:
            // Message used to advertise that a node is publishing a new topic
            //13 [sender IP] 3 [Publisher IP] [Published Topic]
            sscanf(infoPubSub,"%*i %i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

            if(pbCurrentRecord != nullptr){
                for (i = 0; i < MAX_TOPICS; i++) {
                    // Save all subscribed by the node as-is.
                    pbNewInfo.subscribedTopics[i] = pbCurrentRecord->subscribedTopics[i];
                    // Skip update if the node is already marked as publisher to this topic.
                    if(pbCurrentRecord->publishedTopics[i] == topic){
                        isTableUpdated = true;
                        break;
                    }
                    //Save the new published topic in the first available slot (indicated by -1)
                    if(pbCurrentRecord->publishedTopics[i] == -1){
                        pbNewInfo.publishedTopics[i] = topic;
                    }else{// Preserve all other subscriptions as they are
                        pbNewInfo.publishedTopics[i] = pbCurrentRecord->publishedTopics[i];
                    }
                }
                if(!isTableUpdated)tableUpdate(publishSubscribeTable,nodeIP,&pbNewInfo);
            }else{
                //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1,
                // except for the announced subscribed topic
                for (int i = 0; i < MAX_TOPICS; i++){
                    pbNewInfo.subscribedTopics[i] = -1;
                    if(i == 0){pbNewInfo.publishedTopics[i] = topic;}
                    else{pbNewInfo.publishedTopics[i] = -1;}
                }
                tableAdd(publishSubscribeTable,nodeIP,&pbNewInfo);
            }
            break;

        case PUBSUB_UNADVERTISE:
            // Message used to advertise that a node is unpublishing a topic
            //13 [sender IP] 3 [Publisher IP] [UnPublished Topic]
            sscanf(infoPubSub,"%*i %i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

            pbCurrentRecord = (PubSubInfo*) tableRead(publishSubscribeTable,nodeIP);
            if(pbCurrentRecord != nullptr){
                for (i = 0; i < MAX_TOPICS; i++) {
                    // Save all topics published by the node as-is.
                    pbNewInfo.subscribedTopics[i] = pbCurrentRecord->subscribedTopics[i];

                    //If the topic is found in the subscribed topics list, unsubscribe
                    if(pbCurrentRecord->publishedTopics[i] == topic){
                        // Remove the subscription by shifting all subsequent entries one position forward to overwrite the target
                        for (k = i; k < MAX_TOPICS-1; k++) {
                            pbNewInfo.publishedTopics[k] = pbCurrentRecord->publishedTopics[k+1];
                        }
                        //TODO put the last position to -1
                        pbNewInfo.publishedTopics[MAX_TOPICS-1] = -1;

                        isTableUpdated = true;
                        break;
                    }

                }
                if(!isTableUpdated)tableUpdate(publishSubscribeTable,nodeIP,&pbNewInfo);
            }else{
                //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1
                for (int i = 0; i < MAX_TOPICS; i++){
                    pbNewInfo.publishedTopics[i] = -1;
                    pbNewInfo.subscribedTopics[i] = -1;
                }
                tableAdd(publishSubscribeTable,nodeIP,&pbNewInfo);
            }

            break;

        default:
            break;

    }

}

void middlewareInfluenceRoutingPubSub(char* dataMessage){

}

void setPubSubInfo(void* av, void* bv){
    PubSubInfo *a = (PubSubInfo*) av;
    PubSubInfo *b = (PubSubInfo*) bv;

    for (int i = 0; i < MAX_TOPICS; i++) {
        a->publishedTopics[i] = b->publishedTopics[i];
        a->subscribedTopics[i] = b->subscribedTopics[i];
    }
}
