/*
 * Copyright (C) 2018 Istituto Italiano di Tecnologia (IIT)
 * All rights reserved.
 *
 * This software may be modified and distributed under the terms of the
 * GNU Lesser General Public License v2.1 or any later version.
 */

#ifndef WBT_MASSMATRIX_H
#define WBT_MASSMATRIX_H

#include "WBBlock.h"
#include <memory>

namespace wbt {
    class MassMatrix;
}

namespace iDynTree {
    class MatrixDynSize;
}

class wbt::MassMatrix : public wbt::WBBlock
{
private:
    std::unique_ptr<iDynTree::MatrixDynSize> m_massMatrix;

public:
    static const std::string ClassName;

    MassMatrix();
    ~MassMatrix() override = default;

    bool configureSizeAndPorts(BlockInformation* blockInfo) override;
    bool initialize(BlockInformation* blockInfo) override;
    bool terminate(const BlockInformation* blockInfo) override;
    bool output(const BlockInformation* blockInfo) override;
};

#endif // WBT_MASSMATRIX_H
