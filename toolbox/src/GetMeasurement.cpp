/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#include "GetMeasurement.h"
#include "BlockInformation.h"
#include "Log.h"
#include "RobotInterface.h"
#include "Signal.h"

#include <yarp/dev/IEncoders.h>
#include <yarp/dev/ITorqueControl.h>

#include <cmath>

using namespace wbt;

const std::string GetMeasurement::ClassName = "GetMeasurement";

const unsigned PARAM_IDX_BIAS = WBBlock::NumberOfParameters - 1;
const unsigned PARAM_IDX_MEAS_TYPE = PARAM_IDX_BIAS + 1;

void deg2rad(std::vector<double>& v)
{
    const double Deg2Rad = M_PI / 180.0;
    for (auto& element : v) {
        element *= Deg2Rad;
    }
}

unsigned GetMeasurement::numberOfParameters()
{
    return WBBlock::numberOfParameters() + 1;
}

bool GetMeasurement::parseParameters(BlockInformation* blockInfo)
{
    ParameterMetadata paramMD_measType(
        ParameterType::STRING, PARAM_IDX_MEAS_TYPE, 1, 1, "MeasuredType");

    bool ok = true;
    ok = ok && blockInfo->addParameterMetadata(paramMD_measType);

    if (!ok) {
        wbtError << "Failed to store parameters metadata.";
        return false;
    }

    return blockInfo->parseParameters(m_parameters);
}

bool GetMeasurement::configureSizeAndPorts(BlockInformation* blockInfo)
{
    if (!WBBlock::configureSizeAndPorts(blockInfo)) {
        return false;
    }

    // INPUTS
    // ======
    //
    // No inputs
    //

    if (!blockInfo->setNumberOfInputPorts(0)) {
        wbtError << "Failed to configure the number of input ports.";
        return false;
    }

    // OUTPUTS
    // =======
    //
    // 1) Vector with the information asked (1xDoFs)
    //

    if (!blockInfo->setNumberOfOutputPorts(1)) {
        wbtError << "Failed to configure the number of output ports.";
        return false;
    }

    // Get the DoFs
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }
    const auto dofs = robotInterface->getConfiguration().getNumberOfDoFs();

    bool success = blockInfo->setOutputPortVectorSize(0, dofs);
    blockInfo->setOutputPortType(0, DataType::DOUBLE);
    if (!success) {
        wbtError << "Failed to configure output ports.";
        return false;
    }

    return true;
}

bool GetMeasurement::initialize(BlockInformation* blockInfo)
{
    if (!WBBlock::initialize(blockInfo)) {
        return false;
    }

    if (!parseParameters(blockInfo)) {
        wbtError << "Failed to parse parameters.";
        return false;
    }

    // Read the measured type
    std::string measuredType;
    if (!m_parameters.getParameter("MeasuredType", measuredType)) {
        wbtError << "Could not read measured type parameter.";
        return false;
    }

    // Set the measured type
    if (measuredType == "Joints Position") {
        m_measuredType = wbt::MEASUREMENT_JOINT_POS;
    }
    else if (measuredType == "Joints Velocity") {
        m_measuredType = wbt::MEASUREMENT_JOINT_VEL;
    }
    else if (measuredType == "Joints Acceleration") {
        m_measuredType = wbt::MEASUREMENT_JOINT_ACC;
    }
    else if (measuredType == "Joints Torque") {
        m_measuredType = wbt::ESTIMATE_JOINT_TORQUE;
    }
    else {
        wbtError << "Measurement not supported.";
        return false;
    }

    // Get the DoFs
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }
    const auto dofs = robotInterface->getConfiguration().getNumberOfDoFs();

    // Initialize the size of the output vector
    m_measurement.resize(dofs);

    // Retain the ControlBoardRemapper
    if (!robotInterface->retainRemoteControlBoardRemapper()) {
        wbtError << "Couldn't retain the RemoteControlBoardRemapper.";
        return false;
    }

    return true;
}

bool GetMeasurement::terminate(const BlockInformation* blockInfo)
{
    // Get the RobotInterface
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }

    // Release the RemoteControlBoardRemapper
    bool ok = true;
    ok = ok && robotInterface->releaseRemoteControlBoardRemapper();
    if (!ok) {
        wbtError << "Failed to release the RemoteControlBoardRemapper.";
        // Don't return false here. WBBlock::terminate must be called in any case
    }

    return ok && WBBlock::terminate(blockInfo);
}

bool GetMeasurement::output(const BlockInformation* blockInfo)
{
    // Get the RobotInterface
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }

    bool ok;
    switch (m_measuredType) {
        case MEASUREMENT_JOINT_POS: {
            // Get the interface
            yarp::dev::IEncoders* iEncoders = nullptr;
            if (!robotInterface->getInterface(iEncoders) || !iEncoders) {
                wbtError << "Failed to get IPidControl interface.";
                return false;
            }
            // Get the measurement
            ok = iEncoders->getEncoders(m_measurement.data());
            deg2rad(m_measurement);
            break;
        }
        case MEASUREMENT_JOINT_VEL: {
            // Get the interface
            yarp::dev::IEncoders* iEncoders = nullptr;
            if (!robotInterface->getInterface(iEncoders) || !iEncoders) {
                wbtError << "Failed to get IEncoders interface.";
                return false;
            }
            // Get the measurement
            ok = iEncoders->getEncoderSpeeds(m_measurement.data());
            deg2rad(m_measurement);
            break;
        }
        case MEASUREMENT_JOINT_ACC: {
            // Get the interface
            yarp::dev::IEncoders* iEncoders = nullptr;
            if (!robotInterface->getInterface(iEncoders) || !iEncoders) {
                wbtError << "Failed to get IEncoders interface.";
                return false;
            }
            // Get the measurement
            ok = iEncoders->getEncoderAccelerations(m_measurement.data());
            deg2rad(m_measurement);
            break;
        }
        case ESTIMATE_JOINT_TORQUE: {
            // Get the interface
            yarp::dev::ITorqueControl* iTorqueControl = nullptr;
            if (!robotInterface->getInterface(iTorqueControl) || !iTorqueControl) {
                wbtError << "Failed to get ITorqueControl interface.";
                return false;
            }
            // Get the measurement
            ok = iTorqueControl->getTorques(m_measurement.data());
            break;
        }
    }

    if (!ok) {
        wbtError << "Failed to get measurement.";
        return false;
    }

    // Get the output signal
    Signal output = blockInfo->getOutputPortSignal(0);
    if (!output.isValid()) {
        wbtError << "Output signal not valid.";
        return false;
    }

    // Fill the output buffer
    output.setBuffer(m_measurement.data(), output.getWidth());

    return true;
}
