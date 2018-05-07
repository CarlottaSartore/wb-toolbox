#include "DotJNu.h"
#include "BlockInformation.h"
#include "Log.h"
#include "RobotInterface.h"
#include "Signal.h"

#include <iDynTree/Core/EigenHelpers.h>
#include <iDynTree/KinDynComputations.h>

#include <memory>

using namespace wbt;

const std::string DotJNu::ClassName = "DotJNu";

const unsigned INPUT_IDX_BASE_POSE = 0;
const unsigned INPUT_IDX_JOINTCONF = 1;
const unsigned INPUT_IDX_BASE_VEL = 2;
const unsigned INPUT_IDX_JOINT_VEL = 3;
const unsigned OUTPUT_IDX_DOTJ_NU = 0;

const unsigned PARAM_IDX_BIAS = WBBlock::NumberOfParameters - 1;
const unsigned PARAM_IDX_FRAME = PARAM_IDX_BIAS + 1;

unsigned DotJNu::numberOfParameters()
{
    return WBBlock::numberOfParameters() + 1;
}

bool DotJNu::parseParameters(BlockInformation* blockInfo)
{
    ParameterMetadata frameMetadata(PARAM_STRING, PARAM_IDX_FRAME, 1, 1, "frame");

    if (!blockInfo->addParameterMetadata(frameMetadata)) {
        wbtError << "Failed to store parameters metadata.";
        return false;
    }

    return blockInfo->parseParameters(m_parameters);
}

bool DotJNu::configureSizeAndPorts(BlockInformation* blockInfo)
{
    // Memory allocation / Saving data not allowed here

    if (!WBBlock::configureSizeAndPorts(blockInfo))
        return false;

    // INPUTS
    // ======
    //
    // 1) Homogeneous transform for base pose wrt the world frame (4x4 matrix)
    // 2) Joints position (1xDoFs vector)
    // 3) Base frame velocity (1x6 vector)
    // 4) Joints velocity (1xDoFs vector)
    //

    // Number of inputs
    if (!blockInfo->setNumberOfInputPorts(4)) {
        wbtError << "Failed to configure the number of input ports.";
        return false;
    }

    const unsigned dofs = getConfiguration().getNumberOfDoFs();

    // Size and type
    bool ok = true;
    ok = ok && blockInfo->setInputPortMatrixSize(INPUT_IDX_BASE_POSE, {4, 4});
    ok = ok && blockInfo->setInputPortVectorSize(INPUT_IDX_JOINTCONF, dofs);
    ok = ok && blockInfo->setInputPortVectorSize(INPUT_IDX_BASE_VEL, 6);
    ok = ok && blockInfo->setInputPortVectorSize(INPUT_IDX_JOINT_VEL, dofs);

    blockInfo->setInputPortType(INPUT_IDX_BASE_POSE, DataType::DOUBLE);
    blockInfo->setInputPortType(INPUT_IDX_JOINTCONF, DataType::DOUBLE);
    blockInfo->setInputPortType(INPUT_IDX_BASE_VEL, DataType::DOUBLE);
    blockInfo->setInputPortType(INPUT_IDX_JOINT_VEL, DataType::DOUBLE);

    if (!ok) {
        wbtError << "Failed to configure input ports.";
        return false;
    }

    // OUTPUTS
    // =======
    //
    // 1) Vector representing the \dot{J} \nu vector (1x6)
    //

    // Number of outputs
    if (!blockInfo->setNumberOfOutputPorts(1)) {
        wbtError << "Failed to configure the number of output ports.";
        return false;
    }

    // Size and type
    ok = blockInfo->setOutputPortVectorSize(OUTPUT_IDX_DOTJ_NU, 6);
    blockInfo->setOutputPortType(OUTPUT_IDX_DOTJ_NU, DataType::DOUBLE);

    return ok;
}

bool DotJNu::initialize(BlockInformation* blockInfo)
{
    if (!WBBlock::initialize(blockInfo)) {
        return false;
    }

    // INPUT PARAMETERS
    // ================

    if (!parseParameters(blockInfo)) {
        wbtError << "Failed to parse parameters.";
        return false;
    }

    std::string frame;
    if (!m_parameters.getParameter("frame", frame)) {
        wbtError << "Cannot retrieve string from frame parameter.";
        return false;
    }

    // Check if the frame is valid
    // ---------------------------

    const auto& model = getRobotInterface()->getKinDynComputations();
    if (!model) {
        wbtError << "Cannot retrieve handle to KinDynComputations.";
        return false;
    }

    if (frame != "com") {
        m_frameIndex = model->getFrameIndex(frame);
        if (m_frameIndex == iDynTree::FRAME_INVALID_INDEX) {
            wbtError << "Cannot find " + frame + " in the frame list.";
            return false;
        }
    }
    else {
        m_frameIsCoM = true;
        m_frameIndex = iDynTree::FRAME_INVALID_INDEX;
    }

    // OUTPUT
    // ======
    m_dotJNu = std::unique_ptr<iDynTree::Vector6>(new iDynTree::Vector6());
    m_dotJNu->zero();

    return static_cast<bool>(m_dotJNu);
}

bool DotJNu::terminate(const BlockInformation* blockInfo)
{
    return WBBlock::terminate(blockInfo);
}

bool DotJNu::output(const BlockInformation* blockInfo)
{
    const auto& model = getRobotInterface()->getKinDynComputations();

    if (!model) {
        wbtError << "Failed to retrieve the KinDynComputations object.";
        return false;
    }

    // GET THE SIGNALS POPULATE THE ROBOT STATE
    // ========================================

    const Signal basePoseSig = blockInfo->getInputPortSignal(INPUT_IDX_BASE_POSE);
    const Signal jointsPosSig = blockInfo->getInputPortSignal(INPUT_IDX_JOINTCONF);
    const Signal baseVelocitySignal = blockInfo->getInputPortSignal(INPUT_IDX_BASE_VEL);
    const Signal jointsVelocitySignal = blockInfo->getInputPortSignal(INPUT_IDX_JOINT_VEL);

    bool ok =
        setRobotState(&basePoseSig, &jointsPosSig, &baseVelocitySignal, &jointsVelocitySignal);

    if (!ok) {
        wbtError << "Failed to set the robot state.";
        return false;
    }

    // OUTPUT
    // ======

    if (!m_frameIsCoM) {
        *m_dotJNu = model->getFrameBiasAcc(m_frameIndex);
    }
    else {
        iDynTree::Vector3 comBiasAcc = model->getCenterOfMassBiasAcc();
        toEigen(*m_dotJNu).segment<3>(0) = iDynTree::toEigen(comBiasAcc);
        toEigen(*m_dotJNu).segment<3>(3).setZero();
    }

    // Forward the output to Simulink
    Signal output = blockInfo->getOutputPortSignal(OUTPUT_IDX_DOTJ_NU);
    output.setBuffer(m_dotJNu->data(), blockInfo->getOutputPortWidth(OUTPUT_IDX_DOTJ_NU));
    return true;
}
