/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#include "MassMatrix.h"
#include "BlockInformation.h"
#include "Configuration.h"
#include "Log.h"
#include "RobotInterface.h"
#include "Signal.h"

#include <Eigen/Core>
#include <iDynTree/Core/EigenHelpers.h>
#include <iDynTree/Core/MatrixDynSize.h>
#include <iDynTree/KinDynComputations.h>

#include <ostream>

using namespace wbt;

const std::string MassMatrix::ClassName = "MassMatrix";

const unsigned INPUT_IDX_BASE_POSE = 0;
const unsigned INPUT_IDX_JOINTCONF = 1;
const unsigned OUTPUT_IDX_MASS_MAT = 0;

class MassMatrix::impl
{
public:
    iDynTree::MatrixDynSize massMatrix;
};

MassMatrix::MassMatrix()
    : pImpl{new impl()}
{}

bool MassMatrix::configureSizeAndPorts(BlockInformation* blockInfo)
{
    if (!WBBlock::configureSizeAndPorts(blockInfo)) {
        return false;
    }

    // INPUTS
    // ======
    //
    // 1) Homogeneous transform for base pose wrt the world frame (4x4 matrix)
    // 2) Joints position (1xDoFs vector)
    //

    // Number of inputs
    if (!blockInfo->setNumberOfInputPorts(2)) {
        wbtError << "Failed to configure the number of input ports.";
        return false;
    }

    // Get the DoFs
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }
    const auto dofs = robotInterface->getConfiguration().getNumberOfDoFs();

    // Size and type
    bool success = true;
    success = success && blockInfo->setInputPortMatrixSize(INPUT_IDX_BASE_POSE, {4, 4});
    success = success && blockInfo->setInputPortVectorSize(INPUT_IDX_JOINTCONF, dofs);

    blockInfo->setInputPortType(INPUT_IDX_BASE_POSE, DataType::DOUBLE);
    blockInfo->setInputPortType(INPUT_IDX_JOINTCONF, DataType::DOUBLE);

    if (!success) {
        wbtError << "Failed to configure input ports.";
        return false;
    }

    // OUTPUTS
    // =======
    //
    // 1) Matrix epresenting the mass matrix (DoFs+6)x(DoFs+6)
    //

    // Number of outputs
    if (!blockInfo->setNumberOfOutputPorts(1)) {
        wbtError << "Failed to configure the number of output ports.";
        return false;
    }

    // Size and type
    success = blockInfo->setOutputPortMatrixSize(OUTPUT_IDX_MASS_MAT, {dofs + 6, dofs + 6});
    blockInfo->setOutputPortType(OUTPUT_IDX_MASS_MAT, DataType::DOUBLE);

    return success;
}

bool MassMatrix::initialize(BlockInformation* blockInfo)
{
    if (!WBBlock::initialize(blockInfo)) {
        return false;
    }

    // Get the DoFs
    const auto robotInterface = getRobotInterface(blockInfo).lock();
    if (!robotInterface) {
        wbtError << "RobotInterface has not been correctly initialized.";
        return false;
    }
    const auto dofs = robotInterface->getConfiguration().getNumberOfDoFs();

    // Output
    pImpl->massMatrix.resize(6 + dofs, 6 + dofs);
    pImpl->massMatrix.zero();

    return true;
}

bool MassMatrix::terminate(const BlockInformation* blockInfo)
{
    return WBBlock::terminate(blockInfo);
}

bool MassMatrix::output(const BlockInformation* blockInfo)
{
    using namespace Eigen;
    using namespace iDynTree;
    using MatrixXdSimulink = Matrix<double, Dynamic, Dynamic, Eigen::ColMajor>;
    using MatrixXdiDynTree = Matrix<double, Dynamic, Dynamic, Eigen::RowMajor>;

    // Get the KinDynComputations object
    auto kinDyn = getKinDynComputations(blockInfo).lock();
    if (!kinDyn) {
        wbtError << "Failed to retrieve the KinDynComputations object.";
        return false;
    }

    if (!kinDyn) {
        wbtError << "Failed to retrieve the KinDynComputations object.";
        return false;
    }

    // GET THE SIGNALS POPULATE THE ROBOT STATE
    // ========================================

    const Signal basePoseSig = blockInfo->getInputPortSignal(INPUT_IDX_BASE_POSE);
    const Signal jointsPosSig = blockInfo->getInputPortSignal(INPUT_IDX_JOINTCONF);

    if (!basePoseSig.isValid() || !jointsPosSig.isValid()) {
        wbtError << "Input signals not valid.";
        return false;
    }

    bool ok = setRobotState(&basePoseSig, &jointsPosSig, nullptr, nullptr, kinDyn.get());

    if (!ok) {
        wbtError << "Failed to set the robot state.";
        return false;
    }

    // OUTPUT
    // ======

    // Compute the Mass Matrix
    kinDyn->getFreeFloatingMassMatrix(pImpl->massMatrix);

    // Get the output signal memory location
    Signal output = blockInfo->getOutputPortSignal(OUTPUT_IDX_MASS_MAT);
    if (!output.isValid()) {
        wbtError << "Output signal not valid.";
        return false;
    }

    // Allocate objects for row-major -> col-major conversion
    Map<MatrixXdiDynTree> massMatrixRowMajor = toEigen(pImpl->massMatrix);
    Map<MatrixXdSimulink> massMatrixColMajor(
        output.getBuffer<double>(),
        blockInfo->getOutputPortMatrixSize(OUTPUT_IDX_MASS_MAT).first,
        blockInfo->getOutputPortMatrixSize(OUTPUT_IDX_MASS_MAT).second);

    // Forward the buffer to Simulink transforming it to ColMajor
    massMatrixColMajor = massMatrixRowMajor;
    return true;
}
