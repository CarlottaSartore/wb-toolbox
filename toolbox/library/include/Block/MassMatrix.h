/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#ifndef WBT_MASSMATRIX_H
#define WBT_MASSMATRIX_H

#include "Base/WBBlock.h"

#include <memory>
#include <string>

namespace wbt {
    class BlockInformation;
    class MassMatrix;
} // namespace wbt

/**
 * @brief The wbt::MassMatrix class
 */
class wbt::MassMatrix final : public wbt::WBBlock
{
private:
    class impl;
    std::unique_ptr<impl> pImpl;

public:
    MassMatrix();
    ~MassMatrix() override;

    bool configureSizeAndPorts(BlockInformation* blockInfo) override;
    bool initialize(BlockInformation* blockInfo) override;
    bool terminate(const BlockInformation* blockInfo) override;
    bool output(const BlockInformation* blockInfo) override;
};

#endif // WBT_MASSMATRIX_H
