#pragma once

// ==============================================================================
//                         User Configuration File
// ==============================================================================
// This file lets you override the library’s default settings.
// To override a setting, uncomment the line and set your preferred value.
// Settings left commented will use the library’s built-in defaults.
// The commented values shown here reflect the current defaults defined in the library.


// ==============================================================================
//                         Application Variables
// ==============================================================================


// #define APPLICATION_PROCESSING_INTERVAL 12000000
// #define APPLICATION_RUNS_PERIODIC_TASKS false


// ==============================================================================
//                          Routing Configuration
// ==============================================================================
// Parameters that affect how the routing layer builds and maintains routes.

// #define ROUTING_UPDATE_INTERVAL 180000
// #define ROUTE_TTL_MS 60000         // Time before a route entry expires (in milliseconds)
// #define ALLOW_BROADCAST true       // Enable broadcast messaging
// #define MAX_ROUTE_HOPS 10          // Limit on the number of hops a message can take


// ==============================================================================
//                          Lifecycle Configuration
// ==============================================================================



// #define CHILD_RECONNECT_TIMEOUT 3000
// #define MAIN_TREE_RECONNECT_TIMEOUT 20000
// #define MAX_PARENT_SEARCH_ATTEMPTS 3
// #define CHILD_REGISTRATION_RETRY_COUNT 2


// ==============================================================================
//                           Network Configuration
// ==============================================================================



// #define ROOT_NODE false             // Set to true if this node is the root of the mesh
// #define NETWORK_ID 1               // Identifier for this network (for isolating multiple meshes)
// #define CHILD_TABLE_SIZE 8         // Number of child entries a node can hold


// ==============================================================================
//                            Wi-Fi Configuration
// ==============================================================================


// #define MESH_SSID "MyMeshNetwork"      // Mesh SSID (must be the same on all nodes)
// #define MESH_PASSWORD "password123"    // Mesh password (must match on all nodes)
// #define WIFI_CHANNEL 6                 // Wi-Fi channel (must be fixed across the mesh)
// #define AP_IP_BASE "192.168.4."        // Base IP for SoftAP mode (if applicable)


// ==============================================================================
//                         Middleware Configuration
// ==============================================================================


// #define ENABLE_MIDDLEWARE true         // Enable or disable the middleware layer
// #define MIDDLEWARE_BUFFER_SIZE 256     // Max size (bytes) of middleware message buffers
// #define MIDDLEWARE_TASK_DELAY_MS 50    // Time (ms) between middleware task executions