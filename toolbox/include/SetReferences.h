/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#ifndef WBT_SETREFERENCES_H
#define WBT_SETREFERENCES_H

#include "WBBlock.h"
#include <vector>

namespace wbt {
    class SetReferences;
}

/**
 * @brief The wbt::SetReferences class
 *
 * @section Parameters
 *
 * In addition to @ref wbblock_parameters, wbt::SetReferences requires:
 *
 * | Type | Index | Rows  | Cols  | Name  |
 * | ---- | :---: | :---: | :---: | ----- |
 * | ::STRING | 0 + WBBlock::NumberOfParameters | 1 | 1 | "CtrlType" |
 * | ::DOUBLE | 0 + WBBlock::NumberOfParameters | 1 | 1 | "RefSpeed" |
 *
 */
class wbt::SetReferences : public wbt::WBBlock
{
private:
    std::vector<int> m_controlModes;
    bool m_resetControlMode = true;
    double m_refSpeed;
    static const std::vector<double> rad2deg(const double* buffer, const unsigned width);

public:
    static const std::string ClassName;

    SetReferences() = default;
    ~SetReferences() override = default;

    unsigned numberOfParameters() override;
    bool parseParameters(BlockInformation* blockInfo) override;
    bool configureSizeAndPorts(BlockInformation* blockInfo) override;
    bool initialize(BlockInformation* blockInfo) override;
    bool initializeInitialConditions(const BlockInformation* blockInfo) override;
    bool terminate(const BlockInformation* blockInfo) override;
    bool output(const BlockInformation* blockInfo) override;
};

#endif // WBT_SETREFERENCES_H
