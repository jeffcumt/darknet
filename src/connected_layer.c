#include "connected_layer.h"
#include "utils.h"
#include "mini_blas.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

connected_layer *make_connected_layer(int batch, int inputs, int outputs, float dropout, ACTIVATION activation)
{
    fprintf(stderr, "Connected Layer: %d inputs, %d outputs\n", inputs, outputs);
    int i;
    connected_layer *layer = calloc(1, sizeof(connected_layer));
    layer->inputs = inputs;
    layer->outputs = outputs;
    layer->batch=batch;
    layer->dropout = dropout;

    layer->output = calloc(batch*outputs, sizeof(float*));
    layer->delta = calloc(batch*outputs, sizeof(float*));

    layer->weight_updates = calloc(inputs*outputs, sizeof(float));
    layer->weight_adapt = calloc(inputs*outputs, sizeof(float));
    layer->weight_momentum = calloc(inputs*outputs, sizeof(float));
    layer->weights = calloc(inputs*outputs, sizeof(float));
    float scale = 1./inputs;
    for(i = 0; i < inputs*outputs; ++i)
        layer->weights[i] = scale*(rand_uniform());

    layer->bias_updates = calloc(outputs, sizeof(float));
    layer->bias_adapt = calloc(outputs, sizeof(float));
    layer->bias_momentum = calloc(outputs, sizeof(float));
    layer->biases = calloc(outputs, sizeof(float));
    for(i = 0; i < outputs; ++i)
        //layer->biases[i] = rand_normal()*scale + scale;
        layer->biases[i] = 1;

    layer->activation = activation;
    return layer;
}

void update_connected_layer(connected_layer layer, float step, float momentum, float decay)
{
    int i;
    for(i = 0; i < layer.outputs; ++i){
        layer.bias_momentum[i] = step*(layer.bias_updates[i]) + momentum*layer.bias_momentum[i];
        layer.biases[i] += layer.bias_momentum[i];
    }
    for(i = 0; i < layer.outputs*layer.inputs; ++i){
        layer.weight_momentum[i] = step*(layer.weight_updates[i] - decay*layer.weights[i]) + momentum*layer.weight_momentum[i];
        layer.weights[i] += layer.weight_momentum[i];
    }
    memset(layer.bias_updates, 0, layer.outputs*sizeof(float));
    memset(layer.weight_updates, 0, layer.outputs*layer.inputs*sizeof(float));
}

void forward_connected_layer(connected_layer layer, float *input, int train)
{
    if(!train) layer.dropout = 0;
    memcpy(layer.output, layer.biases, layer.outputs*sizeof(float));
    int m = layer.batch;
    int k = layer.inputs;
    int n = layer.outputs;
    float *a = input;
    float *b = layer.weights;
    float *c = layer.output;
    gemm(0,0,m,n,k,1,a,k,b,n,1,c,n);
    activate_array(layer.output, layer.outputs*layer.batch, layer.activation, layer.dropout);
}

void backward_connected_layer(connected_layer layer, float *input, float *delta)
{
    int i;
    for(i = 0; i < layer.outputs*layer.batch; ++i){
        layer.delta[i] *= gradient(layer.output[i], layer.activation);
        layer.bias_updates[i%layer.batch] += layer.delta[i];
    }
    int m = layer.inputs;
    int k = layer.batch;
    int n = layer.outputs;
    float *a = input;
    float *b = layer.delta;
    float *c = layer.weight_updates;
    gemm(0,0,m,n,k,1,a,k,b,n,1,c,n);

    m = layer.inputs;
    k = layer.outputs;
    n = layer.batch;

    a = layer.weights;
    b = layer.delta;
    c = delta;

    if(c) gemm(0,0,m,n,k,1,a,k,b,n,0,c,n);
}

