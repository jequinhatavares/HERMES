#ifndef NN_PARAMS_H
#define NN_PARAMS_H

#include <cstdint>


// Supported activation functions
typedef enum ActivationType{
    ACT_RELU,
    ACT_SIGMOID,
    ACT_TANH,
    ACT_SOFTMAX,
    ACT_LINEAR
} ActivationType;


// Neural Network Layer Definition
typedef struct Layer{
    uint8_t numInputs;          // Input dimensions
    uint8_t numOutputs;         // Output dimensions (neurons)
    ActivationType activation;  // Activation function
    float* weights;       // Weights (size: inputs × outputs)
    float* biases;        // Biases (size = outputs)
} Layer;

// Top-level Neural Network Structure
typedef struct NeuralNetwork{
    uint8_t inputSize;          // Input layer size
    uint8_t outputSize;         // Output layer size
    uint8_t numHiddenLayers;    // Hidden layers count
    uint8_t numLayers;          // Total layers (hidden + output)
    uint8_t numNeurons;         // Total Number of neurons in the NN
    Layer* layers;        // Layer array
} NeuralNetwork;

// Declare neural network reference
extern const NeuralNetwork neuralNetwork;

// Implementation section (define when NEURAL_NET_IMPL is set) to avoid link time errors of multiple definitions
// This works by defining and initializing the variables only when NEURAL_NET_IMPL is defined
#ifdef NEURAL_NET_IMPL

// --- Layer 0: Input (2) → Hidden (4) ---
static float _weights0[] = {
    // Weights for 2 inputs × 4 outputs (row-major)
    0.5f, -0.2f,   // Input 1 → Hidden 1, 2, 3, 4
    0.3f,  0.1f,
    -0.4f, 0.6f,
    0.8f, -0.7f
};
static float _biases0[4] = { 0.1f, -0.1f, 0.2f, -0.2f };

// --- Layer 1: Hidden (4) → Hidden (4) ---
static float _weights1[] = {
    // Weights for 4 inputs × 4 outputs
    0.2f, -0.3f, 0.4f, -0.5f,
    0.1f,  0.6f, -0.7f, 0.8f,
    -0.9f, 0.5f, 0.3f, -0.1f,
    0.7f, -0.4f, 0.2f, 0.6f
};
static float _biases1[] = { -0.3f, 0.4f, -0.5f, 0.6f };

// --- Layer 2: Hidden (4) → Output (2) ---
static float _weights2[] = {
    // Weights for 4 inputs × 2 outputs
    0.9f, -0.8f,0.7f, -0.6f,
    0.5f, -0.4f,0.3f, -0.2f
};
static float _biases2[2] = { 0.1f, -0.1f };

// Layer array
static Layer _layers[3] = {
    { // Layer 0 (Input → Hidden)
        .numInputs = 2,
        .numOutputs = 4,
        .activation = ACT_RELU,
        .weights = _weights0,
        .biases = _biases0
    },
    { // Layer 1 (Hidden → Hidden)
        .numInputs = 4,
        .numOutputs = 4,
        .activation = ACT_RELU,
        .weights = _weights1,
        .biases = _biases1
    },
    { // Layer 2 (Hidden → Output)
        .numInputs = 4,
        .numOutputs = 2,
        .activation = ACT_RELU,
        .weights = _weights2,
        .biases = _biases2
    }
};

// Network instance
const NeuralNetwork neuralNetwork = {
    .inputSize = 2,
    .outputSize = 2,
    .numHiddenLayers = 2,
    .numLayers = 3,  // Input → Hidden → Hidden → Output
    .numNeurons = 12,
    .layers = _layers,
};


#endif

#endif //NN_PARAMS_H
