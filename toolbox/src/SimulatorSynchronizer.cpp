#include "SimulatorSynchronizer.h"
#include "Error.h"
#include "BlockInformation.h"

#include <yarp/os/Network.h>
#include <yarp/os/Port.h>
#include <thrift/ClockServer.h>
#include <cmath>

#define PARAM_IDX_1 1                           // Period
#define PARAM_IDX_2 2                           // Gazebo clock port
#define PARAM_IDX_3 3                           // RPC client port name


namespace wbt {

    struct SimulatorSynchronizer::RPCData
    {
        struct {
            std::string clientPortName;
            std::string serverPortName;

            unsigned numberOfSteps;
        } configuration;

        yarp::os::Port clientPort;
        gazebo::ClockServer clockServer;
    };
    
    std::string SimulatorSynchronizer::ClassName = "SimulatorSynchronizer";

    SimulatorSynchronizer::SimulatorSynchronizer()
    : m_period(0.01)
    , m_firstRun(true)
    , m_rpcData(0) {}

    SimulatorSynchronizer::~SimulatorSynchronizer() {}
    
    unsigned SimulatorSynchronizer::numberOfParameters() { return 3; }

    std::vector<std::string> SimulatorSynchronizer::additionalBlockOptions()
    {
        return std::vector<std::string>(1, wbt::BlockOptionPrioritizeOrder);
    }

    bool SimulatorSynchronizer::configureSizeAndPorts(BlockInformation *blockInfo, wbt::Error *error)
    {
        // Specify I/O
        // INPUTS
        if (!blockInfo->setNumberOfInputPorts(0)) {
            if (error) error->message = "Failed to set input port number to 0";
            return false;
        }

        if (!blockInfo->setNumberOfOuputPorts(0)) {
            if (error) error->message = "Failed to set output port number";
            return false;
        }

        return true;
    }

    bool SimulatorSynchronizer::initialize(BlockInformation *blockInfo, wbt::Error *error)
    {
        m_period = blockInfo->getScalarParameterAtIndex(PARAM_IDX_1).doubleData();
        std::string serverPortName;
        std::string clientPortName;

        if (!blockInfo->getStringParameterAtIndex(PARAM_IDX_2, serverPortName) || !blockInfo->getStringParameterAtIndex(PARAM_IDX_3, clientPortName)) {
            if (error) error->message = "Error reading RPC parameters";
            return false;
        }

        yarp::os::Network::init();
        if (!yarp::os::Network::initialized() || !yarp::os::Network::checkNetwork()) {
            if (error) error->message = "Error initializing Yarp network";
            return false;
        }

        m_rpcData = new struct RPCData();
        if (!m_rpcData) {
            if (error) error->message = "Error creating RPC data structure";
            return false;
        }

        m_rpcData->configuration.clientPortName = clientPortName;
        m_rpcData->configuration.serverPortName = serverPortName;

        m_firstRun = true;

        return true;
    }
    
    bool SimulatorSynchronizer::terminate(BlockInformation */*S*/, wbt::Error *error)
    {
        if (m_rpcData) {
            if (m_rpcData->clientPort.isOpen()) {
                m_rpcData->clockServer.continueSimulation();
                if (!yarp::os::Network::disconnect(m_rpcData->configuration.clientPortName, m_rpcData->configuration.serverPortName)) {
                    if (error) error->message = "Error disconnecting from simulator clock server";
                }
                m_rpcData->clientPort.close();
            }
            delete m_rpcData;
            m_rpcData = 0;
        }
        yarp::os::Network::fini();
        return true;
    }

    bool SimulatorSynchronizer::output(BlockInformation */*S*/, wbt::Error *error)
    {
        if (m_firstRun) {
            m_firstRun = false;

            if (!m_rpcData->clientPort.open(m_rpcData->configuration.clientPortName)
                || !yarp::os::Network::connect(m_rpcData->configuration.clientPortName, m_rpcData->configuration.serverPortName)) {
                if (error) error->message = "Error connecting to simulator clock server";
                return false;
            }

            m_rpcData->clockServer.yarp().attachAsClient(m_rpcData->clientPort);

            double stepSize = m_rpcData->clockServer.getStepSize();
            m_rpcData->configuration.numberOfSteps = static_cast<unsigned>(m_period / stepSize);
            m_rpcData->clockServer.pauseSimulation();
        }
        m_rpcData->clockServer.stepSimulationAndWait(m_rpcData->configuration.numberOfSteps);

        return true;
    }
}
