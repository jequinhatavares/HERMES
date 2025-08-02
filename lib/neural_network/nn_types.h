#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

#include <cstdint>

typedef enum NeuralNetworkMessageType{
    NN_ASSIGN_COMPUTATION,  //O Message from the coordinator assigning neurons to nodes
    NN_ASSIGN_OUTPUT_TARGETS,      //1 Message from the coordinator specifying which neurons require the output of the assigned neuron
    NN_ASSIGN_INPUTS,       //2 Message sent by the coordinator node assigning a input node to a given physical node
    NN_NEURON_OUTPUT,       //3 Message from a node transmitting its computed neuron output
    NN_FORWARD,             //4 Message from the coordinator to trigger a new inference operation
    NN_NACK,                //5 Message from a node indicating that some required inputs are missing
    NN_ACK,                 //6 Message from a node to the coordinator acknowledging receipt of neuron assignment
    NN_WORKER_REGISTRATION, //7 Message sent from a node to the root node to register as a potential neural network worker,including device type information (ESP8266, ESP32, or Raspberry Pi).
    NN_INPUT_REGISTRATION,  //8 Message sent from a node to the root node to register as a input of the NN
}NeuralNetworkMessageType;


/*** NeuronId is defined as a uint8_t, allowing up to 256 neurons with IDs ranging from 0 to 255.
    To support a larger neural network (more than 256 neurons), simply change the type of this variable
    to uint16_t, uint32_t, or uint64_t as needed — the rest of the code will continue to function correctly.***/
typedef uint8_t NeuronId;


#endif //MESSAGE_TYPES_H
