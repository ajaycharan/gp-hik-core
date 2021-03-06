/** 
* @file FMKGPHyperparameterOptimization.h
* @brief Heart of the framework to set up everything, perform optimization, classification, and variance prediction (Interface)
* @author Alexander Freytag, Erik Rodner
* @date 01-02-2012 (dd-mm-yyyy)
*/
#ifndef _NICE_FMKGPHYPERPARAMETEROPTIMIZATIONINCLUDE
#define _NICE_FMKGPHYPERPARAMETEROPTIMIZATIONINCLUDE

// STL includes
#include <vector>
#include <set>
#include <map>

// NICE-core includes
#include <core/algebra/EigValues.h>
#include <core/algebra/IterativeLinearSolver.h>
#include <core/basics/Config.h>
#include <core/basics/Persistent.h>
#include <core/vector/VectorT.h>

#ifdef NICE_USELIB_MATIO
#include <core/matlabAccess/MatFileIO.h>
#endif

// gp-hik-core includes
#include "gp-hik-core/FastMinKernel.h"
#include "gp-hik-core/GPLikelihoodApprox.h"
#include "gp-hik-core/IKMLinearCombination.h"
#include "gp-hik-core/OnlineLearnable.h"

#include "gp-hik-core/quantization/Quantization.h"
#include "gp-hik-core/parameterizedFunctions/ParameterizedFunction.h"

namespace NICE {
  
  /** 
 * @class FMKGPHyperparameterOptimization
 * @brief Heart of the framework to set up everything, perform optimization, classification, and variance prediction
 * @author Alexander Freytag, Erik Rodner
 */
  
class FMKGPHyperparameterOptimization : public NICE::Persistent, public NICE::OnlineLearnable
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
    
    /** verbose flag */
    bool b_verbose;    
    /** verbose flag for time measurement outputs */
    bool b_verboseTime;        
    /** debug flag for several outputs useful for debugging*/
    bool b_debug;    
    
    //////////////////////////////////////
    // classification related variables //
    //////////////////////////////////////
    
    /** per default, we perform classification, if not stated otherwise */
    bool b_performRegression;
    
    /** object storing sorted data and providing fast hik methods */
    NICE::FastMinKernel *fmk;

    /** object performing feature quantization */
    NICE::Quantization *q;
    
    
    /** upper bound for hyper parameters (ParameterizedFunction) to optimize */
    double d_parameterUpperBound;
    
    /** lower bound for hyper parameters (ParameterizedFunction) to optimize */
    double d_parameterLowerBound;
    
    /** the parameterized function we use within the minimum kernel */
    NICE::ParameterizedFunction *pf;

    
   
    
    /** Simple type definition for precomputation matrices used for fast classification */
    typedef VVector PrecomputedType;

    /** precomputed arrays A (1 per class) needed for classification without quantization  */
    std::map< uint, PrecomputedType > precomputedA;    
    /** precomputed arrays B (1 per class) needed for classification without quantization  */
    std::map< uint, PrecomputedType > precomputedB;
    
    /** precomputed LUTs (1 per class) needed for classification with quantization  */
    std::map< uint, double * > precomputedT;  
    
    //! storing the labels is needed for Incremental Learning (re-optimization)
    NICE::Vector labels; 
    
    //! store the class number of the positive class (i.e., larger class no), only used in binary settings
    uint i_binaryLabelPositive;
    //! store the class number of the negative class (i.e., smaller class no), only used in binary settings
    uint i_binaryLabelNegative;
    
    //! contains all class numbers of the currently known classes
    std::set<uint> knownClasses;
    
    //! container for multiple kernel matrices (e.g., a data-containing kernel matrix (GMHIKernel) and a noise matrix (IKMNoise) )
    NICE::IKMLinearCombination * ikmsum;    
    
    //////////////////////////////////////////////
    //           Iterative Linear Solver        //
    //////////////////////////////////////////////
    
    /** method for solving linear equation systems - needed to compute K^-1 \times y */
    IterativeLinearSolver *linsolver;    
    
    /** Max. number of iterations the iterative linear solver is allowed to run */
    int ils_max_iterations;    
  
    /////////////////////////////////////
    // optimization related parameters //
    /////////////////////////////////////
    
    enum OPTIMIZATIONTECHNIQUE{
      OPT_GREEDY = 0,
      OPT_DOWNHILLSIMPLEX,
      OPT_NONE
    };

    /** specify the optimization method used (see corresponding enum) */
    OPTIMIZATIONTECHNIQUE optimizationMethod;
    
    //! whether or not to optimize noise with the GP likelihood
    bool optimizeNoise;     
    
        // specific to greedy optimization
    /** step size used in grid based greedy optimization technique */
    double parameterStepSize;
    
     
    
        // specific to downhill simplex optimization
    /** Max. number of iterations the downhill simplex optimizer is allowed to run */
    int downhillSimplexMaxIterations;
    
    /** Max. time the downhill simplex optimizer is allowed to run */
    double downhillSimplexTimeLimit;
    
    /** Max. number of iterations the iterative linear solver is allowed to run */
    double downhillSimplexParamTol;
    
    
    //////////////////////////////////////////////
    // likelihood computation related variables //
    //////////////////////////////////////////////  

    /** whether to compute the exact likelihood by computing the exact kernel matrix (not recommended - only for debugging/comparison purpose) */
    bool verifyApproximation;

    /** method computing eigenvalues and eigenvectors*/
    NICE::EigValues *eig;
    
    /** number of Eigenvalues to consider in the approximation of |K|_F used for approximating the likelihood */
    int nrOfEigenvaluesToConsider;
    
    //! k largest eigenvalues of the kernel matrix (k == nrOfEigenvaluesToConsider)
    NICE::Vector eigenMax;

    //! eigenvectors corresponding to k largest eigenvalues (k == nrOfEigenvaluesToConsider) -- format: nxk
    NICE::Matrix eigenMaxVectors;
    

    ////////////////////////////////////////////
    // variance computation related variables //
    ////////////////////////////////////////////
    
    /** number of Eigenvalues to consider in the fine approximation of the predictive variance (fine approximation only) */
    int nrOfEigenvaluesToConsiderForVarApprox;
    
    /** precomputed array needed for rough variance approximation without quantization */ 
    PrecomputedType precomputedAForVarEst;
    
    /** precomputed LUT needed for rough variance approximation with quantization  */
    double * precomputedTForVarEst;    
    
    /////////////////////////////////////////////////////
    // online / incremental learning related variables //
    /////////////////////////////////////////////////////

    /** whether or not to use previous alpha solutions as initialization after adding new examples*/
    bool b_usePreviousAlphas;
    
    //! store alpha vectors for good initializations in the IL setting, if activated
    std::map<uint, NICE::Vector> previousAlphas;     

    
    /////////////////////////
    /////////////////////////
    //  PROTECTED METHODS  //
    /////////////////////////
    /////////////////////////
    

    /**
    * @brief calculate binary label vectors using a multi-class label vector
    * @author Alexander Freytag
    */    
    uint prepareBinaryLabels ( std::map<uint, NICE::Vector> & _binaryLabels, 
                              const NICE::Vector & _y , 
                              std::set<uint> & _myClasses
                            );     
    
    /**
    * @brief prepare the GPLike object for given binary labels and already given ikmsum-object
    * @author Alexander Freytag
    */
    inline void setupGPLikelihoodApprox( GPLikelihoodApprox * & _gplike, 
                                         const std::map<uint, NICE::Vector> & _binaryLabels,
                                         uint & _parameterVectorSize
                                       );    
    
    /**
    * @brief update eigenvectors and eigenvalues for given ikmsum-objects and a method to compute eigenvalues
    * @author Alexander Freytag
    */
    inline void updateEigenDecomposition( const int & _noEigenValues );
    
    /**
    * @brief core of the optimize-functions
    * @author Alexander Freytag
    */
    inline void performOptimization( GPLikelihoodApprox & gplike, 
                                     const uint & parameterVectorSize
                                   );
    
    /**
    * @brief apply the optimized transformation values to the underlying features
    * @author Alexander Freytag
    */    
    inline void transformFeaturesWithOptimalParameters(const GPLikelihoodApprox & _gplike, 
                                                       const uint & _parameterVectorSize
                                                      );
    
    /**
    * @brief build the resulting matrices A and B as well as lookup tables T for fast evaluations using the optimized parameter settings
    * @author Alexander Freytag
    */
    inline void computeMatricesAndLUTs( const GPLikelihoodApprox & _gplike);
    
     

    /**
    * @brief Update matrices (A, B, LUTs) and optionally find optimal parameters after adding (a) new example(s).  
    * @author Alexander Freytag
    */           
    void updateAfterIncrement (
      const std::set<uint> _newClasses,
      const bool & _performOptimizationAfterIncrement = false
    );    
  

    
  public:  

    /**
    * @brief default constructor
    * @author Alexander Freytag
    */
    FMKGPHyperparameterOptimization( );
    
    /**
    * @brief simple constructor
    * @author Alexander Freytag
    * @param b_performRegression
    */
    FMKGPHyperparameterOptimization( const bool & _performRegression );

    /**
    * @brief recommended constructor, only calls this->initialize with same input arguments
    * @author Alexander Freytag
    * @param conf
    * @param confSection
    *
    */
    FMKGPHyperparameterOptimization( const Config *_conf, 
                                     const std::string & _confSection = "FMKGPHyperparameterOptimization" 
                                   );
    
    
    /**
    * @brief recommended constructor, only calls this->initialize with same input arguments
    * @author Alexander Freytag
    *
    * @param conf
    * @param fmk pointer to a pre-initialized structure (will be deleted)
    * @param confSection
    */
    FMKGPHyperparameterOptimization( const Config *_conf, 
                                     FastMinKernel *_fmk, 
                                     const std::string & _confSection = "FMKGPHyperparameterOptimization" 
                                   );
      
    /**
    * @brief standard destructor
    * @author Alexander Freytag
    */
    virtual ~FMKGPHyperparameterOptimization();
    
    /**
    * @brief Set variables and parameters to default or config-specified values
    * @author Alexander Freytag
    */       
    void initFromConfig( const Config *_conf, 
                         const std::string & _confSection = "FMKGPHyperparameterOptimization" 
                       );
    
    
    ///////////////////// ///////////////////// /////////////////////
    //                         GET / SET
    ///////////////////// ///////////////////// /////////////////////
    
    /**
    * @brief Set lower bound for hyper parameters to optimize
    * @author Alexander Freytag
    */    
    void setParameterUpperBound(const double & _parameterUpperBound);
    /**
    * @brief Set upper bound for hyper parameters to optimize
    * @author Alexander Freytag
    */    
    void setParameterLowerBound(const double & _parameterLowerBound);  
    
    /**
    * @brief Get the currently known class numbers
    * @author Alexander Freytag
    */    
    std::set<uint> getKnownClassNumbers ( ) const;
    
    /**
     * @brief Change between classification and regression, only allowed if not trained. Otherwise, exceptions will be thrown...
     * @author Alexander Freytag
     * @date 05-02-2014 (dd-mm-yyyy)
     */
    void setPerformRegression ( const bool & _performRegression );
    
    /**
     * @brief Set the FastMinKernel object. Only allowed if not trained. Otherwise, exceptions will be thrown...
     * @author Alexander Freytag
     * @date 05-02-2014 (dd-mm-yyyy)
     */    
    void setFastMinKernel ( FastMinKernel *fmk );  

    /**
     * @brief Set the number of EV we considere for variance approximation. Only allowed if not trained. Otherwise, exceptions will be thrown...
     * @author Alexander Freytag
     * @date 06-02-2014 (dd-mm-yyyy)
     */        
    void setNrOfEigenvaluesToConsiderForVarApprox ( const int & _nrOfEigenvaluesToConsiderForVarApprox );
    
    ///////////////////// ///////////////////// /////////////////////
    //                      CLASSIFIER STUFF
    ///////////////////// ///////////////////// /////////////////////  
    
       
#ifdef NICE_USELIB_MATIO
    /**
    * @brief Perform hyperparameter optimization
    * @author Alexander Freytag
    * 
    * @param data MATLAB data structure, like a feature matrix loaded from ImageNet
    * @param y label vector (arbitrary), will be converted into a binary label vector
    * @param positives set of positive examples (indices)
    * @param negatives set of negative examples (indices)
    */
    void optimizeBinary ( const sparse_t & data, 
                          const NICE::Vector & y, 
                          const std::set<uint> & positives, 
                          const std::set<uint> & negatives, 
                          double noise 
                        );

    /**
    * @brief Perform hyperparameter optimization for GP multi-class or binary problems
    * @author Alexander Freytag
    * 
    * @param data MATLAB data structure, like a feature matrix loaded from ImageNet
    * @param y label vector with multi-class labels
    * @param examples mapping of example index to new index
    */
    void optimize ( const sparse_t & data, 
                    const NICE::Vector & y, 
                    const std::map<uint, uint> & examples, 
                    double noise 
                  );
#endif

    /**
    * @brief Perform hyperparameter optimization (multi-class or binary) assuming an already initialized fmk object
    * @author Alexander Freytag
    *
    * @param y label vector (multi-class as well as binary labels supported)
    */
    void optimize ( const NICE::Vector & y );
    
    /**
    * @brief Perform hyperparameter optimization (multi-class or binary) assuming an already initialized fmk object
    *
    * @param binLabels vector of binary label vectors (1,-1) and corresponding class no.
    */
    void optimize ( std::map<uint, NICE::Vector> & _binaryLabels );  
   
    /**
    * @brief Compute the necessary variables for appxorimations of predictive variance (LUTs), assuming an already initialized fmk object
    * @author Alexander Freytag
    * @date 11-04-2012 (dd-mm-yyyy)
    */       
    void prepareVarianceApproximationRough();
    
    /**
    * @brief Compute the necessary variables for fine appxorimations of predictive variance (EVs), assuming an already initialized fmk object
    * @author Alexander Freytag
    * @date 11-04-2012 (dd-mm-yyyy)
    */       
    void prepareVarianceApproximationFine();    
    
    /**
    * @brief classify an example 
    *
    * @param x input example (sparse vector)
    * @param scores scores for each class number
    *
    * @return class number achieving the best score
    */
    uint classify ( const NICE::SparseVector & _x, 
                   SparseVector & _scores 
                 ) const;

    /**
    * @brief classify an example
    *
    * @param x input example (sparse vector)
    * @param scores scores for each class number (non-sparse)
    *
    * @return class number achieving the best score
    */
    uint classify ( const NICE::SparseVector & _x,
                   NICE::Vector & _scores
                 ) const;
    
    /**
    * @brief classify an example that is given as non-sparse vector
    * NOTE: whenever possible, you should use sparse vectors to obtain significantly smaller computation times
    * 
    * @date 18-06-2013 (dd-mm-yyyy)
    * @author Alexander Freytag
    *
    * @param x input example (non-sparse vector)
    * @param scores scores for each class number
    *
    * @return class number achieving the best score
    */
    uint classify ( const NICE::Vector & _x, 
                    SparseVector & _scores 
                  ) const;    

    //////////////////////////////////////////
    // variance computation: sparse inputs
    //////////////////////////////////////////
    
    /**
    * @brief compute predictive variance for a given test example using a rough approximation: k_{**} -  k_*^T (K+\sigma I)^{-1} k_* <= k_{**} - |k_*|^2 * 1 / \lambda_max(K + \sigma I), where we approximate |k_*|^2 by neglecting the mixed terms
    * @author Alexander Freytag
    * @date 10-04-2012 (dd-mm-yyyy)
    * @param x input example
    * @param predVariance contains the approximation of the predictive variance
    *
    */    
    void computePredictiveVarianceApproximateRough(const NICE::SparseVector & _x, 
                                                   double & _predVariance 
                                                  ) const;
    
    /**
    * @brief compute predictive variance for a given test example using a fine approximation  (k eigenvalues and eigenvectors to approximate the quadratic term)
    * @author Alexander Freytag
    * @date 18-04-2012 (dd-mm-yyyy)
    * @param x input example
     * @param predVariance contains the approximation of the predictive variance
    *
    */    
    void computePredictiveVarianceApproximateFine(const NICE::SparseVector & _x, 
                                                  double & _predVariance 
                                                 ) const; 
    
    /**
    * @brief compute exact predictive variance for a given test example using ILS methods (exact, but more time consuming than approx versions)
    * @author Alexander Freytag
    * @date 10-04-2012 (dd-mm-yyyy)
    * @param x input example
     * @param predVariance contains the approximation of the predictive variance
    *
    */    
    void computePredictiveVarianceExact(const NICE::SparseVector & _x, 
                                        double & _predVariance 
                                       ) const; 
    
    
    //////////////////////////////////////////
    // variance computation: non-sparse inputs
    //////////////////////////////////////////
    
    /**
    * @brief compute predictive variance for a given test example using a rough approximation: k_{**} -  k_*^T (K+\sigma I)^{-1} k_* <= k_{**} - |k_*|^2 * 1 / \lambda_max(K + \sigma I), where we approximate |k_*|^2 by neglecting the mixed terms
    * @author Alexander Freytag
    * @date 19-12-2013 (dd-mm-yyyy)
    * @param x input example
    * @param predVariance contains the approximation of the predictive variance
    *
    */    
    void computePredictiveVarianceApproximateRough(const NICE::Vector & _x, 
                                                   double & _predVariance 
                                                  ) const;    

   
    
    /**
    * @brief compute predictive variance for a given test example using a fine approximation  (k eigenvalues and eigenvectors to approximate the quadratic term)
    * @author Alexander Freytag
    * @date 19-12-2013 (dd-mm-yyyy)
    * @param x input example
    * @param predVariance contains the approximation of the predictive variance
    *
    */    
    void computePredictiveVarianceApproximateFine(const NICE::Vector & _x, 
                                                  double & _predVariance 
                                                 ) const;      
    

    
   /**
    * @brief compute exact predictive variance for a given test example using ILS methods (exact, but more time consuming than approx versions)
    * @author Alexander Freytag
    * @date 19-12-2013 (dd-mm-yyyy)
    * @param x input example
    * @param predVariance contains the approximation of the predictive variance
    *
    */    
    void computePredictiveVarianceExact(const NICE::Vector & _x, 
                                        double & _predVariance 
                                       ) const;  
    
    
    
    
    
    ///////////////////// INTERFACE PERSISTENT /////////////////////
    // interface specific methods for store and restore
    ///////////////////// INTERFACE PERSISTENT ///////////////////// 
    
    /** 
     * @brief Load current object from external file (stream)
     * @author Alexander Freytag
     */     
    void restore ( std::istream & _is, 
                   int _format = 0 
                 );
    
    /** 
     * @brief Save current object to external file (stream)
     * @author Alexander Freytag
     */      
    void store ( std::ostream & _os,
                 int _format = 0 
               ) const;
    
    /** 
     * @brief Clear current object
     * @author Alexander Freytag
     */      
    void clear ( ) ;
    
    ///////////////////// INTERFACE ONLINE LEARNABLE /////////////////////
    // interface specific methods for incremental extensions
    ///////////////////// INTERFACE ONLINE LEARNABLE /////////////////////    
    
    /** 
     * @brief add a new example
     * @author Alexander Freytag
     */       
    virtual void addExample( const NICE::SparseVector * _example, 
                             const double & _label, 
                             const bool & _performOptimizationAfterIncrement = true
                           );

    /** 
     * @brief add several new examples
     * @author Alexander Freytag
     */    
    virtual void addMultipleExamples( const std::vector< const NICE::SparseVector * > & _newExamples,
                                      const NICE::Vector & _newLabels,
                                      const bool & _performOptimizationAfterIncrement = true
                                    );         
};

}

#endif
