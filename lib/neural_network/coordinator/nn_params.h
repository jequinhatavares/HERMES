#ifndef NN_PARAMS_H
#define NN_PARAMS_H

struct NNStruture{
    int nNeurons;
    int nHiddenLayers;
    int* nodePerLayer;
};

float fc1_weight[4][3] = {
        { 0.12f, -0.34f, 0.56f },
        { -0.45f, 0.67f, -0.89f },
        { 0.23f, -0.12f, 0.91f },
        { -0.78f, 0.54f, -0.33f }
};

float fc1_bias[4] = {
        0.01f, -0.02f, 0.03f, -0.04f
};

float fc2_weight[2][4] = {
        { 0.14f, 0.25f, -0.36f, 0.47f },
        { -0.15f, 0.26f, -0.37f, 0.48f }
};

float fc2_bias[2] = {
        0.05f, -0.06f
};


#endif //NN_PARAMS_H
