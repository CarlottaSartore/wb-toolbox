#ifndef WBT_JACOBIAN_H
#define WBT_JACOBIAN_H

#include "WBBlock.h"
#include <iDynTree/Model/Indices.h>
#include <memory>

namespace wbt {
    class Jacobian;
}

namespace iDynTree {
    class MatrixDynSize;
}

class wbt::Jacobian : public wbt::WBBlock
{
private:
    // Support variables
    std::unique_ptr<iDynTree::MatrixDynSize> m_jacobianCOM;

    // Output
    std::unique_ptr<iDynTree::MatrixDynSize> m_jacobian;

    // Other variables
    bool m_frameIsCoM;
    iDynTree::FrameIndex m_frameIndex;

public:
    static const std::string ClassName;
    Jacobian();
    ~Jacobian() override = default;

    unsigned numberOfParameters() override;
    bool configureSizeAndPorts(BlockInformation* blockInfo) override;
    bool initialize(BlockInformation* blockInfo) override;
    bool terminate(const BlockInformation* blockInfo) override;
    bool output(const BlockInformation* blockInfo) override;
};

#endif /* WBT_JACOBIAN_H */
