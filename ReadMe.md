# Project Documentation

## Overview

This project implements a multihop routing protocol designed for devices such as ESP32 and NodeMCU. 
It autonomously builds and manages a self-constructed tree topology, handling all aspects of routing within the network.
The communication technology used is Wi-Fi.

## Table of Contents
1. [How to Run the Project](#how-to-run-the-project)
2. [Code Structure](#code-structure)
3. [Routing Protocol Documentation](#routing-protocol-documentation)
4. [Node Lifecycle (State Machine)](#node-lifecycle-state-machine)
5. [Logging](#logging)
6. [CLI](#cli)
7. [Visualization Program](#visualization-program)

## How to Run the Project
Provide instructions on how to set up the environment, dependencies, and the steps to run the project.

## Code Structure
Explain the general structure of the code, detailing the main modules and their roles in the project.

## Routing Protocol Documentation
### Overview
Provide an overview of the routing protocol, including how it operates within the network.
### Key Components
Detail the important components of the routing protocol and how they interact.
### Workflow
Describe the step-by-step workflow of the protocol, from initialization to route management.

## Node Lifecycle (State Machine)
Explain the node lifecycle, detailing the states each node can be in and the transitions between them.

## Logging
Describe how logging is handled within the project, including the configuration and types of logs generated.

## CLI
For debugging, development, and monitoring purposes, a Command Line Interface (CLI) has been implemented. 
To enter the CLI, simply press "Enter" in the serial monitor of the node you wish to monitor. 
Upon entering, a menu will be displayed with various options, such as visualizing the node’s routing and children tables or sending messages to other nodes within the network.

<span style="color: blue;">Note</span>: When in CLI mode, the node becomes "locked" in this mode and will not respond to or receive any network messages.

**Tip**: In PlatformIO, to view the words you type in the serial monitor, press `[CTRL] + [T]` followed by `[CTRL] + [E]`.


## Visualization Program
Explain the purpose of the visualization program, how it connects to the root node, and how it displays network connections and metrics.
