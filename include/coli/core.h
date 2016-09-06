#ifndef _H_COLI_CORE_
#define _H_COLI_CORE_

#include <isl/set.h>
#include <isl/map.h>
#include <isl/union_map.h>
#include <isl/union_set.h>
#include <isl/ast_build.h>
#include <isl/schedule.h>
#include <isl/schedule_node.h>
#include <isl/space.h>

#include <map>
#include <string.h>

#include <Halide.h>
#include <coli/debug.h>

namespace coli
{

class function;
class computation;
class buffer;
class invariant;


namespace argument
{
	/**
	 * Types of function arguments.
	 */
	enum type {input, output, internal};
}


/**
  * A class that holds all the global variables necessary for COLi.
  * It also holds COLi options.
  */
class global
{
private:
	static bool auto_data_mapping;

public:
	/**
	  * If this option is set to true, COLi automatically
	  * modifies the computation data mapping whenever a new
	  * schedule is applied to a computation.
	  * If it is set to false, it is up to the user to set
	  * the right data mapping before code generation.
	  */
	static void set_auto_data_mapping(bool v)
	{
		global::auto_data_mapping = v;
	}

	/**
	  * Return whether auto data mapping is set.
	  * If auto data mapping is set, COLi automatically
	  * modifies the computation data mapping whenever a new
	  * schedule is applied to a computation.
	  * If it is set to false, it is up to the user to set
	  * the right data mapping before code generation.
	  */
	static bool get_auto_data_mapping()
	{
		return global::auto_data_mapping;
	}

	static void set_default_coli_options()
	{
		set_auto_data_mapping(true);
	}

	global()
	{
		set_default_coli_options();
	}
};


/**
  * A class to represent functions.  A function is composed of
  * computations (of type coli::computation).
  */
class function
{
private:
	/**
	  * The name of the function.
	  */
	std::string name;

	/**
	  * Function arguments.  These are the buffers or scalars that are
	  * passed to the function.
	  */
	std::vector<Halide::Argument> arguments;

	/**
	  * A vector representing the invariants of the function.
	  * An object of the invariant class is usually used to repsent
	  * function invariants (symbolic constants or
	  * variables that are invariant to the function i.e. do not change
	  * their value during the execution of the function).
	  */
	std::vector<coli::invariant> invariants;

	/**
	  * An isl context associate with the function.
	  */
	isl_ctx *ctx;

	/**
	  * An isl AST representation for the function.
	  * The isl AST is generated by calling gen_isl_ast().
	  */
	isl_ast_node *ast;

	/**
	  * A vector representing the parallel dimensions.
	  * A parallel dimension is identified using the pair
	  * <computation_name, level>, for example the pair
	  * <S0, 0> indicates that the loop with level 0
	  * (i.e. the outermost loop) around the computation S0
	  * should be parallelized.
	  */
	std::map<std::string, int> parallel_dimensions;

	/**
	  * A vector representing the vector dimensions.
	  * A vector dimension is identified using the pair
	  * <computation_name, level>, for example the pair
	  * <S0, 0> indicates that the loop with level 0
	  * (i.e. the outermost loop) around the computation S0
	  * should be vectorized.
	  */
	std::map<std::string, int> vector_dimensions;


public:
	/**
	  * Body of the function (a vector of computations).
	  * The order of the computations in the vector do not have any
	  * effect on the actual order of execution of the computations.
	  * The order of execution of computations is specified through the
	  * schedule.
	  */
	std::vector<computation *> body;

	/**
	  * A Halide statement that represents the whole function.
	  * This value stored in halide_stmt is generated by the code generator.
	  */
	Halide::Internal::Stmt *halide_stmt;

	/**
	  * List of buffers of the function.  Some of these buffers are passed
	  * to the function as arguments and some are declared and allocated
	  * within the function itself.
	  */
	std::map<std::string, Halide::Buffer *> buffers_list;

public:

	function(std::string name): name(name) {
		assert((name.length()>0) && ("Empty function name"));

		halide_stmt = NULL;

		// Allocate an isl context.  This isl context will be used by
		// the isl library calls within coli.
		ctx = isl_ctx_alloc();
	};

	/**
	  * Get the arguments of the function.
	  */
	std::vector<Halide::Argument> get_arguments()
	{
		return arguments;
	}

	/**
	  * Get the name of the function.
	  */
	std::string get_name()
	{
		return name;
	}

	/**
	  * Return a vector representing the invariants of the function.
	  */
	std::vector<coli::invariant> get_invariants()
	{
		return invariants;
	}

	/**
	  * Return the Halide statement that represents the whole
	  * function.
	  * The Halide statement is generated by the code generator so
	  * before calling this function you should first generate the
	  * Halide statement.
	  */
	Halide::Internal::Stmt get_halide_stmt()
	{
		return *halide_stmt;
	}

	/**
	  * Return the computations of the function.
	  */
	std::vector<computation *> get_computations()
	{
		return body;
	}

	/**
	  * Add an invariant to the function.
	  */
	void add_invariant(coli::invariant param);

	/**
	  * Add a computation to the function.  The order in which
	  * computations are added to the function is not important.
	  * The order of execution is specified using the schedule.
	  */
	void add_computation(computation *cpt);

	/**
	  * Set the arguments of the function.
	  * The arguments in the vector will be the arguments of the function
	  * (with the order of their appearance in the vector).
	  */
	void set_arguments(std::vector<coli::buffer *> buffer_vec);

	/**
	  * This functions applies to the schedule of each computation
	  * in the function.  It makes the dimensions of the ranges of
	  * all the schedules equal.  This is done by adding dimensions
	  * equal to 0 to the range of schedules.
	  * This function is called automatically when gen_isl_ast()
	  * or gen_time_processor_domain() are called.
	  */
	void align_schedules();

	/**
	 * This functions applies to the schedule of each computation
	 * in the function.  It computes the maximal
	 * dimension among the dimensions of the ranges of all the
	 * schedules.
	 */
	int get_max_schedules_range_dim();

	/**
	  * Dump the iteration domain of the function.
	  */
	void dump_iteration_domain();

	/**
	  * Dump the schedule of the computations of the function.
	  * This is mainly useful for debugging.
	  * The schedule is a relation between the iteration space and a
	  * time space.  The relation provides a logical date of execution for
	  * each point in the iteration space.
	  * The schedule needs first to be set before calling this function.
	  */
	void dump_schedule();

	/**
	  * Dump the function on stdard output (dump most of the fields of
	  * the function class).
	  * This is mainly useful for debugging.
	  */
	void dump();

	// ----------------------------------
	// ----------------------------------
	// ----------------------------------

	/**
	  * Return the computation that has the name \p str.
	  * This function assumes that the function has only one computation
	  * that has that name.
	  */
	computation *get_computation_by_name(std::string str);

	/**
	  * Return true if the computation \p comp should be parallelized
	  * at the loop level \p lev.
	  */
	bool should_parallelize(std::string comp, int lev)
	{
		assert(comp.length() > 0);
		assert(lev >= 0);

		const auto &iter = this->parallel_dimensions.find(comp);
		if (iter == this->parallel_dimensions.end())
		{
	    	return false;
		}
		return (iter->second == lev);
	}

	/**
	  * Return true if the computation \p comp should be vectorized
	  * at the loop level \p lev.
	  */
	bool should_vectorize(std::string comp, int lev)
	{
		assert(comp.length() > 0);
		assert(lev >= 0);

		const auto &iter = this->vector_dimensions.find(comp);
		if (iter == this->vector_dimensions.end())
		{
	    	return false;
		}
		return (iter->second == lev);
	}

	/**
	  * Tag the dimension \p dim of the computation \p computation_name to
	  * be parallelized.
	  * The outermost loop level (which corresponds to the leftmost
	  * dimension in the iteration space) is 0.
	  */
	void add_parallel_dimension(std::string computation_name, int vec_dim);

	/**
	  * Tag the dimension \p dim of the computation \p computation_name to
	  * be parallelized.
	  * The outermost loop level (which corresponds to the leftmost
	  * dimension in the iteration space) is 0.
	  */
	void add_vector_dimension(std::string computation_name, int vec_dim);

	/**
	  * Return the union of all the iteration domains
	  * of the computations of the function.
	  */
	isl_union_set *get_iteration_domain();

	/**
	  * Return the union of all the schedules
	  * of the computations of the function.
	  */
	isl_union_map *get_schedule();

	/**
	  * Return the isl context associated with this function.
	  */
	isl_ctx *get_ctx()
	{
		return ctx;
	}

	/**
	  * Return the isl ast associated with this function.
	  */
	isl_ast_node *get_isl_ast()
	{
		assert((ast != NULL) && ("You should generate an ISL ast first (gen_isl_ast())."));

		return ast;
	}

	/**
	  * Return the time-processor domain of all the computations
	  * of the function.
	  * In the time-processor representation, the logical time of
	  * execution and the processor where the computation will be
	  * executed are both specified.
	  */
	isl_union_set *get_time_processor_domain();

	/**
	  * Generate an object file.  This object file will contain the compiled
	  * function.
	  * \p obj_file_name indicates the name of the generated file.
	  * \p os indicates the target operating system.
	  * \p arch indicates the architecture of the target (the instruction set).
	  * \p bits indicate the bit-width of the target machine.
	  *    must be 0 for unknown, or 32 or 64.
	  * For a full list of supported value for \p os and \p arch please
	  * check the documentation of Halide::Target
	  * (http://halide-lang.org/docs/struct_halide_1_1_target.html).
	  * If the machine parameters are not supplied, it will detect one automatically.
	  */
	// @{
	void gen_halide_obj(std::string obj_file_name, Halide::Target::OS os,
			Halide::Target::Arch arch, int bits);

	void gen_halide_obj(std::string obj_file_name) {
		Halide::Target target = Halide::get_host_target();
		gen_halide_obj(obj_file_name, target.os, target.arch, target.bits);
	}
	// @}

	/**
	  * Generate C code on stdout.
	  * Currently C code code generation is very basic and does not
	  * support many features compared to the Halide code generator.
	  */
	void gen_c_code();

	/**
	  * Generate an isl AST for the function.
	  */
	void gen_isl_ast();

	/**
	  * Generate a Halide stmt for the function.
	  */
	void gen_halide_stmt();

	/**
	  * Generate the time-processor domain of the
	  * function.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */
	void gen_time_processor_domain();

	/**
	  * Set the isl context associated with this class.
	  */
	void set_ctx(isl_ctx *ctx)
	{
		this->ctx = ctx;
	}

	/**
	  * Dump (on stdout) the time processor domain of the function.
	  * This is mainly useful for debugging.
	  */
	void dump_time_processor_domain();

	/**
	  * Dump the Halide stmt of the function.
	  * gen_halide_stmt should be called before calling this function.
	  */
	void dump_halide_stmt();
};


/**
  * A class that represents a buffer.  The result of a computation
  * can be stored in a buffer.  A computation can also be a binding
  * to a buffer (i.e. a buffer element is represented as a computation).
  */
class buffer
{
	/**
	  * The name of the buffer.
	  */
	std::string name;

	/**
	  * The number of dimensions of the buffer.
	  */
	int nb_dims;

	/**
	  * The size of buffer dimensions.  Assuming the following
	  * buffer: buf[N0][N1][N2].  The first vector element represents the
	  * leftmost dimension of the buffer (N0), the second vector element
	  * represents N1, ...
	  */
	std::vector<int> dim_sizes;

	/**
	  * The type of the elements of the buffer.
	  */
	Halide::Type type;

	/**
	  * Buffer data.
	  */
	uint8_t *data;

	/**
	  * The coli function where this buffer is declared or where the
	  * buffer is an argument.
	  */
	coli::function *fct;

	/**
	  * Is the buffer passed as an argument to the function ?
	  */
	bool is_arg;

	/**
	 * Type of the argument.
	 */
	coli::argument::type argtype;

public:
	/**
	  * Create a coli buffer where computations can be stored
	  * or buffers bound to computation.
	  * \p name is the name of the buffer.
	  * \p nb_dims is the number of dimensions of the buffer.
	  * A scalar is a one dimensional buffer with one element.
	  * \p dim_sizes is a vector of integers that represent the size
	  * of each dimension in the buffer.
	  * \p type is the type of the buffer.
	  * \p data is the data stored in the buffer.  This is useful
	  * for binding a computation to an already existing buffer.
	  * \p fct is i a pointer to a coli function where the buffer is
	  * declared/used.
	  * \p is_argument indicates whether the buffer is passed to the
	  * function as an argument.  All the buffers passed as arguments
	  * to the function should be allocated by the user outside the
	  * function.  Buffers that are allocated within the function cannot
	  * be used outside the function.
	  */
	buffer(std::string name, int nb_dims, std::vector<int> dim_sizes,
		Halide::Type type, uint8_t *data, bool is_argument,
		coli::argument::type argt, coli::function *fct):
		name(name), nb_dims(nb_dims), dim_sizes(dim_sizes), type(type),
		data(data), fct(fct)
	{
		assert(name.length()>0 && "Empty buffer name");
		assert(nb_dims>0 && "Buffer dimensions <= 0");
		assert(nb_dims == dim_sizes.size() && "Mismatch in the number of dimensions");
		assert(fct != NULL && "Input function is NULL");

		Halide::Buffer *buf = new Halide::Buffer(type, dim_sizes, data, name);
		fct->buffers_list.insert(std::pair<std::string, Halide::Buffer *>(buf->name(), buf));

		this->is_arg = is_argument;
		if (this->is_arg == true)
			argtype = argt;
		else
			argtype = coli::argument::internal;
	};

	/**
	  * Return the name of the buffer.
	  */
	std::string get_name()
	{
		return name;
	}

	Halide::Type get_type()
	{
		return type;
	}

	/**
	  * Get the number of dimensions of the buffer.
	  */
	int get_n_dims()
	{
		return nb_dims;
	}

	/**
	  * Return the type of the argument.
	  */
	coli::argument::type get_argument_type()
	{
		return argtype;
	}
};



/**
  * A class that represents computations.
  */
class computation {
private:
	isl_ctx *ctx;

	/**
	  * Time-processor domain of the computation.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */
	isl_set *time_processor_domain;

	/**
	  * Iteration domain of the computation.
	  * In this representation, the order of execution of computations
	  * is not specified, the computations are also not mapped to memory.
	 */
	isl_set *iteration_domain;

	/**
	  * Initialize a computation.
	  * This is a private function that should not be called explicitely
	  * by users.
	  */
	void init_computation(std::string iteration_space_str, coli::function *fct) {
		assert(fct != NULL);
		assert(iteration_space_str.length()>0 && ("Empty iteration space"));

		// Initialize all the fields to NULL (useful for later asserts)
		index_expr = NULL;
		access = NULL;
		schedule = NULL;
		stmt = Halide::Internal::Stmt();
		time_processor_domain = NULL;

		this->ctx = fct->get_ctx();

		iteration_domain = isl_set_read_from_str(ctx, iteration_space_str.c_str());
		name = std::string(isl_space_get_tuple_name(isl_set_get_space(iteration_domain), isl_dim_type::isl_dim_set));
		function = fct;
		function->add_computation(this);
		this->set_identity_schedule();
	}

public:

	/**
	  * Schedule of the computation.
	  */
	isl_map *schedule;

	/**
	  * The function where this computation is declared.
	  */
	coli::function *function;

	/**
	  * The name of this computation.
	  */
	std::string name;

	/**
	  * Halide expression that represents the computation.
	  */
	Halide::Expr expression;

	/**
	  * Halide statement that assigns the computation to a buffer location.
	  */
	Halide::Internal::Stmt stmt;

	/**
	  * Access function.  A map indicating how each computation should be stored
	  * in memory.
	  */
	isl_map *access;

	/**
	  * An isl_ast_expr representing the index of the array where
	  * the computation will be stored.  This index is computed after the scheduling is done.
	  */
	isl_ast_expr *index_expr;

	/**
  	  * A number identifying the root dimension level.
  	  * THis should be used with computation::after().
	  */
	const static int root_dimension = -1;

	/**
	  * Create a computation and make it represent an expression.
	  *
	  * \p iteration_space_str is a string that represents the iteration
	  * space of the computation.  The iteration space should be encoded
	  * using the ISL set format. For details about the ISL format for set
	  * please check the ISL documentation:
	  * http://isl.gforge.inria.fr/user.html#Sets-and-Relations
	  *
	  * The iteration space of a statement is a set that contains
	  * all execution instances of the statement (a statement in a loop
	  * has an execution instance for each loop iteration upon which
	  * it executes). Each execution instance of a statement in a loop
	  * nest is uniquely represented by an identifier and a tuple of
	  * integers  (typically,  the  values  of  the  outer  loop  iterators).
	  * These  integer  tuples  are  compactly  described  by affine constraints.
	  * The iteration space of the statement S0 in the following loop nest
	  * for (i=0; i<N; i++)
	  *   for (j=0; j<M; j++)
	  *      S0;
	  *
	  * is
	  *
	  * {S0[i,j]: 0<=i<N and 0<=j<N}
	  *
	  * THis should be read as: the set of point (i,j) such that
	  * 0<=i<N and 0<=j<N.
	  *
	  * \p fct is a pointer to the coli function where this computation
	  * should be added.
	  */
	computation(std::string iteration_space_str, Halide::Expr expr, coli::function *fct) {
		init_computation(iteration_space_str, fct);

		this->expression = expr;
	}

	/**
	  * Return the access function of the computation.
	  */
	isl_map *get_access()
	{
		return access;
	}

	/**
	  * Return the function where the computation is declared.
	  */
	coli::function *get_function()
	{
		return function;
	}

	/**
	  * Return the iteration domain of the computation.
	  * In this representation, the order of execution of computations
	  * is not specified, the computations are also not mapped to memory.
	  */
	isl_set *get_iteration_domain()
	{
		// Every computation should have an iteration space.
		assert(iteration_domain != NULL);

		return iteration_domain;
	}

	/**
	  * Return the time-processor domain of the computation.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */
	isl_set *get_time_processor_domain()
	{
		return time_processor_domain;
	}

	/**
	  * Return the schedule of the computation.
	  */
	isl_map *get_schedule()
	{
		return this->schedule;
	}

	/**
	  * Return the name of the computation.
	  */
	std::string get_name()
	{
		return name;
	}

	/**
	  * Return the context of the computations.
	  */
	isl_ctx *get_ctx()
	{
		return ctx;
	}

	/**
	  * Tag the dimension \p dim of the computation to be parallelized.
	  * The outermost loop level (which corresponds to the leftmost
	  * dimension in the iteration space) is 0.
	  */
	void tag_parallel_dimension(int dim);

	/**
	  * Tag the dimension \p dim of the computation to be vectorized.
	  * The outermost loop level (which corresponds to the leftmost
	  * dimension in the iteration space) is 0.
	  */
	void tag_vector_dimension(int dim);

	/**
	  * Generate the time-processor domain of the computation.
	  * In this representation, the logical time of execution and the
	  * processor where the computation will be executed are both
	  * specified.
	  */
	void gen_time_processor_domain()
	{
		assert(this->get_iteration_domain() != NULL);
		assert(this->get_schedule() != NULL);

		time_processor_domain = isl_set_apply(
				isl_set_copy(this->get_iteration_domain()),
				isl_map_copy(this->get_schedule()));
	}

	/**
	  * Schedule this computation to run after the comp computation
	  * at dimension dim of the time-processor space.
	  * Use computation::root_dimension to indicate the root dimension
	  * (i.e. the outermost processor-time dimension).
	  */
	void after(computation &comp, int dim);

	void set_access(std::string access_str)
	{
		assert(access_str.length() > 0);

		this->access = isl_map_read_from_str(this->ctx, access_str.c_str());
	}

	void create_halide_assignement();

	/**
	  * Set the identity schedule.
	  */
	void set_identity_schedule()
	{
		isl_space *sp = isl_set_get_space(this->get_iteration_domain());
		isl_map *sched = isl_map_identity(isl_space_map_from_set(sp));
		sched = isl_map_intersect_domain(sched,
				isl_set_copy(this->get_iteration_domain()));
		sched = isl_map_set_tuple_name(sched, isl_dim_out, "");
		sched = isl_map_coalesce(sched);
		IF_DEBUG2(coli::str_dump("\nThe following identity schedule is set: ",
					isl_map_to_str(sched)));
		this->set_schedule(sched);
	}

	/**
	  * Tile the two dimension \p inDim0 and \p inDim1 with rectangular
	  * tiling.  \p sizeX and \p sizeY represent the tile size.
	  * \p inDim0 and \p inDim1 should be two consecutive dimensions,
	  * and the should satisfy \p inDim0 > \p inDim1.
	  */
	void tile(int inDim0, int inDim1, int sizeX, int sizeY);

	/**
	 * Modify the schedule of this computation so that it splits the
	 * dimension inDim0 of the iteration space into two new dimensions.
	 * The size of the inner dimension created is sizeX.
	 */
	void split(int inDim0, int sizeX);

	/**
	 * Modify the schedule of this computation so that the two dimensions
	 * inDim0 and inDime1 are interchanged (swaped).
	 */
	void interchange(int inDim0, int inDim1);

	/**
	  * Set the mapping from iteration space to time-processor space.
	  * The name of the domain and range space must be identical.
	  * The input string must be in the ISL map format.
	  */
	void set_schedule(std::string map_str);

	/**
	  * Set the mapping from iteration space to time-processor space.
	  * The name of the domain and range space must be identical.
	  */
	void set_schedule(isl_map *map)
	{
		this->schedule = map;
	}

	/**
	  * Bind the computation to a buffer.
	  * i.e. create a one-to-one data mapping between the computation
	  * the buffer.
	  */
	void bind_to(buffer *buff)
	{
		assert(buff != NULL);

		isl_space *sp = isl_set_get_space(this->get_iteration_domain());
		isl_map *map = isl_map_identity(isl_space_map_from_set(sp));
		map = isl_map_intersect_domain(map,
				isl_set_copy(this->get_iteration_domain()));
		map = isl_map_set_tuple_name(map, isl_dim_out,
						buff->get_name().c_str());
		map = isl_map_coalesce(map);
		IF_DEBUG2(coli::str_dump("\nBinding.  The following access function is set: ",
					isl_map_to_str(map)));
		this->set_access(isl_map_to_str(map));
	}


	/**
	  * Dump the iteration domain of the computation.
	  * This is useful for debugging.
	  */
	void dump_iteration_domain();

	/**
	  * Dump the schedule of the computation.
	  * This is mainly useful for debugging.
	  * The schedule is a relation between the iteration space and a
	  * time space.  The relation provides a logical date of execution for
	  * each point in the iteration space.
	  * The schedule needs first to be set before calling this function.
	  */
	void dump_schedule();

	/**
	  * Dump (on stdout) the computation (dump most of the fields of the
	  * computation class).
	  * This is mainly useful for debugging.
	  */
	void dump();
};


/**
  * A class that represents loop invariants.
  * An object of the invariant class can be an expression, a symbolic constant
  * or a variable that is invariant to all the loops of the function.
  */
class invariant
{
private:
	// An expression that represents the invariant.
	Halide::Expr expr;

	// The name of the variable holding the invariant.
	std::string name;

public:
	/**
	  * Create an invariant where \p param_name is the name of
	  * the variable that will hold the value of the invariant.
	  * \p param_expr is the expression that defines the value
	  * of the invariant.
	  * \p func is the function in which the invariant is defined.
	  */
	invariant(std::string param_name, Halide::Expr param_expr,
			coli::function *func)
	{
		assert((param_name.length() > 0) && "Parameter name empty");
		assert((param_expr.defined()) && "Parameter undefined");
		assert((func != NULL) && "Function undefined");

		this->name = param_name;
		this->expr = param_expr;
		func->add_invariant(*this);
	}

	/**
	  * Return the name of the invariant. i.e. the name of the variable
	  * that is used to store the value of the value of the invariant.
	  */
	std::string get_name()
	{
		return name;
	}

	/**
	  * Return the expression that represents the value of the invariant.
	  */
	Halide::Expr get_expr()
	{
		return expr;
	}
};


namespace parser
{

/**
  * A class to hold parsed tokens of an isl_space.
  * This class is useful in parsing isl_space objects.
  */
class space
{
private:
	std::vector<std::string> constraints;

public:
	std::vector<std::string> dimensions;

	/**
	  * Parse an isl_space.
	  * The isl_space is a string in the format of ISL.
	  */
	space(std::string isl_space_str)
	{
		assert(isl_space_str.empty() == false);
		this->parse(isl_space_str);
	};

	space() {};

	/**
	  * Return a string that represents the parsed string.
	  */
	std::string get_str()
	{
		std::string result;

		for (int i=0; i<dimensions.size(); i++)
		{
			if (i != 0)
				result = result + ",";
			result = result + dimensions.at(i);
		}

		return result;
	};

	void add_constraint(std::string cst)
	{
		constraints.push_back(cst);
	}

	std::vector<std::string> get_constraints()
	{
		return constraints;
	}

	void replace(std::string in, std::string out1, std::string out2)
	{
		std::vector<std::string> new_dimensions;

		for (auto dim:dimensions)
		{
			if (dim.compare(in) == 0)
			{
				new_dimensions.push_back(out1);
				new_dimensions.push_back(out2);
			}
			else
				new_dimensions.push_back(dim);
		}

		dimensions = new_dimensions;
	}

	void parse(std::string space);
	bool empty() {return dimensions.empty();};
};


/**
 * A class to hold parsed tokens of isl_constraints.
 */
class constraint
{
public:
	std::vector<std::string> constraints;
	constraint() { };

	void parse(std::string str);

	void add(std::string str)
	{
		assert(str.empty() == false);
		constraints.push_back(str);
	}

	void add_constraints(std::vector<std::string> vec)
	{
		for (auto cst:vec)
			constraints.push_back(cst);
	}

	std::string get_str()
	{
		std::string result;

		for (int i=0; i<constraints.size(); i++)
		{
			if (i != 0)
				result = result + " and ";
			result = result + constraints.at(i);
		}

		return result;
	};

	bool empty() {return constraints.empty();};
};


/**
  * A class to hold parsed tokens of isl_maps.
  */
class map
{
public:
	coli::parser::space parameters;
	std::string domain_name;
	std::string range_name;
	coli::parser::space domain;
	coli::parser::space range;
	coli::parser::constraint constraints;

	map(std::string map_str)
	{
		int map_begin =  map_str.find("{")+1;
		int map_end   =  map_str.find("}")-1;

		assert(map_begin != std::string::npos);
		assert(map_end != std::string::npos);

		int domain_space_begin = map_str.find("[", map_begin)+1;
		int domain_space_begin_pre_bracket = map_str.find("[", map_begin)-1;
		int domain_space_end   = map_str.find("]", map_begin)-1;

		assert(domain_space_begin != std::string::npos);
		assert(domain_space_end != std::string::npos);

		domain_name = map_str.substr(map_begin,
		 		             domain_space_begin_pre_bracket-map_begin+1);

		std::string domain_space_str =
			map_str.substr(domain_space_begin,
		 		       domain_space_end-domain_space_begin+1);

		domain.parse(domain_space_str);

		int pos_arrow = map_str.find("->", domain_space_end);
		int first_char_after_arrow = pos_arrow + 2;

		assert(pos_arrow != std::string::npos);

		int range_space_begin = map_str.find("[", first_char_after_arrow)+1;
		int range_space_begin_pre_bracket = map_str.find("[", first_char_after_arrow)-1;
		int range_space_end = map_str.find("]", first_char_after_arrow)-1;

		assert(range_space_begin != std::string::npos);
		assert(range_space_end != std::string::npos);

		range_name = map_str.substr(first_char_after_arrow,
		 		             range_space_begin_pre_bracket-first_char_after_arrow+1);
		std::string range_space_str = map_str.substr(range_space_begin,
							 range_space_end-range_space_begin+1);
		range.parse(range_space_str);
		constraints.add_constraints(range.get_constraints());

		int column_pos = map_str.find(":")+1;

		if (column_pos != std::string::npos)
		{
			std::string constraints_str = map_str.substr(column_pos,
								     map_end-column_pos+1);
			constraints.parse(constraints_str);
		}

		if (DEBUG2)
		{
			coli::str_dump("Parsing the map : " + map_str + "\n");
			coli::str_dump("The parsed map  : " + this->get_str() + "\n");
		}
	};

	std::string get_str()
	{
		std::string result;

		result = "{" + domain_name + "[" + domain.get_str() + "] ->" +
			  range_name + "[" +
			  range.get_str() + "]";

		if (constraints.empty() == false)
			result = result + " : " + constraints.get_str();

		result = result + " }";

		return result;
	};

	isl_map *get_isl_map(isl_ctx *ctx)
	{
		return isl_map_read_from_str(ctx, this->get_str().c_str());
	};
};

}

// Halide IR specific functions

void halide_stmt_dump(Halide::Internal::Stmt s);

}

#endif
