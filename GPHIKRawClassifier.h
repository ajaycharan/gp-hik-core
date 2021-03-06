/**
* @file GPHIKRawClassifier.h
* @brief ..
* @author Erik Rodner
* @date 16-09-2015 (dd-mm-yyyy)
*/
#ifndef _NICE_GPHIKRAWCLASSIFIERINCLUDE
#define _NICE_GPHIKRAWCLASSIFIERINCLUDE

// STL includes
#include <string>
#include <limits>
#include <set>

// NICE-core includes
#include <core/basics/Config.h>
#include <core/basics/Persistent.h>
#include <core/vector/SparseVectorT.h>
#include <core/algebra/ILSConjugateGradients.h>

//
#include "quantization/Quantization.h"
#include "GMHIKernelRaw.h"

namespace NICE {

 /**
 * @class GPHIKRawClassifier
 * @brief ...
 * @author Erik Rodner, Alexander Freytag
 */

class GPHIKRawClassifier
{

  protected:

    /////////////////////////
    /////////////////////////
    // PROTECTED VARIABLES //
    /////////////////////////
    /////////////////////////

    ///////////////////////////////////
    // output/debug related settings //
    ///////////////////////////////////

    /** verbose flag for useful output*/
    bool b_verbose;
    /** debug flag for several outputs useful for debugging*/
    bool b_debug;

    //////////////////////////////////////
    //      general specifications      //
    //////////////////////////////////////

    /** Header in configfile where variable settings are stored */
    std::string confSection;

    //////////////////////////////////////
    //     EigenValue Decomposition     //
    //////////////////////////////////////

    bool b_eig_verbose;
    int i_eig_value_max_iterations;

    //////////////////////////////////////
    // classification related variables //
    //////////////////////////////////////
    /** memorize whether the classifier was already trained*/
    bool b_isTrained;


    /** Gaussian label noise for model regularization */
    double d_noise;

    ILSConjugateGradients *solver;
    /** object performing feature quantization */
    NICE::Quantization *q;

    typedef double ** PrecomputedType;

    /** precomputed arrays A (1 per class) needed for classification without quantization  */
    std::map< uint, PrecomputedType > precomputedA;
    /** precomputed arrays B (1 per class) needed for classification without quantization  */
    std::map< uint, PrecomputedType > precomputedB;

    /** precomputed LUTs (1 per class) needed for classification with quantization  */
    std::map< uint, double * > precomputedT;

    uint *nnz_per_dimension;
    uint num_examples;
    uint num_dimension;

    double f_tolerance;

    GMHIKernelRaw *gm;
    std::set<uint> knownClasses;

    /////////////////////////
    /////////////////////////
    //  PROTECTED METHODS  //
    /////////////////////////
    /////////////////////////

    void clearSetsOfTablesAandB();
    void clearSetsOfTablesT();


    /////////////////////////
    /////////////////////////
    //    PUBLIC METHODS   //
    /////////////////////////
    /////////////////////////

  public:

    /**
     * @brief default constructor
     */
    GPHIKRawClassifier( );


    /**
     * @brief standard constructor
     */
    GPHIKRawClassifier( const NICE::Config *_conf ,
                     const std::string & s_confSection = "GPHIKRawClassifier"
                   );

    /**
     * @brief simple destructor
     */
    ~GPHIKRawClassifier();

    /**
    * @brief Setup internal variables and objects used
    * @param conf Config file to specify variable settings
    * @param s_confSection
    */
    void initFromConfig(const NICE::Config *_conf,
                        const std::string & s_confSection
                       );

    ///////////////////// ///////////////////// /////////////////////
    //                         GET / SET
    ///////////////////// ///////////////////// /////////////////////

    /**
     * @brief Return currently known class numbers
     */
    std::set<uint> getKnownClassNumbers ( ) const;



    ///////////////////// ///////////////////// /////////////////////
    //                      CLASSIFIER STUFF
    ///////////////////// ///////////////////// /////////////////////

    /**
     * @brief classify a given example with the previously learned model
     * @author Alexander Freytag, Erik Rodner
     * @param example (SparseVector) to be classified given in a sparse representation
     * @param result (int) class number of most likely class
     * @param scores (SparseVector) classification scores for known classes
     */
    void classify ( const NICE::SparseVector * _example,
                    uint & _result,
                    NICE::SparseVector & _scores
                  ) const;

    /**
     * @brief classify a given example with the previously learned model
     * @author Alexander Freytag, Erik Rodner
     * @param example (SparseVector) to be classified given in a sparse representation
     * @param result (int) class number of most likely class
     * @param scores (Vector) classification scores for known classes
     */
    void classify ( const NICE::SparseVector * _example,
                    uint & _result,
                    NICE::Vector & _scores
                  ) const;

    /**
     * @brief classify a given set of examples with the previously learned model
     * @author Alexander Freytag, Erik Rodner
     * @param examples ((std::vector< NICE::SparseVector *>)) to be classified given in a sparse representation
     * @param results (Vector) class number of most likely class per example
     * @param scores (NICE::Matrix) classification scores for known classes and test examples
     */
    void classify ( const std::vector< const NICE::SparseVector *> _examples,
                    NICE::Vector & _results,
                    NICE::Matrix & _scores
                  ) const;

    /**
     * @brief train this classifier using a given set of examples and a given set of binary label vectors
     * @date 18-10-2012 (dd-mm-yyyy)
     * @author Alexander Freytag, Erik Rodner
     * @param examples (std::vector< NICE::SparseVector *>) training data given in a sparse representation
     * @param labels (Vector) class labels (multi-class)
     */
    void train ( const std::vector< const NICE::SparseVector *> & _examples,
                 const NICE::Vector & _labels
               );

    /**
     * @brief train this classifier using a given set of examples and a given set of binary label vectors
     * @author Alexander Freytag, Erik Rodner
     * @param examples examples to use given in a sparse data structure
     * @param binLabels corresponding binary labels with class no. There is no need here that every examples has only on positive entry in this set (1,-1)
     */
    void train ( const std::vector< const NICE::SparseVector *> & _examples,
                 std::map<uint, NICE::Vector> & _binLabels
               );

};

}

#endif
