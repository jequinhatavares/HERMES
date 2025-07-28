#include "strategy_pubsub.h"

Strategy strategyPubSub = {
        .handleMessage = handleMessageStrategyPubSub,
        .influenceRouting = influenceRoutingStrategyPubSub,
        .onTimer = onTimerStrategyPubSub,
        .onNetworkEvent = onNetworkEventStrategyPubSub,
        .getContext = getContextStrategyPubSub,

};

PubSubContext pubsubContext ={
        .subscribeToTopic = subscribeToTopic,
        .unsubscribeToTopic = unsubscribeToTopic ,
        .advertiseTopic = advertiseTopic,
        .unadvertiseTopic = unadvertiseTopic,
};

/***
 * Middleware Publish Subscribe table
 *
 * mTable[TABLE_MAX_SIZE] - An array where each element is a struct containing two pointers:
 *                         one to the key (used for indexing the metrics table) and another to the value (the corresponding entry).
 *
 * TTable - A struct that holds metadata for the metrics table, including:
 * * * .numberOfItems - The current number of entries in the metrics table.
 * * * .isEqual - A function pointer for comparing table keys (IP addresses).
* * * .table - A pointer to the mTable.
 *
 * childrenTable - A pointer to TTable, used for accessing the children table.
 *
 * valuesPubSub[TABLE_MAX_SIZE] - Preallocated memory for storing the published and subscribed values of each node
 ***/
TableEntry psTable[TABLE_MAX_SIZE];
TableInfo PSTable = {
        .numberOfItems = 0,
        .maxNumberOfItems = TABLE_MAX_SIZE,
        .isEqual = isIPEqual,
        .table = psTable,
        .setKey = setIP,
        .setValue = setPubSubInfo,
};
TableInfo* pubsubTable = &PSTable;


uint8_t nodesPubSub[TABLE_MAX_SIZE][4];

unsigned long lastMiddlewareUpdateTimePubSub = 0;

//Function Pointers Initializers
void (*encodeTopicValue)(char*,size_t,void *) = nullptr;
void (*decodeTopicValue)(char*,int8_t *) = nullptr;

PubSubInfo valuesPubSub[TABLE_MAX_SIZE];


/**
 * initStrategyPubSub
 * Initializes the PubSub strategy by setting callback functions and setting up the PubSubTable
 *
 * @param setValueFunction - Callback function for setting table values
 * @param encodeTopicFunction - Callback for encoding topic data
 * @param decodeTopicFunction - Callback for decoding topic data
 * @return void
 */
void initStrategyPubSub(void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,int8_t *) ){
    //Initialize the pubsubTable
    tableInit(pubsubTable, nodesPubSub, valuesPubSub, sizeof(uint8_t [4]), sizeof(PubSubInfo));

    //Initialize function to encode/decode the topics value
    encodeTopicValue = encodeTopicFunction;
    decodeTopicValue = decodeTopicFunction;
}

/**
 * rewriteSenderIPPubSub
 * Updates sender IP in PubSub messages during network propagation
 *
 * @param messageBuffer - Original message buffer
 * @param writeBuffer - Buffer for the updated message
 * @param writeBufferSize - Size of write buffer
 * @param type - PubSub message type
 * @return void
 */
void rewriteSenderIPPubSub(char* messageBuffer, char* writeBuffer, size_t writeBufferSize, PubSubMessageType type){
    MessageType globalMessageType;
    PubSubMessageType typePubSub;
    uint8_t nodeIP[4],IP[4],topic1;
    char updatedMessage[255];

    if (type != PUBSUB_NODE_UPDATE){ // type == PUBSUB_PUBLISH, PUBSUB_SUBSCRIBE, PUBSUB_UNSUBSCRIBE, PUBSUB_ADVERTISE, PUBSUB_UNADVERTISE,
        // If the encoded message already contains topic information, it means this is a propagation of an already encoded message.
        // In this case, only the sender address needs to be updated before further propagation.
        if( sscanf(messageBuffer,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %hhd",&globalMessageType,&typePubSub,&IP[0],&IP[1],&IP[2],&IP[3]
                ,&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3], &topic1) == 11 ){

            snprintf(writeBuffer,writeBufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %hhd",globalMessageType,typePubSub,myIP[0],myIP[1],myIP[2],myIP[3]
                    ,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3], topic1);

        }
    }else{

        // Parse the beginning of the message
        sscanf(messageBuffer, "%d %d %*hhu.%*hhu.%*hhu.%*hhu %hhu.%hhu.%hhu.%hhu",
               &globalMessageType, &typePubSub,
               &nodeIP[0], &nodeIP[1], &nodeIP[2], &nodeIP[3]);

        // Find the part after the IPs (the '|' and beyond)
        char* restOfMessage = strchr(messageBuffer, '|');
        if (restOfMessage == NULL) return;  // malformed messageBuffer

        // Compose the updated messageBuffer
        snprintf(updatedMessage, sizeof(updatedMessage), "%d %d %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %s",
                 globalMessageType, typePubSub,
                 myIP[0],myIP[1],myIP[2],myIP[3],
                 nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3],
                 restOfMessage);

        // Copy it back
        strncpy(writeBuffer, updatedMessage, writeBufferSize);/******/

    }

}

/**
 * encodeMessageStrategyPubSub
 * Constructs PubSub messages based on message type
 *
 * @param messageBuffer - buffer where the messages is going to be stored
 * @param bufferSize - Size of the buffer
 * @param typePubSub - Type of PubSub message to encode
 * @param topic - Topic value used for message types involving topic subscription or advertisement changes
 * @return void
 */
void encodeMessageStrategyPubSub(char* messageBuffer, size_t bufferSize, int typePubSub, int8_t topic) {
    PubSubInfo *nodePubSubInfo;
    int offset = 0,i,j;
    uint8_t *nodeIP;

    // These messages encode the node's own publish/subscribe information
    switch (typePubSub) {

        case PUBSUB_SUBSCRIBE:
            //Message sent when a one subscribes to a certain topic
            //13 0 [sender IP] [Subscriber IP] [Topic]
            snprintf(messageBuffer,bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %i",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE,
                     myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
            break;
        case PUBSUB_UNSUBSCRIBE:
            //Message sent when a one unsubscribes to a certain topic
            //13 1 [sender IP] [Unsubscriber IP] [Topic]
            snprintf(messageBuffer,bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %i",MIDDLEWARE_MESSAGE,PUBSUB_UNSUBSCRIBE,
                     myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
            break;
        case PUBSUB_ADVERTISE:
            // Message used to advertise that the node is publishing a new topic
            //13 2 [sender IP] [Publisher IP] [Published Topic]
            snprintf(messageBuffer,bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %i",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE,
                     myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
            break;
        case PUBSUB_UNADVERTISE:
            // Message used to advertise that the node is unpublishing a topic
            //13 3 [sender IP] [Publisher IP] [UnPublished Topic]
            snprintf(messageBuffer,bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu %i",MIDDLEWARE_MESSAGE,PUBSUB_UNADVERTISE,
                     myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
            break;

        case PUBSUB_NODE_UPDATE:
            // Message used to advertise all publish-subscribe information of the node
            //13 4 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List] //maxsize:48+1
            nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);

            if (nodePubSubInfo != nullptr) {
                offset = snprintf(messageBuffer, bufferSize, "%i %i %hhu.%hhu.%hhu.%hhu %hhu.%hhu.%hhu.%hhu | ",
                                      MIDDLEWARE_MESSAGE, PUBSUB_NODE_UPDATE,
                                      myIP[0], myIP[1], myIP[2], myIP[3],
                                      myIP[0], myIP[1], myIP[2], myIP[3]);

                // Append published topics
                for (i = 0; i < MAX_TOPICS; i++) {
                    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%i ", nodePubSubInfo->publishedTopics[i]);
                }

                // Append subscribed topics
                for (i = 0; i < MAX_TOPICS; i++) {
                    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%i ", nodePubSubInfo->subscribedTopics[i]);
                }
            }

            break;

        case PUBSUB_TABLE_UPDATE:
            //13 5 [sender IP] |[node IP] [Published Topic List] [Subscribed Topics List] |[node IP] [Published Topic List] [Subscribed Topics List]...
            offset = snprintf(messageBuffer,bufferSize,"%i %i %hhu.%hhu.%hhu.%hhu |",MIDDLEWARE_MESSAGE,PUBSUB_TABLE_UPDATE,
                     myIP[0],myIP[1],myIP[2],myIP[3]);
            for (i = 0; i < pubsubTable->numberOfItems; i++) {
                nodeIP = (uint8_t *) tableKey(pubsubTable,i);
                PubSubInfo* entry = (PubSubInfo*) tableRead(pubsubTable, nodeIP);
                if (entry != nullptr) {
                    // Append node IP
                    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%hhu.%hhu.%hhu.%hhu ",
                                       nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3]);

                    // Append published topics
                    for(j = 0; j < MAX_TOPICS; j++) {
                        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%i ", entry->publishedTopics[j]);
                    }

                    // Append subscribed topics
                    for(j = 0; j < MAX_TOPICS; j++) {
                        //Special encoding condition for not having space in the end of the message
                        if(i == pubsubTable->numberOfItems-1 && j == MAX_TOPICS-1){
                            offset += snprintf(messageBuffer + offset, bufferSize - offset, "%i", entry->subscribedTopics[j]);
                        }else{
                            offset += snprintf(messageBuffer + offset, bufferSize - offset, "%i ", entry->subscribedTopics[j]);
                        }
                    }

                    if(i != pubsubTable->numberOfItems-1){
                        // Add another separator to mark end of this node’s entry
                        offset += snprintf(messageBuffer + offset, bufferSize - offset, "|");
                    }

                }
            }
            break;
    }
}

/**
 * handleMessageStrategyPubSub
 * Processes incoming PubSub message and depending on the message it can: add/remove/update values from the PubSub Table
 *
 * @param messageBuffer - Received message buffer
 * @param bufferSize - Size of received buffer
 * @return void
 */
void handleMessageStrategyPubSub(char* messageBuffer, size_t bufferSize) {
    char infoPubSub[100];
    uint8_t IP[4],nodeIP[4];
    int topic,i,k,count=0, charsRead = 0;
    PubSubMessageType type;
    PubSubInfo pbNewInfo,*pbCurrentRecord;
    bool isTableUpdated = false;
    RoutingTableEntry *routingTableValue;
    char* token, *spaceToken,*entry;
    char* nodeIPPart, *topicsPart;
    char *saveptr1, *saveptr2;

    //MESSAGE_TYPE  PUBSUB_TYPE  [sender/destination IP]  [nodeIP]  topic
    sscanf(messageBuffer,"%*i %i %hhu.%hhu.%hhu.%hhu %n",&type,&IP[0],&IP[1],&IP[2],&IP[3],&charsRead);

    // Copy the rest of the string manually
    strncpy(infoPubSub, messageBuffer + charsRead, sizeof(infoPubSub) - 1);

    //printf("InfoPubSub: %s\n",infoPubSub);

    switch (type) {

        case PUBSUB_SUBSCRIBE:
            //Message sent when a one node subscribes to a certain topic
            //13 1 [sender IP] [Subscriber IP] [Topic]
            sscanf(infoPubSub,"%hhu.%hhu.%hhu.%hhu %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

            if(!isIPEqual(myIP,IP)){
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
                    for (i = 0; i < MAX_TOPICS; i++){
                        pbNewInfo.publishedTopics[i] = -1;
                        if(i == 0){pbNewInfo.subscribedTopics[i] = topic;}
                        else{pbNewInfo.subscribedTopics[i] = -1;}
                    }
                    tableAdd(pubsubTable,nodeIP,&pbNewInfo);
                }
            }

            //Forward the message to the next hop toward the destination IP
            //routingTableValue = (RoutingTableEntry*) findRouteToNode(IP);
            //if(routingTableValue != nullptr){
                //sendMessage(routingTableValue->nextHopIP, messageBuffer);
            //}


            //Propagate the subscription message in the network
            rewriteSenderIPPubSub(messageBuffer,smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_SUBSCRIBE);
            propagateMessage(smallSendBuffer,IP);
            break;

        case PUBSUB_UNSUBSCRIBE:
            //Message sent when a one unsubscribes to a certain topic
            //13 2 [sender IP] [Unsubscriber IP] [Topic]
            sscanf(infoPubSub,"%hhu.%hhu.%hhu.%hhu %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

            if(!isIPEqual(myIP,IP)) {
                pbCurrentRecord = (PubSubInfo *) tableRead(pubsubTable, nodeIP);
                if (pbCurrentRecord != nullptr) {
                    for (i = 0; i < MAX_TOPICS; i++) {
                        // Save all topics published by the node as-is.
                        pbNewInfo.publishedTopics[i] = pbCurrentRecord->publishedTopics[i];

                        //If the topic is found in the subscribed topics list, unsubscribe
                        if (pbCurrentRecord->subscribedTopics[i] == topic) {// Remove the subscription by shifting all subsequent entries one position forward to overwrite the target
                            for (k = i; k < MAX_TOPICS - 1; k++) {
                                pbNewInfo.subscribedTopics[k] = pbCurrentRecord->subscribedTopics[k + 1];
                            }
                            pbNewInfo.subscribedTopics[MAX_TOPICS - 1] = -1;

                            break;
                        }

                    }
                    tableUpdate(pubsubTable, nodeIP, &pbNewInfo);
                } else {
                    //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1
                    for (i = 0; i < MAX_TOPICS; i++) {
                        pbNewInfo.publishedTopics[i] = -1;
                        pbNewInfo.subscribedTopics[i] = -1;
                    }
                    tableAdd(pubsubTable, nodeIP, &pbNewInfo);
                }
            }
            //Forward the message to the next hop toward the destination IP
            //routingTableValue = (RoutingTableEntry*) findRouteToNode(IP);
            //if(routingTableValue != nullptr){
                //sendMessage(routingTableValue->nextHopIP, messageBuffer);
            //}

            //Propagate the deleted subscription message in the network
            rewriteSenderIPPubSub(messageBuffer,smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_UNSUBSCRIBE);
            propagateMessage(messageBuffer,IP);

            break;
        case PUBSUB_ADVERTISE:
            // Message used to advertise that a node is publishing a new topic
            //13 3 [sender IP] [Publisher IP] [Published Topic]
            sscanf(infoPubSub,"%hhu.%hhu.%hhu.%hhu %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);
            pbCurrentRecord = (PubSubInfo *) tableRead(pubsubTable, nodeIP);
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
                for (i = 0; i < MAX_TOPICS; i++){
                    pbNewInfo.subscribedTopics[i] = -1;
                    if(i == 0){pbNewInfo.publishedTopics[i] = topic;}
                    else{pbNewInfo.publishedTopics[i] = -1;}
                }
                tableAdd(pubsubTable,nodeIP,&pbNewInfo);
            }

            //Propagate the adverting message in the network
            rewriteSenderIPPubSub(messageBuffer,smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_ADVERTISE);
            propagateMessage(smallSendBuffer,IP);

            break;

        case PUBSUB_UNADVERTISE:
            // Message used to advertise that a node is unpublishing a topic
            //13 4 [sender IP] [Publisher IP] [UnPublished Topic]
            sscanf(infoPubSub,"%hhu.%hhu.%hhu.%hhu %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

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

                        break;
                    }

                }
                tableUpdate(pubsubTable,nodeIP,&pbNewInfo);
            }else{
                //If the node does not exist in the table, add it with all published and subscribed topics initialized to -1
                for (i = 0; i < MAX_TOPICS; i++){
                    pbNewInfo.publishedTopics[i] = -1;
                    pbNewInfo.subscribedTopics[i] = -1;
                }
                tableAdd(pubsubTable,nodeIP,&pbNewInfo);
            }

            //Propagate the unadvertised topic message in the network
            rewriteSenderIPPubSub(messageBuffer,smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_UNADVERTISE);
            propagateMessage(smallSendBuffer,IP);

            break;

        case PUBSUB_NODE_UPDATE:
            //Max size: 40 + nTopics * 2 +(nTopics-1)*2
            // Message used to advertise all publish-subscribe information of the node
            //13 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List]
            nodeIPPart = strtok(infoPubSub, "|");  // First part before the '|'
            topicsPart = strtok(NULL, "|");           // Second part after the '|'

            if (nodeIPPart != NULL && topicsPart != NULL) {
                // Parse node IP from nodeIPPart (you may need to skip the message type and sender IP first)
                sscanf(nodeIPPart, "%hhu.%hhu.%hhu.%hhu",&nodeIP[0], &nodeIP[1], &nodeIP[2], &nodeIP[3]);

                // Parse the topic lists (published and subscribed)
                count = 0;
                spaceToken = strtok(topicsPart, " ");
                while (spaceToken != NULL && count < 2 * MAX_TOPICS) {
                    if (count < MAX_TOPICS) {
                        pbNewInfo.publishedTopics[count] = atoi(spaceToken);
                    } else {
                        pbNewInfo.subscribedTopics[count - MAX_TOPICS] = atoi(spaceToken);
                    }
                    count++;
                    spaceToken = strtok(NULL, " ");
                }
            }

            pbCurrentRecord = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
            if(pbCurrentRecord != nullptr){
                tableUpdate(pubsubTable,nodeIP,&pbNewInfo);
            }else{
                tableAdd(pubsubTable,nodeIP,&pbNewInfo);
            }

            //Propagate the unadvertised topic message in the network
            rewriteSenderIPPubSub(messageBuffer,largeSendBuffer, sizeof(largeSendBuffer),PUBSUB_NODE_UPDATE);
            propagateMessage(largeSendBuffer,IP);

            break;/******/

        case PUBSUB_TABLE_UPDATE:
            //Buffer max size = 22 + 30*nNodes
            //13 6 [sender IP] |[node IP] [Published Topic List] [Subscribed Topics List] |[node IP] [Published Topic List] [Subscribed Topics List]...
            entry = strtok_r(infoPubSub, "|", &saveptr1);

            while (entry != nullptr) {
                // Trim leading spaces
                while (*entry == ' ') entry++;

                // Now parse this entry
                token = strtok_r(entry, " ", &saveptr2);

                if (token == NULL) {
                    entry = strtok_r(NULL, "|", &saveptr1);
                    continue;
                }

                // Parse node IP
                sscanf(token, "%hhu.%hhu.%hhu.%hhu", &nodeIP[0], &nodeIP[1], &nodeIP[2], &nodeIP[3]);
                //printf("nodeIP: %d.%d.%d.%d\n", nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3]);

                // Parse topic values
                count = 0;
                while ((token = strtok_r(NULL, " ", &saveptr2)) != NULL && count < 2 * MAX_TOPICS) {
                    if (count < MAX_TOPICS) {
                        pbNewInfo.publishedTopics[count] = atoi(token);
                        //printf("Parsed Topic:%i\n",pbNewInfo.publishedTopics[count]);
                    } else {
                        pbNewInfo.subscribedTopics[count - MAX_TOPICS] = atoi(token);
                        //printf("Parsed Topic:%i\n",pbNewInfo.subscribedTopics[count- MAX_TOPICS]);
                    }
                    count++;
                }

                // Update/Add pubsub table except when its related to this node
                if(!isIPEqual(nodeIP,myIP)){
                    PubSubInfo* current = (PubSubInfo*) tableRead(pubsubTable, nodeIP);
                    if (current != nullptr) {
                        tableUpdate(pubsubTable, nodeIP, &pbNewInfo);
                    } else {
                        tableAdd(pubsubTable, nodeIP, &pbNewInfo);
                    }
                }

                // Next entry
                entry = strtok_r(NULL, "|", &saveptr1);
            }

        default:
            break;

    }
}

/**
 * onNetworkEventStrategyPubSub
 * Triggers the strategy logic to respond to a specific network event.
 * If a child node connects to this node, it sends the full pub/sub table to the child.
 * When the node itself joins the network, it sends its publishing and subscribing topic information to the parent node.
 *
 * @param networkEvent - Type of network event (join/connect/disconnect)
 * @param involvedIP - IP address of relevant network node
 * @return void
 */
void onNetworkEventStrategyPubSub(int networkEvent, uint8_t involvedIP[4]){
    switch (networkEvent) {
        case NETEVENT_JOINED_NETWORK:
            encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_NODE_UPDATE,-1);
            sendMessage(involvedIP,smallSendBuffer);
            LOG(MESSAGES,INFO,"Sending [MIDDLEWARE/PUBSUB_NODE_INFO] message: \"%s\" to: %hhu.%hhu.%hhu.%hhu\n",smallSendBuffer,involvedIP[0],involvedIP[1],involvedIP[2],involvedIP[3]);

            break;
        case NETEVENT_CHILD_CONNECTED:
            encodeMessageStrategyPubSub(largeSendBuffer, sizeof(largeSendBuffer),PUBSUB_TABLE_UPDATE,-1);
            sendMessage(involvedIP,largeSendBuffer);
            LOG(MESSAGES,INFO,"Sending [MIDDLEWARE/PUBSUB_TABLE_INFO] message: \"%s\" to: %hhu.%hhu.%hhu.%hhu\n",largeSendBuffer,involvedIP[0],involvedIP[1],involvedIP[2],involvedIP[3]);
            break;
        case NETEVENT_CHILD_DISCONNECTED:
            break;
        default:
            break;
    }
}

/**
 * influenceRoutingStrategyPubSub
 * Influences routing by forwarding the message containing the topic to all subscribers.
 *
 * @param dataMessage - message containing (or not) the topic data
 * @return void
 */
void influenceRoutingStrategyPubSub(char* dataMessage){
    int i,j;
    PubSubInfo *myPubSubInfo, *nodePubSubInfo;
    uint8_t *nextHopIP, *nodeIP;
    int8_t topicType;
    bool publisher = false;
    int nChars = 0;
    char payload[100];

    sscanf(dataMessage, "%*d %*d.%*d.%*d.%*d %*d.%*d.%*d.%*d %n",&nChars);

    // Copy the rest of the string manually
    strncpy(payload, dataMessage + nChars, sizeof(payload) - 1);

    LOG(MIDDLEWARE,DEBUG,"%s\n",payload);

    // Determine which topic is being published
    decodeTopicValue(payload,&topicType);

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
    if(!publisher){
        LOG(MIDDLEWARE,DEBUG,"This node is not publisher of the topic: %i\n",topicType);
        return;
    }else{
        LOG(MIDDLEWARE,DEBUG,"This node publishes the topic: %i\n",topicType);
    }

    // Go through the table to find which nodes have subscribed to the topic I'm publishing
    for (i = 0; i < pubsubTable->numberOfItems; i++) {
        nodeIP = (uint8_t *) tableKey(pubsubTable,i);
        if(nodeIP != nullptr){
            //Discard my entry in the table
            if(isIPEqual(nodeIP,myIP)){
                continue;
            }
            nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,nodeIP);
            if(nodePubSubInfo != nullptr){// Safeguarding against null pointers
                // For each entry in the table, check if any of its subscribed topics match the topic I'm publishing
                if(containsTopic(nodePubSubInfo->subscribedTopics,topicType)){

                    // Encode the DATA_MESSAGE to send to the subscriber
                    encodeDataMessage(largeSendBuffer,sizeof(largeSendBuffer),payload,myIP,nodeIP);

                    // Send the published topic to the subscriber node
                    nextHopIP = (uint8_t *) findRouteToNode(nodeIP);
                    if(nextHopIP != nullptr){
                        sendMessage(nextHopIP,largeSendBuffer);
                        LOG(MIDDLEWARE,ERROR,"Sending [DATA] message: %s to %hhu.%hhu.%hhu.%hhu\n",largeSendBuffer,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
                    }else{
                        LOG(MIDDLEWARE,ERROR,"ERROR: Unable to find a path to the node in the routing table\n");
                    }
                }
            }

        }
    }
}

/**
 * onTimerStrategyPubSub
 * Handles periodic PubSub maintenance tasks (currently unused)
 *
 * @return void
 */
void onTimerStrategyPubSub(){
    unsigned long currentTime = getCurrentTime();
    //Periodically send this node's metric to all other nodes in the network
    if( (currentTime - lastMiddlewareUpdateTimePubSub) >= MIDDLEWARE_UPDATE_INTERVAL ) {
        /***encodeMessageStrategyPubSub(largeSendBuffer, sizeof(largeSendBuffer), PUBSUB_NODE_UPDATE);
        propagateMessage(largeSendBuffer, myIP);
        LOG(MIDDLEWARE,DEBUG,"Sending periodic [MIDDLEWARE/PUBSUB_NODE_INFO] Message: %s\n",largeSendBuffer);
        lastMiddlewareUpdateTimePubSub = currentTime;***/
    }
}

/**
 * subscribeToTopic
 * Registers a subscription to the specified topic on the local node and propagates
 * the subscription information to the rest of the network.
 *
 * @param subTopic - Identifier of the topic to subscribe to
 * @return void
 */
void subscribeToTopic(int8_t subTopic) {
    int i;
    PubSubInfo *myPubSubInfo, myInitInfo;

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
   if(myPubSubInfo != nullptr){ // Update my entry in the table if it already exists
        for (i = 0; i < MAX_TOPICS ; i++) {
            //The topic is already in my list of subscribed topics
            if(myPubSubInfo->subscribedTopics[i] == subTopic){
                return;
            }
            //Fill the first available position with the subscribed topic
            if(myPubSubInfo->subscribedTopics[i] == -1){
                myPubSubInfo->subscribedTopics[i] = subTopic;
                return;
            }
        }
    }else{ // If my entry doesn't exist, create it
        for (i = 0; i < MAX_TOPICS; i++) {
            myInitInfo.publishedTopics[i] = -1;
            if(i == 0){myInitInfo.subscribedTopics[i] = subTopic;}
            else{myInitInfo.subscribedTopics[i] = -1;}
        }
        tableAdd(pubsubTable,myIP,&myInitInfo);
    }

    // Announce that i am subscribing to that topic
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_SUBSCRIBE,subTopic);
    propagateMessage(smallSendBuffer,myIP);

}


/**
 * unsubscribeFromTopic
 * Removes the local subscription to the specified topic and propagates
 * the unsubscription information to the rest of the network.
 *
 * @param subTopic - Identifier of the topic to unsubscribe from
 * @return void
 */
void unsubscribeToTopic(int8_t subTopic){
    int i,k;
    PubSubInfo *myPubSubInfo,myInitInfo;

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    if(myPubSubInfo != nullptr){ // Update my entry in the table if it already exists
        for (i = 0; i < MAX_TOPICS ; i++) {
            //If the topic is found in the subscribed topics list, unsubscribe
            if (myPubSubInfo->subscribedTopics[i] == subTopic) {
                // Remove the subscription by shifting all subsequent entries one position forward to overwrite the target
                for (k = i; k < MAX_TOPICS - 1; k++) {
                    myPubSubInfo->subscribedTopics[k] = myPubSubInfo->subscribedTopics[k + 1];
                }
                //TODO put the last position to -1
                myPubSubInfo->subscribedTopics[MAX_TOPICS - 1] = -1;
                break;
            }
        }
    }else{ // If my entry doesn't exist, create it
        for (i = 0; i < MAX_TOPICS; i++) {
            myInitInfo.publishedTopics[i] = -1;
            myInitInfo.subscribedTopics[i] = -1;
        }
        tableAdd(pubsubTable,myIP,&myInitInfo);
    }

    // Announce that i am unsubscribing to that topic
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_UNSUBSCRIBE,subTopic);
    propagateMessage(smallSendBuffer,myIP);
}

/**
 * advertiseTopic
 * Announces that this node is publishing a specific topic, making this
 * information available to other nodes in the network.
 *
 * @param pubTopic - Identifier of the topic being advertised for publication
 * @return void
 */
void advertiseTopic(int8_t pubTopic){
    int i;
    PubSubInfo *myPubSubInfo, myInitInfo;

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    if(myPubSubInfo != nullptr){ // Update my entry in the table if it already exists
        for (i = 0; i < MAX_TOPICS ; i++) {
            //The topic is already in my list of subscribed topics
            if(myPubSubInfo->publishedTopics[i] == pubTopic){
                return;
            }
            //Fill the first available position with the subscribed topic
            if(myPubSubInfo->publishedTopics[i] == -1){
                myPubSubInfo->publishedTopics[i] = pubTopic;
                return;
            }
        }
    }else{ // If my entry doesn't exist, create it
        for (i = 0; i < MAX_TOPICS; i++) {
            myInitInfo.subscribedTopics[i] = -1;
            if(i == 0){myInitInfo.publishedTopics[i] = pubTopic;}
            else{myInitInfo.publishedTopics[i] = -1;}
        }
        tableAdd(pubsubTable,myIP,&myInitInfo);
    }

    // Advertise to other nodes that I am publishing this topic
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_ADVERTISE,pubTopic);
    propagateMessage(smallSendBuffer,myIP);
}

/**
 * unadvertiseTopic
 * Informs the network that this node is no longer publishing the specified topic,
 * removing it from the advertised publication list.
 *
 * @param pubTopic - Identifier of the topic that is no longer being published
 * @return void
 */
void unadvertiseTopic(int8_t pubTopic){
    int i,k;
    PubSubInfo *myPubSubInfo, myInitInfo;

    myPubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
    if(myPubSubInfo != nullptr){ // Update my entry in the table if it already exists
        for (i = 0; i < MAX_TOPICS ; i++) {
            //If the topic is found in the subscribed topics list, unsubscribe
            if (myPubSubInfo->publishedTopics[i] == pubTopic) {
                // Remove the subscription by shifting all subsequent entries one position forward to overwrite the target
                for (k = i; k < MAX_TOPICS - 1; k++) {
                    myPubSubInfo->publishedTopics[k] = myPubSubInfo->publishedTopics[k + 1];
                }
                myPubSubInfo->publishedTopics[MAX_TOPICS - 1] = -1;
                break;
            }
        }
    }else{ // If my entry doesn't exist, create it
        for (i = 0; i < MAX_TOPICS; i++) {
            myInitInfo.subscribedTopics[i] = -1;
            if(i == 0){myInitInfo.publishedTopics[i] = pubTopic;}
            else{myInitInfo.publishedTopics[i] = -1;}
        }
        tableAdd(pubsubTable,myIP,&myInitInfo);
    }

    // Advertise to other nodes that I am publishing this topic
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_UNADVERTISE,pubTopic);
    propagateMessage(smallSendBuffer,myIP);
}

/**
 * printPubSubStruct
 * Function to print the PubSub Table
 *
 * @param Table - pubsubTable entry
 * @return void
 **/
void printPubSubStruct(TableEntry* Table){
    LOG(MIDDLEWARE,INFO,"Node[%hhu.%hhu.%hhu.%hhu] → ",
        ((int8_t *)Table->key)[0],((int8_t *)Table->key)[1],((int8_t *)Table->key)[2],((int8_t *)Table->key)[3]);

    LOG(MIDDLEWARE,INFO,"(Publishes: ");
    for(int i = 0; i < MAX_TOPICS; ++i) {
        LOG(MIDDLEWARE,INFO,"%d | ",
            ((PubSubInfo *)Table->value)->publishedTopics[i]);
    }
    LOG(MIDDLEWARE,INFO,") ");

    LOG(MIDDLEWARE,INFO,"(Subscriptions: ");
    for(int i = 0; i < MAX_TOPICS; ++i) {
        LOG(MIDDLEWARE,INFO,"%d | ",
            ((PubSubInfo *)Table->value)->subscribedTopics[i]);
    }
    LOG(MIDDLEWARE,INFO,")\n");

}

/**
 * printPubSubTableHeader
 * Prints header of PubSub table
 *
 * @return void
 **/
void printPubSubTableHeader(){
    LOG(MIDDLEWARE,INFO,"**********************| Middleware Strategy Pub/Sub Table |**********************\n");
}

/**
 * containsTopic
 * Checks if topic exists in provided subscription/publication list
 *
 * @param list - Array of topics to search
 * @param topic - Topic identifier to find
 * @return true if topic found, false otherwise
 */
bool containsTopic(const int8_t * list, int8_t topic){
    for (int i = 0; i < MAX_TOPICS; i++) {
        if(list[i] == topic){
            return true;
        }
    }
    return false;
}

void* getContextStrategyPubSub(){
    return &pubsubContext;
}

/******************          User Defined functions            **********************/
void decodeTopic(char* dataMessage, int8_t * topicType){
    TopicTypes type;
    char topicString[20];
    sscanf(dataMessage,"%s", topicString);
    if(strcmp(topicString,"TEMPERATURE") == 0){
        *topicType = TEMPERATURE;
    }else if(strcmp(topicString,"HUMIDITY") == 0){
        *topicType = HUMIDITY;
    }else if(strcmp(topicString,"CAMERA") == 0){
        *topicType = CAMERA;
    }
}
void encodeTopic(char*DataMessage,size_t messageSize, void* topic) {

}
void setPubSubInfo(void* av, void* bv){
    PubSubInfo *a = (PubSubInfo*) av;
    PubSubInfo *b = (PubSubInfo*) bv;

    for (int i = 0; i < MAX_TOPICS; i++) {
        a->publishedTopics[i] = b->publishedTopics[i];
        a->subscribedTopics[i] = b->subscribedTopics[i];
    }
}
