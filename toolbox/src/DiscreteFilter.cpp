#include "DiscreteFilter.h"
#include "BlockInformation.h"
#include "Log.h"
#include "Parameters.h"
#include "Signal.h"

#include <iCub/ctrl/filters.h>
#include <yarp/sig/Vector.h>

#include <algorithm>
#include <cassert>
#include <sstream>
#include <string>

using namespace wbt;
using namespace iCub::ctrl;
using namespace yarp::sig;

const std::string DiscreteFilter::ClassName = "DiscreteFilter";

// Parameters
const unsigned PARAM_IDX_BIAS = Block::NumberOfParameters - 1;
const unsigned PARAM_IDX_STRUCT = PARAM_IDX_BIAS + 1; // Struct containing filter parameters

// Inputs / Outputs
const unsigned INPUT_IDX_SIGNAL = 0;
const unsigned OUTPUT_IDX_SIGNAL = 0;

// Cannot use = default due to yarp::sig::Vector instantiation
DiscreteFilter::DiscreteFilter() {}

unsigned DiscreteFilter::numberOfParameters()
{
    return Block::numberOfParameters() + 1;
}

bool DiscreteFilter::parseParameters(BlockInformation* blockInfo)
{
    ParameterMetadata paramMD_1lowp_fc(PARAM_STRUCT_DOUBLE, PARAM_IDX_STRUCT, 1, 1, "Fc");
    ParameterMetadata paramMD_1lowp_ts(PARAM_STRUCT_DOUBLE, PARAM_IDX_STRUCT, 1, 1, "Ts");
    ParameterMetadata paramMD_md_order(PARAM_STRUCT_INT, PARAM_IDX_STRUCT, 1, 1, "MedianOrder");
    ParameterMetadata paramMD_flt_type(PARAM_STRUCT_STRING, PARAM_IDX_STRUCT, 1, 1, "FilterType");

    ParameterMetadata paramMD_numcoeff(
        PARAM_STRUCT_DOUBLE, PARAM_IDX_STRUCT, 1, ParameterMetadata::DynamicSize, "NumCoeffs");

    ParameterMetadata paramMD_dencoeff(
        PARAM_STRUCT_DOUBLE, PARAM_IDX_STRUCT, 1, ParameterMetadata::DynamicSize, "DenCoeffs");

    ParameterMetadata paramMD_init_status(PARAM_STRUCT_BOOL, PARAM_IDX_STRUCT, 1, 1, "InitStatus");

    ParameterMetadata paramMD_init_y0(
        PARAM_STRUCT_DOUBLE, PARAM_IDX_STRUCT, 1, ParameterMetadata::DynamicSize, "y0");

    ParameterMetadata paramMD_init_u0(
        PARAM_STRUCT_DOUBLE, PARAM_IDX_STRUCT, 1, ParameterMetadata::DynamicSize, "u0");

    bool ok = true;
    ok = ok && blockInfo->addParameterMetadata(paramMD_1lowp_fc);
    ok = ok && blockInfo->addParameterMetadata(paramMD_1lowp_ts);
    ok = ok && blockInfo->addParameterMetadata(paramMD_md_order);
    ok = ok && blockInfo->addParameterMetadata(paramMD_flt_type);
    ok = ok && blockInfo->addParameterMetadata(paramMD_numcoeff);
    ok = ok && blockInfo->addParameterMetadata(paramMD_dencoeff);
    ok = ok && blockInfo->addParameterMetadata(paramMD_init_status);
    ok = ok && blockInfo->addParameterMetadata(paramMD_init_y0);
    ok = ok && blockInfo->addParameterMetadata(paramMD_init_u0);

    if (!ok) {
        wbtError << "Failed to store parameters metadata.";
        return false;
    }

    return blockInfo->parseParameters(m_parameters);
}

bool DiscreteFilter::configureSizeAndPorts(BlockInformation* blockInfo)
{
    if (!Block::initialize(blockInfo)) {
        return false;
    }

    // INPUTS
    // ======
    //
    // 1) The input signal (1xn)
    //

    int numberOfInputPorts = 1;
    if (!blockInfo->setNumberOfInputPorts(numberOfInputPorts)) {
        wbtError << "Failed to set input port number.";
        return false;
    }

    blockInfo->setInputPortVectorSize(INPUT_IDX_SIGNAL, Signal::DynamicSize);
    blockInfo->setInputPortType(INPUT_IDX_SIGNAL, DataType::DOUBLE);

    // OUTPUTS
    // =======
    //
    // 1) The output signal (1xn)
    //

    int numberOfOutputPorts = 1;
    if (!blockInfo->setNumberOfOutputPorts(numberOfOutputPorts)) {
        wbtError << "Failed to set output port number.";
        return false;
    }

    blockInfo->setOutputPortVectorSize(OUTPUT_IDX_SIGNAL, Signal::DynamicSize);
    blockInfo->setOutputPortType(OUTPUT_IDX_SIGNAL, DataType::DOUBLE);

    return true;
}

bool DiscreteFilter::initialize(BlockInformation* blockInfo)
{
    if (!Block::initialize(blockInfo)) {
        return false;
    }

    // PARAMETERS
    // ==========

    if (!parseParameters(blockInfo)) {
        wbtError << "Failed to parse parameters.";
        return false;
    }

    std::string filter_type;
    std::vector<double> num_coeff;
    std::vector<double> den_coeff;
    std::vector<double> y0;
    std::vector<double> u0;
    double firstOrderLowPassFilter_fc = 0.0;
    double firstOrderLowPassFilter_ts = 0.0;
    int medianFilter_order = 0;
    bool initStatus = false;

    bool ok = true;
    ok = ok && m_parameters.getParameter("FilterType", filter_type);
    ok = ok && m_parameters.getParameter("NumCoeffs", num_coeff);
    ok = ok && m_parameters.getParameter("DenCoeffs", den_coeff);
    ok = ok && m_parameters.getParameter("y0", y0);
    ok = ok && m_parameters.getParameter("u0", u0);
    ok = ok && m_parameters.getParameter("InitStatus", initStatus);
    ok = ok && m_parameters.getParameter("Fc", firstOrderLowPassFilter_fc);
    ok = ok && m_parameters.getParameter("Ts", firstOrderLowPassFilter_ts);
    ok = ok && m_parameters.getParameter("MedianOrder", medianFilter_order);

    if (!ok) {
        wbtError << "Failed to get parameters after their parsing.";
        return false;
    }

    // DYNAMICALLY SIZED SIGNALS
    // =========================

    const unsigned inputSignalSize = blockInfo->getInputPortWidth(0);
    blockInfo->setInputPortVectorSize(0, inputSignalSize);
    blockInfo->setOutputPortVectorSize(0, inputSignalSize);

    // CLASS INITIALIZATION
    // ====================

    // Check if numerator and denominator are not empty
    if (num_coeff.empty() || den_coeff.empty()) {
        wbtError << "Empty numerator or denominator not allowed.";
        return false;
    }
    // Check if numerator or denominator are scalar and zero
    if ((num_coeff.size() == 1 && num_coeff.front() == 0.0)
        || (den_coeff.size() == 1 && den_coeff.front() == 0.0)) {
        wbtError << "Passed numerator or denominator not valid.";
        return false;
    }

    // Convert the std::vector to yarp::sig::Vector
    yarp::sig::Vector num(num_coeff.size(), num_coeff.data());
    yarp::sig::Vector den(den_coeff.size(), den_coeff.data());

    // Get the width of the input vector
    const unsigned inputSignalWidth = blockInfo->getInputPortWidth(INPUT_IDX_SIGNAL);

    if (initStatus) {
        // y0 and output signal dimensions should match
        unsigned outputSignalWidth = blockInfo->getOutputPortWidth(OUTPUT_IDX_SIGNAL);
        if (y0.size() != outputSignalWidth) {
            wbtError << "y0 and output signal sizes don't match.";
            return false;
        }
        // u0 and input signal dimensions should match (used only for Generic)
        if ((filter_type == "Generic") && (u0.size() != inputSignalWidth)) {
            wbtError << "(Generic) u0 and input signal sizes don't match.";
            return false;
        }
        // Allocate the initial conditions
        m_y0 = std::unique_ptr<yarp::sig::Vector>(new Vector(y0.size(), y0.data()));
        m_u0 = std::unique_ptr<yarp::sig::Vector>(new Vector(u0.size(), u0.data()));
    }
    else {
        // Initialize zero initial conditions
        m_y0 = std::unique_ptr<yarp::sig::Vector>(new Vector(y0.size(), 0.0));
        m_u0 = std::unique_ptr<yarp::sig::Vector>(new Vector(u0.size(), 0.0));
    }

    // Create the filter object
    // ========================

    // Generic
    // -------
    if (filter_type == "Generic") {
        m_filter = std::unique_ptr<IFilter>(new Filter(num, den));
    }
    // FirstOrderLowPassFilter
    // -----------------------
    else if (filter_type == "FirstOrderLowPassFilter") {
        if (firstOrderLowPassFilter_fc == 0.0 || firstOrderLowPassFilter_ts == 0.0) {
            wbtError << "(FirstOrderLowPassFilter) You need to "
                        "specify Fc and Ts.";
            return false;
        }
        m_filter = std::unique_ptr<IFilter>(
            new FirstOrderLowPassFilter(firstOrderLowPassFilter_fc, firstOrderLowPassFilter_ts));
    }
    // MedianFilter
    // ------------
    else if (filter_type == "MedianFilter") {
        if (static_cast<int>(medianFilter_order) == 0) {
            wbtError << "(MedianFilter) You need to specify the filter order.";
            return false;
        }
        m_filter = std::unique_ptr<IFilter>(new MedianFilter(static_cast<int>(medianFilter_order)));
    }
    else {
        wbtError << "Filter type not recognized.";
        return false;
    }

    // Initialize the other data
    // =========================

    // Allocate the input signal
    m_inputSignalVector = std::unique_ptr<Vector>(new Vector(inputSignalWidth, 0.0));

    return true;
}

bool DiscreteFilter::terminate(const BlockInformation* /*blockInfo*/)
{
    return true;
}

bool DiscreteFilter::initializeInitialConditions(const BlockInformation* /*blockInfo*/)
{
    // Reminder: this function is called when, during runtime, a block is disabled
    // and enabled again. The method ::initialize instead is called just one time.

    // If the initial conditions for the output are not set, allocate a properly
    // sized vector
    // bool initStatus {false};
    // parameters.getParameter("InitStatus", initStatus);

    // Check that initial conditions are initialized
    if (!(m_y0 && m_u0)) {
        wbtError << "Initial conditions not initialized.";
        return false;
    }

    // Initialize the filter. This is required because if the signal is not 1D,
    // the default filter constructor initialize a wrongly sized y0.
    // Moreover, the Filter class has a different constructor that handles the
    // zero-gain case.
    IFilter* baseFilter = m_filter.get();
    Filter* filter_c = dynamic_cast<Filter*>(baseFilter);
    if (filter_c) {
        // The filter is a Filter object
        filter_c->init(*m_y0, *m_u0);
    }
    else {
        // The filter is not a Filter object
        if (m_filter) {
            m_filter->init(*m_y0);
        }
        else {
            wbtError << "Failed to get the IFilter object.";
            return false;
        }
    }

    return true;
}

bool DiscreteFilter::output(const BlockInformation* blockInfo)
{
    if (!m_filter) {
        return false;
    }

    // Get the input and output signals
    const Signal inputSignal = blockInfo->getInputPortSignal(INPUT_IDX_SIGNAL);
    Signal outputSignal = blockInfo->getOutputPortSignal(OUTPUT_IDX_SIGNAL);

    if (inputSignal.isValid() || !outputSignal.isValid()) {
        wbtError << "Signals not valid.";
        return false;
    }

    // Copy the Signal to the data structure that the filter wants
    for (unsigned i = 0; i < inputSignal.getWidth(); ++i) {
        (*m_inputSignalVector)[i] = inputSignal.get<double>(i);
    }

    // Filter the current component of the input signal
    const Vector& outputVector = m_filter->filt(*m_inputSignalVector);

    // Forward the filtered signals to the output port
    outputSignal.setBuffer(outputVector.data(), outputVector.length());

    return true;
}
