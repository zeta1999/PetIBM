/***************************************************************************//**
 * \file NavierStokesSolver.h
 * \author Anush Krishnan (anush@bu.edu)
 * \brief Definition of the class \c NavierStokesSolver.
 */


#if !defined(NAVIER_STOKES_SOLVER_H)
#define NAVIER_STOKES_SOLVER_H

#include "CartesianMesh.h"
#include "FlowDescription.h"
#include "SimulationParameters.h"

#include <fstream>

#include <petscdmda.h>
#include <petscksp.h>


/**
 * \brief Solve the incompressible Navier-Stokes equations in a rectangular or
 *        cuboidal domain.
 */
template <PetscInt dim>
class NavierStokesSolver
{
public:
  std::string caseFolder;

  FlowDescription      *flowDesc;
  SimulationParameters *simParams;
  CartesianMesh        *mesh;
  
  PetscInt timeStep;

  std::vector<PetscReal> dxU, dyU, dzU,
                         dxV, dyV, dzV,
                         dxW, dyW, dzW;

  std::ofstream iterationCountsFile;
  
  DM  pda,
      uda,
      vda,
      wda,
      qPack,
      lambdaPack;
  
  Vec qxLocal,
      qyLocal,
      qzLocal;

  Vec uMapping,
      vMapping,
      wMapping,
      pMapping;

  Vec H, rn;
  Vec RInv, MHat;

  Mat A;
  Mat QT, BNQ;
  Mat QTBNQ;
  Vec BN;
  Vec bc1, rhs1, r2, rhs2, temp;
  Vec q, qStar, lambda;
  KSP ksp1, ksp2;
  PC  pc2;

  PetscLogStage stageInitialize,
                stageRHSVelocitySystem,
                stageSolveVelocitySystem,
                stageRHSPoissonSystem,
                stageSolvePoissonSystem,
                stageProjectionStep;

  // initialize data common to NavierStokesSolver and derived classes
  PetscErrorCode initializeCommon();

  // create DMDA structures for flow variables
  virtual PetscErrorCode createDMs();

  // create vectors used to store flow variables
  virtual PetscErrorCode createVecs();

  // set up Krylov solvers used to solve linear systems
  PetscErrorCode createKSPs();

  // initialize spaces between adjacent velocity nodes
  void initializeMeshSpacings();

  // populate flux vectors with initial conditions
  virtual PetscErrorCode initializeFluxes();
  // initialize lambda vector with previously saved data
  virtual PetscErrorCode initializeLambda();

  // create mapping from local flux vectors to global flux vectors
  PetscErrorCode createLocalToGlobalMappingsFluxes();

  // create mapping from local pressure variable to global lambda vector
  PetscErrorCode createLocalToGlobalMappingsLambda();

  // update values in ghost nodes at the domain boundaries
  PetscErrorCode updateBoundaryGhosts();

  // generate diagonal matrices M and Rinv
  PetscErrorCode generateDiagonalMatrices();

  // count number of non-zeros in the diagonal and off-diagonal portions of the parallel matrices
  void countNumNonZeros(PetscInt *cols, size_t numCols, PetscInt rowStart, PetscInt rowEnd, 
                        PetscInt &d_nnz, PetscInt &o_nnz);

  // generate the matrix A
  PetscErrorCode generateA();
  // calculate explicit convective and diffusive terms
  PetscErrorCode calculateExplicitTerms();
  // assemble velocity boundary conditions vector
  PetscErrorCode generateBC1();
  // assemble RHS of intermediate velocity system
  PetscErrorCode generateRHS1();
  // assemble boundary conditions vector for the pressure-force system
  virtual PetscErrorCode generateR2();
  // assemble RHS of pressure-force system
  PetscErrorCode generateRHS2();
  // compute matrix \f$ B^N Q \f$
  virtual PetscErrorCode generateBNQ();
  // compute matrix \f$ Q^T B^N Q \f$
  PetscErrorCode generateQTBNQ();

  // calculate and specify to the Krylov solver the null-space of the LHS matrix
  // in the pressure-force system
  virtual PetscErrorCode setNullSpace();

  // solve system for intermediate velocity fluxes \f$ q^* \f$
  PetscErrorCode solveIntermediateVelocity();
  // solver Poisson system for pressure and body forces
  PetscErrorCode solvePoissonSystem();
  // project velocity onto divergence-free field with satisfaction of the no-splip condition
  PetscErrorCode projectionStep();

  // write simulation parameters into file
  PetscErrorCode printSimulationInfo();
  // write grid coordinates into file
  PetscErrorCode writeGrid();
  // write fluxes into files
  PetscErrorCode writeFluxes();
  // write pressure field into file
  virtual PetscErrorCode writeLambda();
  // write KSP iteration counts into file
  PetscErrorCode writeIterationCounts();
  // read fluxes from files
  PetscErrorCode readFluxes();
  // read pressure field from file
  virtual PetscErrorCode readLambda();
  
public:
  // constructor
  NavierStokesSolver(std::string directory, 
                     CartesianMesh *cartesianMesh, 
                     FlowDescription *flowDescription, 
                     SimulationParameters *simulationParameters);
  // destructor
  ~NavierStokesSolver();
  // initial set-up of the system
  virtual PetscErrorCode initialize();
  // clean up at the end of the simulation
  virtual PetscErrorCode finalize();

  // advance in time
  PetscErrorCode stepTime();

  // write numerical solution into respective files
  virtual PetscErrorCode writeData();

  // specify if data needs to be saved at current time-step
  PetscBool savePoint();

  // evaluate if the simulation is completed
  PetscBool finished();
  
  // name of the solver
  virtual std::string name()
  {
    return "Navier-Stokes";
  }

}; // NavierStokesSolver

#endif