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
TableInfo* pubsubTable = &PSTable;


int nodes[TableMaxSize][4];


//Function Pointers Initializers
void (*encodeTopicValue)(char*,size_t,void *) = nullptr;
void (*decodeTopicValue)(char*,void *) = nullptr;

PubSubInfo valuesPubSub[TableMaxSize];

void initMiddlewarePubSub(void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,void *) ){
    //Initialize the pubsubTable
    tableInit(pubsubTable, nodes, valuesPubSub, sizeof(int[4]), sizeof(PubSubInfo));

    //Initialize function to encode/decode the topics value
    encodeTopicValue = encodeTopicFunction;
    decodeTopicValue = decodeTopicFunction;
}

void encodeMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize) {
    PubSubMessageType typePubSub;
    messageType globalMessageType;
    int destinationIP[4];
    int topic;
    int nodeIP[4],IP[4];

    // If the encoded message already contains topic information, it means this is a propagation of an already encoded message.
    // In this case, only the sender address needs to be updated before further propagation.
    if( sscanf(messageBuffer,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",&globalMessageType,&typePubSub,&IP[0],&IP[1],&IP[2],&IP[3]
            ,&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3], &topic) == 11 ){

        snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",&globalMessageType,&typePubSub,myIP[0],myIP[1],myIP[2],myIP[3]
                ,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3], topic);

    }else{ // If the message only contains the type, it indicates that this node should encode its own topic information
        switch (typePubSub) {
            case PUBSUB_PUBLISH:
                break;
            case PUBSUB_SUBSCRIBE:
                //Message sent when a one subscribes to a certain topic
                //13 1 [destination IP] [Subscriber IP] [Topic]
                snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE,
                         destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
                break;
            case PUBSUB_UNSUBSCRIBE:
                //Message sent when a one unsubscribes to a certain topic
                //13 2 [destination IP] [Unsubscriber IP] [Topic]
                snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE,
                         destinationIP[0],destinationIP[1],destinationIP[2],destinationIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
                break;
            case PUBSUB_ADVERTISE:
                // Message used to advertise that the node is publishing a new topic
                //13 3 [sender IP] [Publisher IP] [Published Topic]
                snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE,
                         myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
                break;
            case PUBSUB_UNADVERTISE:
                // Message used to advertise that the node is unpublishing a topic
                //13 4 [sender IP] [Publisher IP] [UnPublished Topic]
                snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE,
                         myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
                break;
        }
    }

}

void handleMiddlewareMessagePubSub(char* messageBuffer, size_t bufferSize) {
    char infoPubSub[20];
    int IP[4],nodeIP[4],topic,i,k;
    PubSubMessageType type;
    PubSubInfo pbNewInfo,*pbCurrentRecord;
    bool isTableUpdated = false;
    routingTableEntry *routingTableValue;
    //MESSAGE_TYPE  PUBSUB_TYPE  [sender/destination IP]  [nodeIP]  topic
    sscanf(messageBuffer,"%*i %i %i.%i.%i.%i %s",&type,&IP[0],&IP[1],&IP[2],&IP[3],infoPubSub);

    switch (type) {

        case PUBSUB_PUBLISH:
            //13  0 [destination IP] [Publisher IP] [Published Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);
            break;
        case PUBSUB_SUBSCRIBE:
            //Message sent when a one node subscribes to a certain topic
            //13 1 [sender IP]  [Subscriber IP] [Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

            if(isIPEqual(myIP,IP)){
                pbCurrentRecord = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
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
                    if(!isTableUpdated)tableUpdate(pubsubTable,nodeIP,&pbNewInfo);
                }else{
                    //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1,
                    // except for the announced subscribed topic
                    for (int i = 0; i < MAX_TOPICS; i++){
                        pbNewInfo.publishedTopics[i] = -1;
                        if(i == 0){pbNewInfo.subscribedTopics[i] = topic;}
                        else{pbNewInfo.subscribedTopics[i] = -1;}
                    }
                    tableAdd(pubsubTable,nodeIP,&pbNewInfo);
                }
            }

            //Forward the message to the next hop toward the destination IP
            /***routingTableValue = (routingTableEntry*) findRouteToNode(IP);
            if(routingTableValue != nullptr){
                sendMessage(routingTableValue->nextHopIP, messageBuffer);
            }
            break;***/

            //Propagate the subscription message in the network
            encodeMiddlewareMessagePubSub(messageBuffer, sizeof(messageBuffer));
            propagateMessage(messageBuffer,IP);

        case PUBSUB_UNSUBSCRIBE:
            //Message sent when a one unsubscribes to a certain topic
            //13 2 [sender IP] [Unsubscriber IP] [Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

            if(isIPEqual(myIP,IP)) {
                pbCurrentRecord = (PubSubInfo *) tableRead(pubsubTable, nodeIP);
                if (pbCurrentRecord != nullptr) {
                    for (i = 0; i < MAX_TOPICS; i++) {
                        // Save all topics published by the node as-is.
                        pbNewInfo.publishedTopics[i] = pbCurrentRecord->publishedTopics[i];

                        //If the topic is found in the subscribed topics list, unsubscribe
                        if (pbCurrentRecord->subscribedTopics[i] == topic) {
                            // Remove the subscription by shifting all subsequent entries one position forward to overwrite the target
                            for (k = i; k < MAX_TOPICS - 1; k++) {
                                pbNewInfo.subscribedTopics[k] = pbCurrentRecord->subscribedTopics[k + 1];
                            }
                            //TODO put the last position to -1
                            pbNewInfo.subscribedTopics[MAX_TOPICS - 1] = -1;

                            isTableUpdated = true;
                            break;
                        }

                    }
                    if (!isTableUpdated)tableUpdate(pubsubTable, nodeIP, &pbNewInfo);
                } else {
                    //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1
                    for (int i = 0; i < MAX_TOPICS; i++) {
                        pbNewInfo.publishedTopics[i] = -1;
                        pbNewInfo.subscribedTopics[i] = -1;
                    }
                    tableAdd(pubsubTable, nodeIP, &pbNewInfo);
                }
            }
            //Forward the message to the next hop toward the destination IP
            /***routingTableValue = (routingTableEntry*) findRouteToNode(IP);
            if(routingTableValue != nullptr){
                sendMessage(routingTableValue->nextHopIP, messageBuffer);
            }***/

            //Propagate the deleted subscription message in the network
            encodeMiddlewareMessagePubSub(messageBuffer, sizeof(messageBuffer));
            propagateMessage(messageBuffer,IP);

            break;
        case PUBSUB_ADVERTISE:
            // Message used to advertise that a node is publishing a new topic
            //13 3 [sender IP] [Publisher IP] [Published Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

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
                if(!isTableUpdated)tableUpdate(pubsubTable,nodeIP,&pbNewInfo);
            }else{
                //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1,
                // except for the announced subscribed topic
                for (int i = 0; i < MAX_TOPICS; i++){
                    pbNewInfo.subscribedTopics[i] = -1;
                    if(i == 0){pbNewInfo.publishedTopics[i] = topic;}
                    else{pbNewInfo.publishedTopics[i] = -1;}
                }
                tableAdd(pubsubTable,nodeIP,&pbNewInfo);
            }


            //Propagate the adverting message in the network
            encodeMiddlewareMessagePubSub(messageBuffer, sizeof(messageBuffer));
            propagateMessage(messageBuffer,IP);


            break;

        case PUBSUB_UNADVERTISE:
            // Message used to advertise that a node is unpublishing a topic
            //13 4 [sender IP] [Publisher IP] [UnPublished Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

            pbCurrentRecord = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
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
                if(!isTableUpdated)tableUpdate(pubsubTable,nodeIP,&pbNewInfo);
            }else{
                //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1
                for (int i = 0; i < MAX_TOPICS; i++){
                    pbNewInfo.publishedTopics[i] = -1;
                    pbNewInfo.subscribedTopics[i] = -1;
                }
                tableAdd(pubsubTable,nodeIP,&pbNewInfo);
            }

            //Propagate the unadvertised topic message in the network
            encodeMiddlewareMessagePubSub(messageBuffer, sizeof(messageBuffer));
            propagateMessage(messageBuffer,IP);

            break;

        default:
            break;

    }

}

void middlewareInfluenceRoutingPubSub(char* dataMessage){
    int i,j;
    PubSubInfo *myPubSubInfo, *nodePubSubInfo;
    int *nodeIP;
    int topicType;
    bool publisher = false;
    messageParameters parameters;
    routingTableEntry *routingTableValue;

    // Determine which topic is being published
    decodeTopicValue(dataMessage,&topicType);

    // First, read this node's entry in the table to check my published topics
    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    if(myPubSubInfo == nullptr){
        LOG(MESSAGES,ERROR,"ERROR: This node is not present in the Middleware PubSub Table\n");
        return;
    }

    // Verify if this node is the publisher of the topic
    for (i  = 0; i < MAX_TOPICS; i++) {
        if(myPubSubInfo->publishedTopics[i] == topicType){
            publisher = true;
        }
    }
    if(!publisher) return;

    // Go through the table to find which nodes have subscribed to the topic I'm publishing
    for (i = 0; i < pubsubTable->numberOfItems; i++) {
        nodeIP = (int*) tableKey(pubsubTable,i);
        if(nodeIP != nullptr){

            //Discard my entry in the table
            if(isIPEqual(nodeIP,myIP)){
                continue;
            }
            nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);

            if(nodePubSubInfo != nullptr){// Safeguarding against null pointers
                for (j = 0; j < MAX_TOPICS; ++j) {
                    // For each entry in the table, check if any of its subscribed topics match the topic I'm publishing
                    if(nodePubSubInfo->subscribedTopics[i] == topicType ){

                        // Encode the DATA_MESSAGE to send to the subscriber
                        assignIP(parameters.IP1,myIP);
                        assignIP(parameters.IP1,nodeIP);
                        strncpy(parameters.payload,dataMessage, sizeof(parameters.payload));
                        encodeMessage(largeSendBuffer,sizeof(largeSendBuffer),DATA_MESSAGE,parameters);

                        // Send the published topic to the subscriber node
                        routingTableValue = (routingTableEntry*) findRouteToNode(nodeIP);
                        if(routingTableValue != nullptr){
                            sendMessage(routingTableValue->nextHopIP,largeSendBuffer);
                        }else{
                            LOG(NETWORK,ERROR,"ERROR: Unable to find a path to the node in the routing table\n");
                        }
                    }

                }
            }

        }
    }
}

void subscribeToTopic(char topic) {
    int i;
    PubSubInfo *myPubSubInfo, myInitInfo;

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    if(myPubSubInfo != nullptr){ // Update my entry in the table if it already exists
        for (i = 0; i < MAX_TOPICS ; i++) {
            //The topic is already in my list of subscribed topics
            if(myPubSubInfo->subscribedTopics[i] == topic){
                return;
            }
            //Fill the first available position with the subscribed topic
            if(myPubSubInfo->subscribedTopics[i] == -1){
                myPubSubInfo->subscribedTopics[i] = topic;
            }
        }
    }else{ // If my entry doesn't exist, create it
        for (i = 0; i < MAX_TOPICS; i++) {
            myInitInfo.publishedTopics[i] = -1;
            if(i == 0){myInitInfo.subscribedTopics[i] = topic;}
            else{myInitInfo.subscribedTopics[i] = -1;}
        }
        tableAdd(pubsubTable,myIP,&myInitInfo);
    }

    // Announce that i am subscribing to that topic
    encodeMiddlewareMessagePubSub(smallSendBuffer, sizeof(smallSendBuffer));
    propagateMessage(smallSendBuffer,myIP);

}

void advertiseTopic(char topic){
    int i;
    PubSubInfo *myPubSubInfo, myInitInfo;

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    if(myPubSubInfo != nullptr){ // Update my entry in the table if it already exists
        for (i = 0; i < MAX_TOPICS ; i++) {
            //The topic is already in my list of subscribed topics
            if(myPubSubInfo->publishedTopics[i] == topic){
                return;
            }
            //Fill the first available position with the subscribed topic
            if(myPubSubInfo->publishedTopics[i] == -1){
                myPubSubInfo->publishedTopics[i] = topic;
            }
        }
    }else{ // If my entry doesn't exist, create it
        for (i = 0; i < MAX_TOPICS; i++) {
            myInitInfo.subscribedTopics[i] = -1;
            if(i == 0){myInitInfo.publishedTopics[i] = topic;}
            else{myInitInfo.publishedTopics[i] = -1;}
        }
        tableAdd(pubsubTable,myIP,&myInitInfo);
    }

    // Advertise to other nodes that I am publishing this topic
    encodeMiddlewareMessagePubSub(smallSendBuffer, sizeof(smallSendBuffer));
    propagateMessage(smallSendBuffer,myIP);
}

/******************          User Defined functions            **********************/
void decodeTopic(char* dataMessage, void* topicType){
    sscanf(dataMessage,"%*i %i", topicType);
}

void setPubSubInfo(void* av, void* bv){
    PubSubInfo *a = (PubSubInfo*) av;
    PubSubInfo *b = (PubSubInfo*) bv;

    for (int i = 0; i < MAX_TOPICS; i++) {
        a->publishedTopics[i] = b->publishedTopics[i];
        a->subscribedTopics[i] = b->subscribedTopics[i];
    }
}
