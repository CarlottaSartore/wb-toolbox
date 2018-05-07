/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#ifndef WBT_SIMULINKBLOCKINFORMATION_H
#define WBT_SIMULINKBLOCKINFORMATION_H

#include "AnyType.h"
#include "BlockInformation.h"
#include "Signal.h"
#include <simstruc.h>
#include <vector>

namespace wbt {
    class SimulinkBlockInformation;
} // namespace wbt

class wbt::SimulinkBlockInformation final : public wbt::BlockInformation
{
private:
    SimStruct* simstruct;
    std::string m_confBlockName;
    std::vector<wbt::ParameterMetadata> m_paramsMetadata;

    DataType mapSimulinkToPortType(const DTypeId& typeId) const;
    DTypeId mapPortTypeToSimulink(const DataType& dataType) const;

public:
    using ParameterIndex = unsigned;

    SimulinkBlockInformation(SimStruct* simstruct);
    ~SimulinkBlockInformation() override = default;

    // BLOCK OPTIONS METHODS
    // =====================

    bool optionFromKey(const std::string& key, double& option) const override;

    // PARAMETERS METHODS
    // ==================

    bool addParameterMetadata(const wbt::ParameterMetadata& paramMD) override;
    bool parseParameters(wbt::Parameters& parameters) override;

    // PORT INFORMATION SETTERS
    // ========================

    bool setNumberOfInputPorts(const unsigned& numberOfPorts) override;
    bool setNumberOfOutputPorts(const unsigned& numberOfPorts) override;
    bool setInputPortVectorSize(const PortIndex& idx, const VectorSize& size) override;
    bool setInputPortMatrixSize(const PortIndex& idx, const MatrixSize& size) override;
    bool setOutputPortVectorSize(const PortIndex& idx, const VectorSize& size) override;
    bool setOutputPortMatrixSize(const PortIndex& idx, const MatrixSize& size) override;
    bool setInputPortType(const PortIndex& idx, const DataType& portType) override;
    bool setOutputPortType(const PortIndex& idx, const DataType& portType) override;

    // PORT INFORMATION GETTERS
    // ========================

    unsigned getInputPortWidth(const PortIndex& idx) const override;
    unsigned getOutputPortWidth(const PortIndex& idx) const override;
    wbt::Signal
    getInputPortSignal(const PortIndex& idx,
                       const VectorSize& size = wbt::Signal::DynamicSize) const override;
    wbt::Signal
    getOutputPortSignal(const PortIndex& idx,
                        const VectorSize& size = wbt::Signal::DynamicSize) const override;
    MatrixSize getInputPortMatrixSize(const PortIndex& idx) const override;
    MatrixSize getOutputPortMatrixSize(const PortIndex& idx) const override;

    std::weak_ptr<wbt::RobotInterface> getRobotInterface() const override;
    std::weak_ptr<iDynTree::KinDynComputations> getKinDynComputations() const override;

    // METHODS OUTSIDE THE INTERFACE
    // =============================

    // Scalar parameters
    bool getScalarParameterAtIndex(const ParameterIndex& idx, double& value) const;
    bool getBooleanParameterAtIndex(const ParameterIndex& idx, bool& value) const;
    bool getStringParameterAtIndex(const ParameterIndex& idx, std::string& value) const;

    // Struct parameters
    bool getStringFieldAtIndex(const ParameterIndex& idx,
                               const std::string& fieldName,
                               std::string& value) const;
    bool getScalarFieldAtIndex(const ParameterIndex& idx,
                               const std::string& fieldName,
                               double& value) const;
    bool getBooleanFieldAtIndex(const ParameterIndex& idx,
                                const std::string& fieldName,
                                bool& value) const;
    bool getCellFieldAtIndex(const ParameterIndex& idx,
                             const std::string& fieldName,
                             AnyCell& value) const;
    bool getVectorDoubleFieldAtIndex(const ParameterIndex& idx,
                                     const std::string& fieldName,
                                     std::vector<double>& value) const;

    // Cell / Struct / Vector paramters
    bool getCellAtIndex(const ParameterIndex& idx, AnyCell& value) const;
    bool getStructAtIndex(const ParameterIndex& idx, AnyStruct& value) const;
    bool getVectorAtIndex(const ParameterIndex& idx, std::vector<double>& value) const;
};

#endif /* WBT_SIMULINKBLOCKINFORMATION_H */
