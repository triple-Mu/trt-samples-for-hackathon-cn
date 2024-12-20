# Workflow of pyTorch -> ONNX -> TensorRT

+ Steps to run.

```bash
python3 main.py
```

+ Here is a workflow of:
  + Export trained model from pyTorch to ONNX
  + Parse ONNX in TensorRT
  + Build TensorRT engine and do inference

+ We need to run `00-Data/get_model.py` firstly to get related ONNX files as input.

+ Here are some independent cases:
  + Normal: use FP32 to work (original ONNX is in FP32 mode).
  + FP16: use FP16 to work.
  + INT8-PTQ: use INT8-PTQ to work, it shows the usage of calibrator as well.
  + INT8-QAT: use INT8-QAT to work, we need a INT8-QAT ONNX as input.

+ Here is a equivalent INT8-PTQ workflow in `C++` directory, the key point is the calibrator, comparing to the example in 01-SimpleDemo

```bash
cd C++
make clean && make
./main.exe
```
