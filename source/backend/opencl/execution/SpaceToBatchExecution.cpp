//
//  SpaceToBatchExecution.cpp
//  MNN
//
//  Created by MNN on 2019/02/28.
//  Copyright © 2018, Alibaba Group Holding Limited
//

#include "backend/opencl/execution/SpaceToBatchExecution.hpp"
#include "core/Macro.h"
#include "core/TensorUtils.hpp"

namespace MNN {
namespace OpenCL {

SpaceToBatchExecution::SpaceToBatchExecution(const std::vector<Tensor *> &inputs, const MNN::Op *op, Backend *backend)
    : Execution(backend) {
#ifdef LOG_VERBOSE
    MNN_PRINT("Start SpaceToBatchExecution init !\n");
#endif
    mOpenCLBackend = static_cast<OpenCLBackend *>(backend);
    auto param     = op->main_as_SpaceBatch();
    mPaddings[0]   = param->padding()->int32s()->data()[0];
    mPaddings[1]   = param->padding()->int32s()->data()[2];
    mBlockShape[0] = param->blockShape()->int32s()->data()[0];
    mBlockShape[1] = param->blockShape()->int32s()->data()[1];
    std::set<std::string> buildOptions;
    std::string kernelName = "space_to_batch";
    mKernel = mOpenCLBackend->getOpenCLRuntime()->buildKernel("space_to_batch", kernelName, buildOptions);
#ifdef LOG_VERBOSE
    MNN_PRINT("end SpaceToBatchExecution init !\n");
#endif
}

ErrorCode SpaceToBatchExecution::onResize(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) {
#ifdef LOG_VERBOSE
    MNN_PRINT("Start SpaceToBatchExecution onResize !\n");
#endif
    auto input  = inputs[0];
    auto output = outputs[0];
    int inputSize[4]  = {input->width(), input->height(), UP_DIV(input->channel(), 4), input->batch()};
    int outputSize[4] = {output->width(), output->height(), UP_DIV(output->channel(), 4), output->batch()};
    uint32_t idx = 0;
    mKernel.setArg(idx++, outputSize[2]);
    mKernel.setArg(idx++, outputSize[0]);
    mKernel.setArg(idx++, outputSize[1]*outputSize[3]);
    mKernel.setArg(idx++, openCLImage(input));
    mKernel.setArg(idx++, openCLImage(output));
    mKernel.setArg(idx++, sizeof(inputSize), inputSize);
    mKernel.setArg(idx++, sizeof(outputSize), outputSize);
    mKernel.setArg(idx++, sizeof(mPaddings), mPaddings);
    mKernel.setArg(idx++, sizeof(mBlockShape), mBlockShape);
#ifdef LOG_VERBOSE
    MNN_PRINT("end SpaceToBatchExecution onResize !\n");
#endif
    return NO_ERROR;
}

ErrorCode SpaceToBatchExecution::onExecute(const std::vector<Tensor *> &inputs, const std::vector<Tensor *> &outputs) {
#ifdef LOG_VERBOSE
    MNN_PRINT("Start SpaceToBatchExecution onExecute !\n");
#endif
    auto input  = inputs[0];
    auto output = outputs[0];

    int inputSize[4]  = {input->width(), input->height(), UP_DIV(input->channel(), 4), input->batch()};
    int outputSize[4] = {output->width(), output->height(), UP_DIV(output->channel(), 4), output->batch()};

    auto runtime = mOpenCLBackend->getOpenCLRuntime();


    runtime->commandQueue().enqueueNDRangeKernel(
        mKernel, cl::NullRange,
        cl::NDRange(UP_DIV(outputSize[2], 16) * 16, UP_DIV(outputSize[0], 16) * 16, outputSize[1] * outputSize[3]),
        cl::NDRange(16, 16, 1));

#ifdef LOG_VERBOSE
    MNN_PRINT("end SpaceToBatchExecution onExecute !\n");
#endif
    return NO_ERROR;
}

OpenCLCreatorRegister<TypedCreator<SpaceToBatchExecution>> __space_to_batch_op(OpType_SpaceToBatchND);

} // namespace OpenCL
} // namespace MNN
