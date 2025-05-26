#include "strategy_pubsub.h"

Strategy strategyPubSub = {
        .handleMessage = handleMessageStrategyPubSub,
        .encodeMessage = encodeMessageStrategyPubSub,
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


int nodesPubSub[TableMaxSize][4];

unsigned long lastMiddlewareUpdateTimePubSub = 0;

//Function Pointers Initializers
void (*encodeTopicValue)(char*,size_t,void *) = nullptr;
void (*decodeTopicValue)(char*,int8_t *) = nullptr;

PubSubInfo valuesPubSub[TableMaxSize];

int8_t topic;

void initStrategyPubSub(void (*setValueFunction)(void*,void *),void (*encodeTopicFunction)(char*,size_t,void *),void (*decodeTopicFunction)(char*,int8_t *) ){
    pubsubTable->setValue = setValueFunction;
    //Initialize the pubsubTable
    tableInit(pubsubTable, nodesPubSub, valuesPubSub, sizeof(int[4]), sizeof(PubSubInfo));

    //Initialize function to encode/decode the topics value
    encodeTopicValue = encodeTopicFunction;
    decodeTopicValue = decodeTopicFunction;
}

void rewriteSenderIPPubSub(char* messageBuffer, size_t bufferSize, PubSubMessageType type){
    messageType globalMessageType;
    PubSubMessageType typePubSub;
    int nodeIP[4],IP[4],topic;
    char updatedMessage[256];

    if (type != PUBSUB_NODE_UPDATE){
        // If the encoded message already contains topic information, it means this is a propagation of an already encoded message.
        // In this case, only the sender address needs to be updated before further propagation.
        if( sscanf(messageBuffer,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",&globalMessageType,&typePubSub,&IP[0],&IP[1],&IP[2],&IP[3]
                ,&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3], &topic) == 11 ){

            snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",globalMessageType,typePubSub,myIP[0],myIP[1],myIP[2],myIP[3]
                    ,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3], topic);

        }
    }else{

        // Parse the beginning of the message
        sscanf(messageBuffer, "%d %d %*d.%*d.%*d.%*d %d.%d.%d.%d",
               &globalMessageType, &typePubSub,
               &nodeIP[0], &nodeIP[1], &nodeIP[2], &nodeIP[3]);

        // Find the part after the IPs (the '|' and beyond)
        char* restOfMessage = strchr(messageBuffer, '|');
        if (restOfMessage == NULL) return;  // malformed messageBuffer

        // Compose the updated messageBuffer
        snprintf(updatedMessage, sizeof(updatedMessage), "%d %d %d.%d.%d.%d %d.%d.%d.%d %s",
                 globalMessageType, typePubSub,
                 myIP[0],myIP[1],myIP[2],myIP[3],
                 nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3],
                 restOfMessage);

        // Copy it back
        strncpy(messageBuffer, updatedMessage, bufferSize);/******/

    }

}

void encodeMessageStrategyPubSub(char* messageBuffer, size_t bufferSize, int typePubSub) {
    PubSubInfo *nodePubSubInfo;
    int offset = 0,*nodeIP,i,j;

    // These messages encode the node's own publish/subscribe information
    switch (typePubSub) {
        case PUBSUB_PUBLISH:
            break;
        case PUBSUB_SUBSCRIBE:
            //Message sent when a one subscribes to a certain topic
            //13 1 [sender IP] [Subscriber IP] [Topic]
            snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",MIDDLEWARE_MESSAGE,PUBSUB_SUBSCRIBE,
                     myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
            break;
        case PUBSUB_UNSUBSCRIBE:
            //Message sent when a one unsubscribes to a certain topic
            //13 2 [sender IP] [Unsubscriber IP] [Topic]
            snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",MIDDLEWARE_MESSAGE,PUBSUB_UNSUBSCRIBE,
                     myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
            break;
        case PUBSUB_ADVERTISE:
            // Message used to advertise that the node is publishing a new topic
            //13 3 [sender IP] [Publisher IP] [Published Topic]
            snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",MIDDLEWARE_MESSAGE,PUBSUB_ADVERTISE,
                     myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
            break;
        case PUBSUB_UNADVERTISE:
            // Message used to advertise that the node is unpublishing a topic
            //13 4 [sender IP] [Publisher IP] [UnPublished Topic]
            snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i %i",MIDDLEWARE_MESSAGE,PUBSUB_UNADVERTISE,
                     myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3],topic);
            break;

        case PUBSUB_NODE_UPDATE:
            // Message used to advertise all publish-subscribe information of the node
            //13 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List] //maxsize:48+1
            nodePubSubInfo = (PubSubInfo*) tableRead(pubsubTable,myIP);
            /***if(nodePubSubInfo != nullptr){
                snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i %i.%i.%i.%i | ",MIDDLEWARE_MESSAGE,PUBSUB_NODE_UPDATE,
                         myIP[0],myIP[1],myIP[2],myIP[3],myIP[0],myIP[1],myIP[2],myIP[3]);

                for (int i = 0; i < MAX_TOPICS; i++) {
                    snprintf(tmpMessage1, sizeof(tmpMessage1),"%i ",nodePubSubInfo->publishedTopics[i]);
                    snprintf(tmpMessage2, sizeof(tmpMessage1),"%i ",nodePubSubInfo->subscribedTopics[i]);

                    strcat(tmpMessage3,tmpMessage1);
                    strcat(tmpMessage4,tmpMessage2);
                }
                strcat(messageBuffer,tmpMessage3);
                strcat(messageBuffer,tmpMessage4);
            }***/
            if (nodePubSubInfo != nullptr) {
                offset = snprintf(messageBuffer, bufferSize, "%i %i %i.%i.%i.%i %i.%i.%i.%i | ",
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
            //13 6 [sender IP] |[node IP] [Published Topic List] [Subscribed Topics List] |[node IP] [Published Topic List] [Subscribed Topics List]...
            offset = snprintf(messageBuffer,bufferSize,"%i %i %i.%i.%i.%i |",MIDDLEWARE_MESSAGE,PUBSUB_TABLE_UPDATE,
                     myIP[0],myIP[1],myIP[2],myIP[3]);
            for (i = 0; i < pubsubTable->numberOfItems; i++) {
                nodeIP = (int*) tableKey(pubsubTable,i);
                PubSubInfo* entry = (PubSubInfo*) tableRead(pubsubTable, nodeIP);
                if (entry != nullptr) {
                    // Append node IP
                    offset += snprintf(messageBuffer + offset, bufferSize - offset, "%i.%i.%i.%i ",
                                       nodeIP[0], nodeIP[1], nodeIP[2], nodeIP[3]);

                    // Append published topics
                    for(j = 0; j < MAX_TOPICS; j++) {
                        offset += snprintf(messageBuffer + offset, bufferSize - offset, "%i ", entry->publishedTopics[j]);
                    }

                    // Append subscribed topics
                    for(j = 0; j < MAX_TOPICS; j++) {
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

void handleMessageStrategyPubSub(char* messageBuffer, size_t bufferSize) {
    char infoPubSub[100];
    int IP[4],nodeIP[4],topic,i,k,count=0, charsRead = 0;
    PubSubMessageType type;
    PubSubInfo pbNewInfo,*pbCurrentRecord;
    bool isTableUpdated = false;
    routingTableEntry *routingTableValue;
    char* token, *spaceToken,*entry;
    char* nodeIPPart, *topicsPart;
    char *saveptr1, *saveptr2;

    //MESSAGE_TYPE  PUBSUB_TYPE  [sender/destination IP]  [nodeIP]  topic
    sscanf(messageBuffer,"%*i %i %i.%i.%i.%i %n",&type,&IP[0],&IP[1],&IP[2],&IP[3],&charsRead);

    // Copy the rest of the string manually
    strncpy(infoPubSub, messageBuffer + charsRead, sizeof(infoPubSub) - 1);

    //printf("InfoPubSub: %s\n",infoPubSub);

    switch (type) {

        case PUBSUB_PUBLISH:
            //13  0 [destination IP] [Publisher IP] [Published Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);
            break;
        case PUBSUB_SUBSCRIBE:
            //Message sent when a one node subscribes to a certain topic
            //13 1 [sender IP] [Subscriber IP] [Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

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
            //routingTableValue = (routingTableEntry*) findRouteToNode(IP);
            //if(routingTableValue != nullptr){
                //sendMessage(routingTableValue->nextHopIP, messageBuffer);
            //}


            //Propagate the subscription message in the network
            rewriteSenderIPPubSub(messageBuffer, bufferSize,PUBSUB_SUBSCRIBE);
            propagateMessage(messageBuffer,IP);
            break;

        case PUBSUB_UNSUBSCRIBE:
            //Message sent when a one unsubscribes to a certain topic
            //13 2 [sender IP] [Unsubscriber IP] [Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);

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
            //routingTableValue = (routingTableEntry*) findRouteToNode(IP);
            //if(routingTableValue != nullptr){
                //sendMessage(routingTableValue->nextHopIP, messageBuffer);
            //}

            //Propagate the deleted subscription message in the network
            rewriteSenderIPPubSub(messageBuffer, bufferSize,PUBSUB_UNSUBSCRIBE);
            propagateMessage(messageBuffer,IP);

            break;
        case PUBSUB_ADVERTISE:
            // Message used to advertise that a node is publishing a new topic
            //13 3 [sender IP] [Publisher IP] [Published Topic]
            sscanf(infoPubSub,"%i.%i.%i.%i %i",&nodeIP[0],&nodeIP[1],&nodeIP[2],&nodeIP[3],&topic);
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
            rewriteSenderIPPubSub(messageBuffer, bufferSize,PUBSUB_ADVERTISE);
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
            rewriteSenderIPPubSub(messageBuffer, bufferSize,PUBSUB_UNADVERTISE);
            propagateMessage(messageBuffer,IP);

            break;

        case PUBSUB_NODE_UPDATE:
            // Message used to advertise all publish-subscribe information of the node
            //13 5 [sender IP] [node IP] | [Published Topic List] [Subscribed Topics List]
            nodeIPPart = strtok(infoPubSub, "|");  // First part before the '|'
            topicsPart = strtok(NULL, "|");           // Second part after the '|'

            if (nodeIPPart != NULL && topicsPart != NULL) {
                // Parse node IP from nodeIPPart (you may need to skip the message type and sender IP first)
                int dummy1, dummy2, senderIP[4];
                sscanf(nodeIPPart, "%d.%d.%d.%d",&nodeIP[0], &nodeIP[1], &nodeIP[2], &nodeIP[3]);

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
            rewriteSenderIPPubSub(messageBuffer, bufferSize,PUBSUB_NODE_UPDATE);
            propagateMessage(messageBuffer,IP);

            break;/******/

        case PUBSUB_TABLE_UPDATE:
            //Buffer max size = 22 + 30*N
            //13 6 [sender IP] |[node IP] [Published Topic List] [Subscribed Topics List] |[node IP] [Published Topic List] [Subscribed Topics List]...
            entry = strtok_r(infoPubSub, "|", &saveptr1);

            while (entry != NULL) {
                // Trim leading spaces
                while (*entry == ' ') entry++;

                // Now parse this entry
                token = strtok_r(entry, " ", &saveptr2);

                if (token == NULL) {
                    entry = strtok_r(NULL, "|", &saveptr1);
                    continue;
                }

                // Parse node IP
                sscanf(token, "%d.%d.%d.%d", &nodeIP[0], &nodeIP[1], &nodeIP[2], &nodeIP[3]);
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


void onNetworkEventStrategyPubSub(int networkEvent, int involvedIP[4]){
    switch (networkEvent) {
        case NETEVENT_JOINED_NETWORK:
            encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_NODE_UPDATE);
            sendMessage(involvedIP,smallSendBuffer);
            LOG(MESSAGES,INFO,"Sending [MIDDLEWARE/PUBSUB_NODE_INFO] message: \"%s\" to: %i.%i.%i.%i\n",smallSendBuffer,involvedIP[0],involvedIP[1],involvedIP[2],involvedIP[3]);

            break;
        case NETEVENT_CHILD_CONNECTED:
            encodeMessageStrategyPubSub(largeSendBuffer, sizeof(largeSendBuffer),PUBSUB_TABLE_UPDATE);
            sendMessage(involvedIP,largeSendBuffer);
            LOG(MESSAGES,INFO,"Sending [MIDDLEWARE/PUBSUB_TABLE_INFO] message: \"%s\" to: %i.%i.%i.%i\n",largeSendBuffer,involvedIP[0],involvedIP[1],involvedIP[2],involvedIP[3]);
            break;
        case NETEVENT_CHILD_DISCONNECTED:
            break;
        default:
            break;
    }
}

void influenceRoutingStrategyPubSub(char* dataMessage){
    int i,j;
    PubSubInfo *myPubSubInfo, *nodePubSubInfo;
    int *nodeIP;
    int8_t topicType;
    bool publisher = false;
    messageParameters parameters;
    int *nextHopIP, nChars = 0;
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
        nodeIP = (int*) tableKey(pubsubTable,i);
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
                    assignIP(parameters.IP1,myIP);
                    assignIP(parameters.IP2,nodeIP);
                    strncpy(parameters.payload,payload, sizeof(parameters.payload));
                    encodeMessage(largeSendBuffer,sizeof(largeSendBuffer),DATA_MESSAGE,parameters);

                    // Send the published topic to the subscriber node
                    nextHopIP = (int*) findRouteToNode(nodeIP);
                    if(nextHopIP != nullptr){
                        sendMessage(nextHopIP,largeSendBuffer);
                        LOG(NETWORK,ERROR,"Sending [DATA] message: %s to %i.%i.%i.%i\n",largeSendBuffer,nodeIP[0],nodeIP[1],nodeIP[2],nodeIP[3]);
                    }else{
                        LOG(NETWORK,ERROR,"ERROR: Unable to find a path to the node in the routing table\n");
                    }
                }
            }

        }
    }
}

void onTimerStrategyPubSub(){
    unsigned long currentTime = getCurrentTime();
    //Periodically send this node's metric to all other nodes in the network
    if( (currentTime - lastMiddlewareUpdateTimePubSub) >= 10000 ) {
        /***encodeMessageStrategyPubSub(largeSendBuffer, sizeof(largeSendBuffer), PUBSUB_NODE_UPDATE);
        propagateMessage(largeSendBuffer, myIP);
        LOG(NETWORK,DEBUG,"Sending periodic [MIDDLEWARE/PUBSUB_NODE_INFO] Message: %s\n",largeSendBuffer);
        lastMiddlewareUpdateTimePubSub = currentTime;***/
    }
}


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
    topic = subTopic;
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_SUBSCRIBE);
    propagateMessage(smallSendBuffer,myIP);

}

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

    // Announce that i am subscribing to that topic
    topic = subTopic;
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_SUBSCRIBE);
    propagateMessage(smallSendBuffer,myIP);
}

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
    topic = pubTopic;
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_ADVERTISE);
    propagateMessage(smallSendBuffer,myIP);
}

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
    topic = pubTopic;
    encodeMessageStrategyPubSub(smallSendBuffer, sizeof(smallSendBuffer),PUBSUB_UNADVERTISE);
    propagateMessage(smallSendBuffer,myIP);
}

void printPubSubStruct(TableEntry* Table){
    LOG(NETWORK,INFO,"Node[%d.%d.%d.%d] → ",
        ((int*)Table->key)[0],((int*)Table->key)[1],((int*)Table->key)[2],((int*)Table->key)[3]);

    LOG(NETWORK,INFO,"(Publishes: ");
    for(int i = 0; i < MAX_TOPICS; ++i) {
        LOG(NETWORK,INFO,"%d | ",
            ((PubSubInfo *)Table->value)->publishedTopics[i]);
    }
    LOG(NETWORK,INFO,") ");

    LOG(NETWORK,INFO,"(Subscriptions: ");
    for(int i = 0; i < MAX_TOPICS; ++i) {
        LOG(NETWORK,INFO,"%d | ",
            ((PubSubInfo *)Table->value)->subscribedTopics[i]);
    }
    LOG(NETWORK,INFO,")\n");

}

bool containsTopic(int8_t * list, int8_t topic){
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
    topicTypes type;
    char topicString[20];
    sscanf(dataMessage,"%s", &topicString);
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
