/*
 * Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <NvInfer.h>
#include <cub/cub.cuh>
#include <curand.h>

#define ALIGNSIZE  1024
#define ALIGNED(x) (((x) + ALIGNSIZE - 1) / ALIGNSIZE * ALIGNSIZE)

using namespace nvinfer1;

class RandomPlugin : public IPluginV2Ext
{
private:
    struct
    {
        int                nRow;
        int                nCol;
        unsigned long long seed;
    } m;

public:
    RandomPlugin(int seed)
    {
        m.seed = (unsigned long long)seed;
    }

    int getNbOutputs() const override
    {
        return 2;
    }

    DataType getOutputDataType(int index, const DataType *inputTypes, int nbInputs) const
    {
        return (index == 0) ? DataType::kINT32 : DataType::kFLOAT;
    }

    Dims getOutputDimensions(int index, const Dims *pInputDim, int nInputDim) override
    {
        return Dims {1, {pInputDim[0].d[0]}};
    }

    bool supportsFormat(DataType type, PluginFormat format) const override
    {
        return (type == DataType::kFLOAT || type == DataType::kINT32) && format == PluginFormat::kNCHW;
    }

    size_t getWorkspaceSize(int nBatch) const override
    {
        return ALIGNED(m.nRow * sizeof(float)) + ((m.nCol == 192) ? ALIGNED(m.nRow * 192 * sizeof(int)) : 0);
    }

    void configurePlugin(const Dims *inputDims, int nbInputs, const Dims *outputDims, int nbOutputs, const DataType *inputTypes, const DataType *outputTypes, const bool *inputIsBroadcast, const bool *outputIsBroadcast, PluginFormat floatFormat, int maxBatchSize)
    {
        m.nRow = inputDims[0].d[0];
        m.nCol = inputDims[0].d[1];
    }

    int enqueue(int nBatch, const void *const *inputs, void **outputs, void *workspace, cudaStream_t stream) override;
    RandomPlugin(const void *buffer, size_t length)
    {
        memcpy(&m, buffer, sizeof(m));
    }
    IPluginV2Ext *clone() const override
    {
        return new RandomPlugin(&m, sizeof(m));
    }
    int initialize() override
    {
        return 0;
    }
    void terminate() override {}
    void destroy() override
    {
        delete this;
    }
    bool isOutputBroadcastAcrossBatch(int outputIndex, const bool *inputIsBroadcasted, int nbInputs) const
    {
        return false;
    }
    bool canBroadcastInputAcrossBatch(int inputIndex) const
    {
        return false;
    }
    void        attachToContext(cudnnContext *contextCudnn, cublasContext *contextCublas, IGpuAllocator *gpuAllocator) {}
    void        detachFromContext() {}
    const char *getPluginType() const override
    {
        return "RandomPlugin";
    }
    const char *getPluginVersion() const override
    {
        return "0";
    }
    void        setPluginNamespace(const char *szNamespace) override {}
    const char *getPluginNamespace() const override
    {
        return "";
    }
    virtual size_t getSerializationSize() const override
    {
        return sizeof(m);
    }
    virtual void serialize(void *buffer) const override
    {
        memcpy(buffer, &m, sizeof(m));
    }
};

class RandomPluginCreator : public IPluginCreator
{
public:
    IPluginV2 *createPlugin(const char *name, const PluginFieldCollection *fc) override
    {
        int seed = 97;
        for (int i = 0; i < fc->nbFields; ++i)
        {
            if (!strcmp(fc->fields[i].name, "seed"))
                seed = *(int *)fc->fields[i].data;
        }
        return new RandomPlugin(seed);
    }

    const char *getPluginName() const override
    {
        return "RandomPlugin";
    }
    const char *getPluginVersion() const override
    {
        return "0";
    }
    void        setPluginNamespace(const char *szNamespace) override {}
    const char *getPluginNamespace() const override
    {
        return "";
    }
    const PluginFieldCollection *getFieldNames() override
    {
        return nullptr;
    }
    IPluginV2 *deserializePlugin(const char *name, const void *serialData, size_t serialLength) override
    {
        return new RandomPlugin(serialData, serialLength);
    }
};
