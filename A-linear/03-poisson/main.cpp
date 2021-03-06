#include "definitions.h"

// This example shows how to solve a simple PDE that describes stationary
// heat transfer in an object consisting of two materials (aluminum and
// copper). The object is heated by constant volumetric heat sources
// (generated, for example, by a DC electric current). The temperature
// on the boundary is fixed. We will learn how to:
//
//   - load the mesh,
//   - perform initial refinements,
//   - create a H1 space over the mesh,
//   - define weak formulation,
//   - select a matrix solver,
//   - assemble and solve the matrix system,
//   - output the solution and element orders in VTK format
//     (to be visualized, e.g., using Paraview),
//   - visualize the solution using Hermes' native OpenGL-based functionality.
//
// PDE: Poisson equation -div(LAMBDA grad u) - VOLUME_HEAT_SRC = 0.
//
// Boundary conditions: Dirichlet u(x, y) = FIXED_BDY_TEMP on the boundary.
//
// Geometry: L-Shape domain (see file domain.mesh).
//
// The following parameters can be changed:

// Read the original or XML mesh file.
const bool USE_XML_FORMAT = true;
// Set to "false" to suppress Hermes OpenGL visualization.
const bool HERMES_VISUALIZATION = true;
// Set to "true" to enable VTK output.
const bool VTK_VISUALIZATION = true;
// Uniform polynomial degree of mesh elements.
const int P_INIT = 5;
// Number of initial uniform mesh refinements.
const int INIT_REF_NUM = 0;

// Problem parameters.
// Thermal cond. of Al for temperatures around 20 deg Celsius.
const double LAMBDA_AL = 236.0;
// Thermal cond. of Cu for temperatures around 20 deg Celsius.
const double LAMBDA_CU = 386.0;
// Volume heat sources generated (for example) by electric current.
const double VOLUME_HEAT_SRC = 5e3;
// Fixed temperature on the boundary.
const double FIXED_BDY_TEMP = 20.0;

int main(int argc, char* argv[])
{
  // Load the mesh.
  MeshSharedPtr mesh(new Mesh);
  if (USE_XML_FORMAT == true)
  {
    MeshReaderH2DXML mloader;
    Hermes::Mixins::Loggable::Static::info("Reading mesh in XML format.");
    try
    {
      mloader.load("domain.xml", mesh);
    }
    catch (Hermes::Exceptions::Exception& e)
    {
      e.print_msg();
    }
  }
  else
  {
    MeshReaderH2D mloader;
    Hermes::Mixins::Loggable::Static::info("Reading mesh in original format.");
    mloader.load("domain.mesh", mesh);
  }

  // Perform initial mesh refinements (optional).
  for (int i = 0; i < INIT_REF_NUM; i++)
    mesh->refine_all_elements();

  // Initialize the weak formulation.
  WeakFormSharedPtr<double> wf(new CustomWeakFormPoisson("Aluminum", new Hermes1DFunction<double>(LAMBDA_AL), "Copper",
    new Hermes1DFunction<double>(LAMBDA_CU), new Hermes2DFunction<double>(-VOLUME_HEAT_SRC)));

  // Initialize essential boundary conditions.
  DefaultEssentialBCConst<double> bc_essential({ "Bottom", "Inner", "Outer", "Left" }, FIXED_BDY_TEMP);
  EssentialBCs<double> bcs(&bc_essential);

  // Create an H1 space with default shapeset.
  SpaceSharedPtr<double> space(new H1Space<double>(mesh, &bcs, P_INIT));
  int ndof = space->get_num_dofs();
  Hermes::Mixins::Loggable::Static::info("ndof = %d", ndof);

  // Initialize Newton solver.
  NewtonSolver<double> newton(wf, space);

  // Perform Newton's iteration.
  try
  {
    newton.solve();
  }
  catch (std::exception& e)
  {
    std::cout << e.what();
  }

  // Translate the resulting coefficient vector into a Solution.
  MeshFunctionSharedPtr<double> sln(new Solution<double>);
  Solution<double>::vector_to_solution(newton.get_sln_vector(), space, sln);

  // VTK output.
  if (VTK_VISUALIZATION)
  {
    // Output solution in VTK format.
    Linearizer lin(FileExport);
    bool mode_3D = true;
    lin.save_solution_vtk(sln, "sln.vtk", "Temperature", mode_3D);
    Hermes::Mixins::Loggable::Static::info("Solution in VTK format saved to file %s.", "sln.vtk");

    // Output mesh and element orders in VTK format.
    Orderizer ord;
    ord.save_orders_vtk(space, "ord.vtk");
    Hermes::Mixins::Loggable::Static::info("Element orders in VTK format saved to file %s.", "ord.vtk");
  }

  // Visualize the solution.
  if (HERMES_VISUALIZATION)
  {
    ScalarView view("Solution", new WinGeom(0, 0, 440, 350));
    // See the Doxygen documentation for explanation.
    view.set_linearizer_criterion(LinearizerCriterionAdaptive(HERMES_EPS_VERYHIGH));
    view.show(sln);
    View::wait();
  }

  return 0;
}