/**
  The goal of this tutorial is to implement in Tiramisu a code that is
  equivalent to the following

  for (int i = 0; i < 10; i++)
    for (int j = 0; j < 20; j++)
      output[i, j] = input[i, j] + i + 2;
*/

#include <tiramisu/tiramisu.h>

#define NN 10
#define MM 20

using namespace tiramisu;

int main(int argc, char **argv)
{
    // Set default tiramisu options.
    tiramisu::init();

    // -------------------------------------------------------
    // Layer I
    // -------------------------------------------------------

    // Declare the function tut_02.
    function tut_02("tut_02");

    // Declare two constants N and M. These constants will be used as loop bounds.
    // The parameters of the constant constructor are as follows:
    //    - The name of the constant is "N".
    //    - The value of the constant is expr((int32_t) NN).
    //    - The constant is of type int32.
    //    - The constant is declared in the beginning of the function tut_02 (i.e.,
    //      its scope is the whole function).
    //    - The next two arguments (NULL and 0) are unused here.
    //    - The last argument is the function in which this constant is declared.
    // For more details please refer to the documentation of the constant constructor.
    constant N_const("N", expr((int32_t) NN), p_int32, true, NULL, 0, &tut_02);
    constant M_const("M", expr((int32_t) MM), p_int32, true, NULL, 0, &tut_02);

    // Declare iterator variables.
    var i("i"), j("j");

    // Declare a wrapper around the input.
    // In Tiramisu, if a function reads an input buffer (or writes to it), that buffer
    // cannot be accessed directly, but should first be wrapped in a dummy computation.
    // This is mainly because computations in Tiramisu do not access memory directly,
    // since the algorithm is supposed to be expressed independently of how data is stored.
    // Therefor all algorithms (computations) access other computations.  The actual data
    // layout is only specified later in Layer III.
    // A wrapper is usually declared with an empty expression and is not supposed to be scheduled
    // (check the documentation of the computation constructor for more details).
    computation input("[N, M]->{input[i,j]: 0<=i<N and 0<=j<M}", expr(), false, p_uint8, &tut_02);

    // Declare expression and output computation.
    expr e = input(i, j) + cast(p_uint8, i) + (uint8_t)4;
    computation output("[N, M]->{output[i,j]: 0<=i<N and 0<=j<M}", e, true, p_uint8, &tut_02);

    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------

    // Set the schedule of the computation.
    var i0("i0"), i1("i1"), j0("j0"), j1("j1");
    
    // Tile the i, j loop around output by a 2x2 tile. The names of iterators
    // in the resulting loop are i0, j0, i1, j1.
    output.tile(i, j, 2, 2, i0, j0, i1, j1);
  
    // Parallelize the outermost loop i0 (OpenMP style parallelism).
    output.parallelize(i0);

    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------

    // Declare input and output buffers.
    buffer b_input("b_input", {expr(NN), expr(MM)}, p_uint8, a_input, &tut_02);
    buffer b_output("b_output", {expr(NN), expr(MM)}, p_uint8, a_output, &tut_02);

    // Map the computations to a buffer.
    // The following call indicates that each computation input[i,j]
    // is stored in the buffer element b_input[i,j] (one-to-one mapping).
    // This is the most common mapping to memory.
    input.store_in(&b_input);
    output.store_in(&b_output);

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------

    // Set the arguments to tut_02
    tut_02.codegen({&b_input, &b_output}, "build/generated_fct_developers_tutorial_02.o");

    // Some debugging
    tut_02.dump_halide_stmt();

    return 0;
}

/**
 * Current limitations:
 * - Note that the type of the invariants N and M are "int32_t". This is
 *   important because these invariants are used later as loop bounds and the
 *   type of the bounds and the iterators should be the same for correct code
 *   generation. This implies that the invariants should be of type "int32_t".
 */
