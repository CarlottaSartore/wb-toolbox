#ifndef WBIT_BLOCK_H
#define WBIT_BLOCK_H

#include <string>
#include "simstruc.h"

namespace wbit {
    class Block;
    class Error;
}

/**
 * Basic abstract class for all the blocks.
 * This class (the whole toolbox in reality) assumes the block represent
 * an instantaneous system (i.e. not a dynamic system).
 * 
 * You can create a new block by deriving this class and implementing at least
 * all the pure virtual methods.
 *
 * @Note: if you need to implement a block which uses the WBI, you should derive
 * from WBIBlock as it already provides some facilities.
 */
class wbit::Block {
public:
    /**
     * Create and returns a new Block object of the specified class.
     *
     * If the class does not exist returns NULL
     * @param blockClassName the derived class name to be instantiated
     *
     * @return the newly created Block object or NULL.
     */
    static wbit::Block* instantiateBlockWithClassName(std::string blockClassName);

    /**
     * Destructor
     *
     */
    virtual ~Block();
    
    /**
     * Returns the number of configuration parameters needed by this block
     *
     * @return the number of parameters
     */
    virtual unsigned numberOfParameters() = 0;

    /**
     * Specify if the parameter at the specified index is tunable
     *
     * Tunable means that it can be changed during the simulation.
     * @Note by default it specifies false for every parameter
     * @param [in]index   index of the parameter
     * @param [out]tunable true if the parameter is tunable. False otherwise
     */
    virtual void parameterAtIndexIsTunable(unsigned index, bool &tunable);

    /**
     * Configure the input and output ports
     *
     * This method is called at configuration time (i.e. during mdlInitializeSizes)
     * In this method you have to configure the number of input and output ports,
     * their size and configuration.
     * @Note: you should not save any object in this method because it will not persist
     * @param S     simulink structure
     * @param [out]error output error object that can be filled in case of error. Check if the pointer exists before dereference it.
     *
     * @return true for success, false otherwise
     */
    virtual bool configureSizeAndPorts(SimStruct *S, wbit::Error *error) = 0;

    /**
     * Never called.
     *
     * @param S     simulink structure
     * @param [out]error output error object that can be filled in case of error. Check if the pointer exists before dereference it.
     *
     * @return true for success, false otherwise
     */
    virtual bool checkParameters(SimStruct *S, wbit::Error *error);

    /**
     * Initialize the object for the simulation
     *
     * This method is called at model startup (i.e. during mdlStart)
     * @Note: you can save and initialize your object in this method
     * @param S     simulink structure
     * @param [out]error output error object that can be filled in case of error. Check if the pointer exists before dereference it.
     *
     * @return true for success, false otherwise
     */
    virtual bool initialize(SimStruct *S, wbit::Error *error) = 0;

    /**
     * Perform model cleanup.
     *
     * This method is called at model termination (i.e. during mdlTerminate).
     * You should release all the resources you requested during the initialize method
     * @param S     simulink structure
     * @param [out]error output error object that can be filled in case of error. Check if the pointer exists before dereference it.
     *
     * @return true for success, false otherwise
     */
    virtual bool terminate(SimStruct *S, wbit::Error *error) = 0;

    /**
     * Compute the output of the block
     *
     * This method is called at every iteration of the model (i.e. during mdlOutput)
     * @param S     simulink structure
     * @param [out]error output error object that can be filled in case of error. Check if the pointer exists before dereference it.
     *
     * @return true for success, false otherwise
     */
    virtual bool output(SimStruct *S, wbit::Error *error) = 0;

public:
    static bool readStringParameterAtIndex(SimStruct *S, unsigned index, std::string &readParameter);
};

#endif /* end of include guard: WBIT_BLOCK_H */
