#include "CentroidalMomentum.h"

#include "BlockInformation.h"
#include "Signal.h"
#include "Error.h"
#include "WBInterface.h"
#include <yarpWholeBodyInterface/yarpWbiUtil.h>
#include <wbi/wholeBodyInterface.h>
#include <Eigen/Core>

namespace wbt {

    std::string CentroidalMomentum::ClassName = "CentroidalMomentum";

    CentroidalMomentum::CentroidalMomentum()
    : m_basePose(0)
    , m_centroidalMomentum(0)
    , m_basePoseRaw(0)
    , m_configuration(0)
    , m_baseVelocity(0)
    , m_jointsVelocity(0) {}

    bool CentroidalMomentum::configureSizeAndPorts(BlockInformation *blockInfo, wbt::Error *error)
    {
        if (!WBIBlock::configureSizeAndPorts(blockInfo, error)) {
            return false;
        }

        unsigned dofs = WBInterface::sharedInstance().numberOfDoFs();

        // Specify I/O
        // Input ports:
        // - 4x4 matrix (homogenous transformation for the base pose w.r.t. world)
        // - DoFs vector for the robot (joints) configurations

        if (!blockInfo->setNumberOfInputPorts(4)) {
            if (error) error->message = "Failed to configure the number of input ports";
            return false;
        }
        bool success = true;

        success = success && blockInfo->setInputPortMatrixSize(0, 4, 4); //base pose
        success = success && blockInfo->setInputPortVectorSize(1, dofs); //joint configuration
        success = success && blockInfo->setInputPortVectorSize(2, 6); //base velocity
        success = success && blockInfo->setInputPortVectorSize(3, dofs); //joints velocitity

        blockInfo->setInputPortType(0, PortDataTypeDouble);
        blockInfo->setInputPortType(1, PortDataTypeDouble);
        blockInfo->setInputPortType(2, PortDataTypeDouble);
        blockInfo->setInputPortType(3, PortDataTypeDouble);

        if (!success) {
            if (error) error->message = "Failed to configure input ports";
            return false;
        }

        // Output port:
        // - 6 vector representing the centroidal momentum
        if (!blockInfo->setNumberOfOuputPorts(1)) {
            if (error) error->message = "Failed to configure the number of output ports";
            return false;
        }

        success = blockInfo->setOutputPortVectorSize(0, 6);
        blockInfo->setOutputPortType(0, PortDataTypeDouble);

        return success;
    }

    bool CentroidalMomentum::initialize(BlockInformation *blockInfo, wbt::Error *error)
    {
        using namespace yarp::os;
        if (!WBIModelBlock::initialize(blockInfo, error)) return false;

        unsigned dofs = WBInterface::sharedInstance().numberOfDoFs();
        m_basePose = new double[16];
        m_centroidalMomentum = new double[6];
        m_basePoseRaw = new double[16];
        m_configuration = new double[dofs];
        m_baseVelocity = new double[6];
        m_jointsVelocity = new double[dofs];

        return m_basePose && m_centroidalMomentum && m_basePoseRaw && m_configuration && m_baseVelocity && m_jointsVelocity;
    }

    bool CentroidalMomentum::terminate(BlockInformation *blockInfo, wbt::Error *error)
    {
        if (m_basePose) {
            delete [] m_basePose;
            m_basePose = 0;
        }
        if (m_centroidalMomentum) {
            delete [] m_centroidalMomentum;
            m_centroidalMomentum = 0;
        }
        if (m_basePoseRaw) {
            delete [] m_basePoseRaw;
            m_basePoseRaw = 0;
        }
        if (m_configuration) {
            delete [] m_configuration;
            m_configuration = 0;
        }
        if (m_baseVelocity) {
            delete [] m_baseVelocity;
            m_baseVelocity = 0;
        }
        if (m_jointsVelocity) {
            delete [] m_jointsVelocity;
            m_jointsVelocity = 0;
        }

        return WBIModelBlock::terminate(blockInfo, error);
    }

    bool CentroidalMomentum::output(BlockInformation *blockInfo, wbt::Error */*error*/)
    {
        //get input
        wbi::iWholeBodyModel * const interface = WBInterface::sharedInstance().model();
        if (interface) {
            Signal basePoseRaw = blockInfo->getInputPortSignal(0);
            Signal configuration = blockInfo->getInputPortSignal(1);
            Signal baseVelocity = blockInfo->getInputPortSignal(2);
            Signal jointsVelocity = blockInfo->getInputPortSignal(3);

            for (unsigned i = 0; i < blockInfo->getInputPortWidth(0); ++i) {
                m_basePoseRaw[i] = basePoseRaw.get(i).doubleData();
            }
            for (unsigned i = 0; i < blockInfo->getInputPortWidth(1); ++i) {
                m_configuration[i] = configuration.get(i).doubleData();
            }
            for (unsigned i = 0; i < blockInfo->getInputPortWidth(2); ++i) {
                m_baseVelocity[i] = baseVelocity.get(i).doubleData();
            }
            for (unsigned i = 0; i < blockInfo->getInputPortWidth(3); ++i) {
                m_jointsVelocity[i] = jointsVelocity.get(i).doubleData();
            }

            Eigen::Map<Eigen::Matrix<double, 4, 4, Eigen::ColMajor> > basePoseColMajor(m_basePoseRaw);
            Eigen::Map<Eigen::Matrix<double, 4, 4, Eigen::RowMajor> > basePose(m_basePose);
            basePose = basePoseColMajor;

            wbi::Frame frame;
            wbi::frameFromSerialization(basePose.data(), frame);

            interface->computeCentroidalMomentum(m_configuration,
                                                 frame,
                                                 m_jointsVelocity,
                                                 m_baseVelocity,
                                                 m_centroidalMomentum);

            Signal output = blockInfo->getOutputPortSignal(0);
            output.setBuffer(m_centroidalMomentum, blockInfo->getOutputPortWidth(0));

            return true;
        }
        return false;
    }
}
