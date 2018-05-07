#ifndef WBT_PARAMETERS_H
#define WBT_PARAMETERS_H

#include "Parameter.h"

#include <unordered_map>
#include <vector>

namespace wbt {
    class Parameters;
    const int PARAM_INVALID_INDEX = -1;
    const std::string PARAM_INVALID_NAME = {};
} // namespace wbt

class wbt::Parameters
{
public:
    typedef int ParamIndex;
    typedef std::string ParamName;

private:
    // Typedefs for generic parameters
    typedef Parameter<int> ParameterInt;
    typedef Parameter<bool> ParameterBool;
    typedef Parameter<double> ParameterDouble;
    typedef Parameter<std::string> ParameterString;

    // Typedefs for the storage of vector parameters
    typedef std::vector<int> ParamVectorInt;
    typedef std::vector<bool> ParamVectorBool;
    typedef std::vector<double> ParamVectorDouble;
    typedef std::vector<std::string> ParamVectorString;

    // Maps for storing parameters and their metadata
    std::unordered_map<ParamName, ParameterInt> m_paramsInt;
    std::unordered_map<ParamName, ParameterBool> m_paramsBool;
    std::unordered_map<ParamName, ParameterDouble> m_paramsDouble;
    std::unordered_map<ParamName, ParameterString> m_paramsString;

    // Maps for handling the internal indexing
    std::unordered_map<ParamName, wbt::ParameterType> m_nameToType;
    std::unordered_map<ParamIndex, ParamName> m_indexToName;
    std::unordered_map<ParamName, ParamIndex> m_nameToIndex;

    bool existIndex(const ParamIndex& index) const;
    bool existName(const ParamName& name, const wbt::ParameterType& type) const;

public:
    Parameters() = default;
    ~Parameters() = default;

    ParamName getParamName(const ParamIndex& index) const;
    ParamIndex getParamIndex(const ParamName& name) const;
    unsigned getNumberOfParameters() const;

    bool existName(const ParamName& name) const;

    // Scalar parameters
    template <typename T>
    bool storeParameter(const T& param, const wbt::ParameterMetadata& paramMetadata);

    // Vector parameters
    template <typename T>
    bool storeParameter(const std::vector<T>& param, const wbt::ParameterMetadata& paramMetadata);

    // Generic
    template <typename T>
    bool storeParameter(const Parameter<T>& parameter);

    // Scalar / Struct Fields
    template <typename T>
    bool getParameter(const ParamName& name, T& param) const;

    template <typename T>
    bool getParameter(const ParamName& name, std::vector<T>& param) const;

    std::vector<Parameter<int>> getIntParameters() const;
    std::vector<Parameter<bool>> getBoolParameters() const;
    std::vector<Parameter<double>> getDoubleParameters() const;
    std::vector<Parameter<std::string>> getStringParameters() const;

    wbt::ParameterMetadata getParameterMetadata(const ParamName& name);

    static bool containConfigurationData(const wbt::Parameters& parameters);
};

// GETPARAMETER
// ============

// SCALAR
namespace wbt {
    // Explicit declaration for numeric types
    extern template bool Parameters::getParameter<int>(const Parameters::ParamName& name,
                                                       int& param) const;
    extern template bool Parameters::getParameter<bool>(const Parameters::ParamName& name,
                                                        bool& param) const;
    extern template bool Parameters::getParameter<double>(const Parameters::ParamName& name,
                                                          double& param) const;
    // Explicit specialization for std::string
    template <>
    bool Parameters::getParameter<std::string>(const Parameters::ParamName& name,
                                               std::string& param) const;
} // namespace wbt

// VECTOR
namespace wbt {
    // Explicit declaration for numeric types
    extern template bool Parameters::getParameter<int>(const Parameters::ParamName& name,
                                                       std::vector<int>& param) const;
    extern template bool Parameters::getParameter<bool>(const Parameters::ParamName& name,
                                                        std::vector<bool>& param) const;
    extern template bool Parameters::getParameter<double>(const Parameters::ParamName& name,
                                                          std::vector<double>& param) const;
    extern template bool
    Parameters::getParameter<std::string>(const Parameters::ParamName& name,
                                          std::vector<std::string>& param) const;
} // namespace wbt

// STOREPARAMETER
// ==============

// SCALAR
namespace wbt {
    // Explicit declaration for numeric types
    extern template bool Parameters::storeParameter<int>(const int& param,
                                                         const ParameterMetadata& paramMetadata);
    extern template bool Parameters::storeParameter<bool>(const bool& param,
                                                          const ParameterMetadata& paramMetadata);
    extern template bool Parameters::storeParameter<double>(const double& param,
                                                            const ParameterMetadata& paramMetadata);
    // Explicit specialization for std::string
    template <>
    bool Parameters::storeParameter<std::string>(const std::string& param,
                                                 const ParameterMetadata& paramMetadata);
} // namespace wbt

// VECTOR
namespace wbt {
    // Explicit declaration for numeric types
    extern template bool Parameters::storeParameter<int>(const std::vector<int>& param,
                                                         const ParameterMetadata& paramMetadata);
    extern template bool Parameters::storeParameter<bool>(const std::vector<bool>& param,
                                                          const ParameterMetadata& paramMetadata);
    extern template bool Parameters::storeParameter<double>(const std::vector<double>& param,
                                                            const ParameterMetadata& paramMetadata);
    extern template bool
    Parameters::storeParameter<std::string>(const std::vector<std::string>& param,
                                            const ParameterMetadata& paramMetadata);
} // namespace wbt

// PARAMETER
namespace wbt {
    // Explicit declaration for numeric types
    extern template bool Parameters::storeParameter<int>(const Parameter<int>& parameter);
    extern template bool Parameters::storeParameter<bool>(const Parameter<bool>& parameter);
    extern template bool Parameters::storeParameter<double>(const Parameter<double>& parameter);
    extern template bool
    Parameters::storeParameter<std::string>(const Parameter<std::string>& parameter);
} // namespace wbt

#endif // WBT_PARAMETERS_H
