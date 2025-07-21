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

// Interval between periodic application-level tasks (in milliseconds)
// #define APPLICATION_PROCESSING_INTERVAL 12000000

// Whether the node runs periodic tasks in the application layer
// #define APPLICATION_RUNS_PERIODIC_TASKS false

// Enable real-time monitoring server features for visualizing the network topology
//  #define MONITORING_ON

// Application Data message maximum payload size
// #define MAX_PAYLOAD_SIZE 200


// ==============================================================================
//                         Middleware Configuration
// ==============================================================================

// Interval between middleware periodic updates (in milliseconds)
// #define MIDDLEWARE_UPDATE_INTERVAL 120000


// ==============================================================================
//                           Network Configuration
// ==============================================================================

// Maximum number of entries in the node’s routing table (defines how many other nodes the network can support in total)
// #define TABLE_MAX_SIZE 10

// Wi-Fi network SSID
// #define WIFI_SSID "JessicaNode"

// Wi-Fi network password
// #define WIFI_PASSWORD "123456789"


// ==============================================================================
//                          Lifecycle Configuration
// ==============================================================================

// Size of the circular buffer that holds lifecycle-related events such as message arrivals, timeouts or disconnection
// signals used by the state machine
// #define CIRCULAR_BUFFER_SIZE 10

// Timeout waiting for a parent node’s reply (in milliseconds)
// #define PARENT_REPLY_TIMEOUT 3000

// Time (in milliseconds) a child has to reconnect to this node’s AP interface before being considered permanently lost
// #define CHILD_RECONNECT_TIMEOUT 3000

// Maximum time (in milliseconds) the node remains in the RECOVER_AWAIT state, waiting
// for its subnetwork to rejoin the main network before detaching and restarting discovery
// #define MAIN_TREE_RECONNECT_TIMEOUT 20000

// Maximum number of attempts to find a new parent before giving up and releasing all child nodes in the current subnetwork
// #define MAX_PARENT_SEARCH_ATTEMPTS 3

// Number of retry attempts to register as a child with a given parent before restarting the parent selection process
// #define CHILD_REGISTRATION_RETRY_COUNT 2

// Number of disconnection events required before triggering parent recovery
// #define PARENT_DISCONNECTION_THRESHOLD 3

// ==============================================================================
//                          Routing Configuration
// ==============================================================================
// Parameters that affect how the routing layer builds and maintains routes.

// Interval between periodic routing updates (in milliseconds)
// #define ROUTING_UPDATE_INTERVAL 180000



// ==============================================================================
//                            UDP Configurations
// ==============================================================================

// UDP port used, if the network integrates a Raspberry Pi set this to a port above 1024
// #define UDP_PORT 12345


// ==============================================================================
//                            Wi-Fi Configuration
// ==============================================================================

// Timeout while trying to connect to the parent’s Wi-Fi network (in milliseconds)
// #define WIFI_CONNECTION_TIMEOUT 3000

// Grace period to ignore disconnections (in milliseconds)
// #define AP_DISCONNECTION_GRACE_PERIOD 3000

// Wi-Fi channel (must be fixed across the network)
// #define WIFI_CHANNEL 6



