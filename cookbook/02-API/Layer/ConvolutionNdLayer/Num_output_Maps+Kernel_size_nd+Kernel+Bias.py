#
# Copyright (c) 2021, NVIDIA CORPORATION. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import numpy as np
from cuda import cudart
import tensorrt as trt

nB, nC, nH, nW = 1, 1, 6, 9  # 输入张量 NCHW
nCOut, nKernelHeight, nKernelWidth = 1, 3, 3  # 卷积权重的输出通道数、高度和宽度
data = np.tile(np.arange(1, 1 + nKernelHeight * nKernelWidth, dtype=np.float32).reshape(nKernelHeight, nKernelWidth), (nC, nH // nKernelHeight, nW // nKernelWidth)).reshape(1, nC, nH, nW)  # 输入数据
weight = np.ascontiguousarray(np.power(10, range(4, -5, -1), dtype=np.float32).reshape(nCOut, nKernelHeight, nKernelWidth))  # 卷积权重
bias = np.ascontiguousarray(np.zeros(nCOut, dtype=np.float32))  # 卷积偏置

np.set_printoptions(precision=8, linewidth=200, suppress=True)
cudart.cudaDeviceSynchronize()

logger = trt.Logger(trt.Logger.ERROR)
builder = trt.Builder(logger)
network = builder.create_network(1 << int(trt.NetworkDefinitionCreationFlag.EXPLICIT_BATCH))
config = builder.create_builder_config()
inputT0 = network.add_input("inputT0", trt.float32, (nB, nC, nH, nW))
#-------------------------------------------------------------------------------# 网络部分
placeHolder = np.zeros(1, dtype=np.float32)
convolutionLayer = network.add_convolution_nd(inputT0, 1, (1, 1), placeHolder)  # 先填入一些参数，bias 为可选参数，默认值 None
convolutionLayer.num_output_maps = nCOut  # 重设卷积输出通道数
convolutionLayer.kernel_size_nd = (nKernelHeight, nKernelWidth)  # 重设卷积窗口尺寸
convolutionLayer.kernel = trt.WEights(weight)  # 重设卷积权值
convolutionLayer.bias = trt.Weights(bias)  # 重设卷积偏置
#-------------------------------------------------------------------------------# 网络部分
network.mark_output(convolutionLayer.get_output(0))
engineString = builder.build_serialized_network(network, config)
engine = trt.Runtime(logger).deserialize_cuda_engine(engineString)
context = engine.create_execution_context()
context.set_binding_shape(0, data.shape)
nInput = np.sum([engine.binding_is_input(i) for i in range(engine.num_bindings)])
nOutput = engine.num_bindings - nInput

bufferH = []
bufferH.append(data)
for i in range(nOutput):
    bufferH.append(np.empty(context.get_binding_shape(nInput + i), dtype=trt.nptype(engine.get_binding_dtype(nInput + i))))
bufferD = []
for i in range(engine.num_bindings):
    bufferD.append(cudart.cudaMalloc(bufferH[i].nbytes)[1])

for i in range(nInput):
    cudart.cudaMemcpy(bufferD[i], np.ascontiguousarray(bufferH[i].reshape(-1)).ctypes.data, bufferH[i].nbytes, cudart.cudaMemcpyKind.cudaMemcpyHostToDevice)
context.execute_v2(bufferD)
for i in range(nOutput):
    cudart.cudaMemcpy(bufferH[nInput + i].ctypes.data, bufferD[nInput + i], bufferH[nInput + i].nbytes, cudart.cudaMemcpyKind.cudaMemcpyDeviceToHost)

for i in range(nInput):
    print("Input %d:" % i, bufferH[i].shape, "\n", bufferH[i])
for i in range(nOutput):
    print("Output %d:" % i, bufferH[nInput + i].shape, "\n", bufferH[nInput + i])

for buffer in bufferD:
    cudart.cudaFree(buffer)