#include "GetLimits.h"
#include "BlockInformation.h"
#include "Log.h"
#include "RobotInterface.h"
#include "Signal.h"

#include <iDynTree/KinDynComputations.h>
#include <iDynTree/Model/Model.h>
#include <yarp/dev/IControlLimits2.h>

#include <limits>
#include <vector>

#define _USE_MATH_DEFINES
#include <cmath>

using namespace wbt;

const std::string GetLimits::ClassName = "GetLimits";

const unsigned PARAM_IDX_BIAS = WBBlock::NumberOfParameters - 1;
const unsigned PARAM_IDX_LIMIT_SRC = PARAM_IDX_BIAS + 1;

double deg2rad(const double& v)
{
    return v * M_PI / 180.0;
}

unsigned GetLimits::numberOfParameters()
{
    return WBBlock::numberOfParameters() + 1;
}

bool GetLimits::parseParameters(BlockInformation* blockInfo)
{
    if (!WBBlock::parseParameters(blockInfo)) {
        wbtError << "Failed to parse WBBlock parameters.";
        return false;
    }

    ParameterMetadata limitType(PARAM_DOUBLE, PARAM_IDX_LIMIT_SRC, 1, 1, "LimitType");

    bool ok = blockInfo->addParameterMetadata(limitType);

    if (!ok) {
        wbtError << "Failed to store parameters metadata.";
        return false;
    }

    return blockInfo->parseParameters(m_parameters);
}

bool GetLimits::configureSizeAndPorts(BlockInformation* blockInfo)
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
    // 1) vector with the information asked (1xDoFs)
    //

    if (!blockInfo->setNumberOfOutputPorts(2)) {
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

    bool success = true;
    success = success && blockInfo->setOutputPortVectorSize(0, dofs); // Min limit
    success = success && blockInfo->setOutputPortVectorSize(1, dofs); // Max limit

    blockInfo->setOutputPortType(0, DataType::DOUBLE);
    blockInfo->setOutputPortType(1, DataType::DOUBLE);

    if (!success) {
        wbtError << "Failed to configure output ports.";
        return false;
    }

    return true;
}

bool GetLimits::initialize(BlockInformation* blockInfo)
{
    using namespace yarp::os;

    if (!WBBlock::initialize(blockInfo)) {
        return false;
    }

    // INPUT PARAMETERS
    // ================

    if (!parseParameters(blockInfo)) {
        wbtError << "Failed to parse parameters.";
        return false;
    }

    // Read the control type
    if (!m_parameters.getParameter("LimitType", m_limitType)) {
        wbtError << "Failed to get parameters after their parsing.";
        return false;
    }

    // Get the DoFs
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }
    const auto& configuration = robotInterface->getConfiguration();
    const auto dofs = configuration.getNumberOfDoFs();

    // Initialize the structure that stores the limits
    m_limits.reset(new Limit(dofs));

    // Initializes some buffers
    double min = 0;
    double max = 0;

    // From the RemoteControlBoardRemapper
    // ===================================
    //
    // In the next methods, the values are asked using joint index and not string.
    // The ControlBoardRemapper internally uses the same joints ordering of its
    // initialization. In this case, it matches 1:1 the controlled joint vector.
    // It is hence possible using i to point to the correct joint.

    // Get the RemoteControlBoardRemapper and IControlLimits2 interface if needed
    yarp::dev::IControlLimits2* iControlLimits2 = nullptr;
    if (m_limitType == "ControlBoardPosition" || m_limitType == "ControlBoardVelocity") {
        // Retain the control board remapper
        if (!robotInterface->retainRemoteControlBoardRemapper()) {
            wbtError << "Couldn't retain the RemoteControlBoardRemapper.";
            return false;
        }
        // Get the interface
        if (!robotInterface->getInterface(iControlLimits2) || !iControlLimits2) {
            wbtError << "Failed to get IControlLimits2 interface.";
            return false;
        }
    }

    if (m_limitType == "ControlBoardPosition") {
        for (auto i = 0; i < dofs; ++i) {
            if (!iControlLimits2->getLimits(i, &min, &max)) {
                wbtError << "Failed to get limits from the interface.";
                return false;
            }
            m_limits->m_min[i] = deg2rad(min);
            m_limits->m_max[i] = deg2rad(max);
        }
    }
    else if (m_limitType == "ControlBoardVelocity") {
        for (auto i = 0; i < dofs; ++i) {
            if (!iControlLimits2->getVelLimits(i, &min, &max)) {
                wbtError << "Failed to get limits from the interface.";
                return false;
            }
            m_limits->m_min[i] = deg2rad(min);
            m_limits->m_max[i] = deg2rad(max);
        }
    }

    // From the URDF model
    // ===================
    //
    // For the time being, only position limits are supported.

    else if (m_limitType == "ModelPosition") {
        iDynTree::IJointConstPtr p_joint;

        // Get the KinDynComputations pointer
        const auto& kindyncomp = robotInterface->getKinDynComputations();
        if (!kindyncomp) {
            wbtError << "Failed to retrieve the KinDynComputations object.";
            return false;
        }

        // Get the model
        const iDynTree::Model model = kindyncomp->model();

        for (unsigned i = 0; i < dofs; ++i) {
            // Get the joint name
            const std::string joint = configuration.getControlledJoints()[i];

            // Get its index
            iDynTree::JointIndex jointIndex = model.getJointIndex(joint);

            if (jointIndex == iDynTree::JOINT_INVALID_INDEX) {
                wbtError << "Invalid iDynTree joint index.";
                return false;
            }

            // Get the joint from the model
            p_joint = model.getJoint(jointIndex);

            if (!p_joint->hasPosLimits()) {
                wbtWarning << "Joint " << joint << " has no model limits.";
                m_limits->m_min[i] = -std::numeric_limits<double>::infinity();
                m_limits->m_max[i] = std::numeric_limits<double>::infinity();
            }
            else {
                if (!p_joint->getPosLimits(0, min, max)) {
                    wbtError << "Failed to get joint limits from the URDF model "
                             << "for the joint " << joint + ".";
                    return false;
                }
                m_limits->m_min[i] = min;
                m_limits->m_max[i] = max;
            }
        }
    }
    // TODO: other limits from the model?
    // else if (limitType == "ModelVelocity") {
    // }
    // else if (limitType == "ModelEffort") {
    // }
    else {
        wbtError << "Limit type " + m_limitType + " not recognized.";
        return false;
    }

    return true;
}

bool GetLimits::terminate(const BlockInformation* blockInfo)
{
    bool ok = true;

    // Release the RemoteControlBoardRemapper
    if (m_limitType == "ControlBoardPosition" || m_limitType == "ControlBoardVelocity") {
        auto robotInterface = getRobotInterface(blockInfo).lock();
        if (!robotInterface || !robotInterface->releaseRemoteControlBoardRemapper()) {
            wbtError << "Failed to release the RemoteControlBoardRemapper.";
            // Don't return false here. WBBlock::terminate must be called in any case
        }
    }

    return ok && WBBlock::terminate(blockInfo);
}

bool GetLimits::output(const BlockInformation* blockInfo)
{
    if (!m_limits) {
        return false;
    }

    Signal minPort = blockInfo->getOutputPortSignal(0);
    Signal maxPort = blockInfo->getOutputPortSignal(1);

    if (!minPort.isValid() || !maxPort.isValid()) {
        wbtError << "Output signals not valid.";
        return false;
    }

    // Get the Configuration
    auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }
    const auto dofs = robotInterface->getConfiguration().getNumberOfDoFs();

    bool ok = true;

    ok = ok && minPort.setBuffer(m_limits->m_min.data(), dofs);
    ok = ok && maxPort.setBuffer(m_limits->m_max.data(), dofs);

    if (!ok) {
        wbtError << "Failed to set output buffers.";
        return false;
    }

    return true;
}
