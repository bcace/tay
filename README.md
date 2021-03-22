# Tay

Tay is an embeddable framework for fast agent-based simulations.

Agent-based models can differ drastically, not just in how agents act and interact once they come into contact, but also in agent size, velocity and position distributions, how the agents move, and which channels they use to interact (e.g. proximity, particle mesh, or direct references). Other than optimizing agent behavior and multi-threading the simulation execution, the most significant optimization in agent-based simulations comes from choosing the best method for quick neighbor-finding, and which method is most appropriate depends heavily on all the model characteristics mentioned above.

Tay provides a set of *interchangeable* and *composable* space partitioning structures. *Interchangeable* means that it's easy to experiment which structure best suits the model, throughout model development. *Composable* means that agents can be divided into several different structures, each chosen to be optimal for its agents while not interfering with their ability to interact.

This repo contains the Tay source in the `tay/tay` directory, tests to compare structures' performance in different test simulations and verify their correctness in the `tay/benchmark` directory, and a showcase application in the `tay/flocking` directory. Development progess is documented in a series of posts [here](https://bcace.github.io), and you can also see  [here](https://www.youtube.com/watch?v=DD93xIQqz5s), where I try to showcase some of Tay's features (*flocking* directory).

## Tay basics

As a library written in C Tay is meant to used from other C code with which it can share data and code directly (agents as simple C structs and agent behavior through function pointers). `TayState` object is the environment that contains model definition and runs simulations based on that model.

```C
TayState *tay = tay_create_state();

/* define the model and run simulations between these two calls */

tay_destroy_state(tay);
```

#### Agents and agent groups

Agents are simply C structs with some special members meant for internal use by the Tay library. First member must be a `TayAgentTag` structure (8 bytes) followed immediately by a single `float4` structure representing the agent's position in space if agent is a point agent, or two `float4` structures (min and max) if agent is a non-point agent.

```C
/* our agent type, defined in host code */
struct Agent {
    TayAgentTag tag;    /* space reserved for Tay, must be first member */
    float4 p;           /* agent position, must be float4 and immediately follow tag */
    int a;              /* some agent variable */
};
```

Agent group is basically an agent pool for agents of a single type, and it also ...

```C
/* before we define the agent group we describe the initial space structure it will use for simulation */
TaySpaceDesc space_desc;
space_desc.space_type = TAY_CPU_AABB_TREE;  /* using the AABB tree structure */
space_desc.space_dims = 3;                  /* 3D space */
space_desc.part_radii = (float4){10.0f, 10.0f, 10.0f, 10.0f}; /* minimum partition sizes */
space_desc.shared_size_in_megabytes = 200; /* memory used internally by the structure */

/* create the new agent group and reserve storage for 100000 agents of that type */
TayGroup *group = tay_add_group(tay, sizeof(Agent), 100000, TAY_TRUE, space_desc);
```

#### Agent behavior

Agent behavior is just a set of C functions with predefined arguments that describe either what a single agent does or how two agents interact. These functions are simply passed to Tay as function pointers. In case of GPU simulations, agent behavior code is passed as a C string containing OpenCL C code similar to that of regular C functions.

As described in [this](https://bcace.github.io/ochre.html) post, if agents use shared memory to communicate directly, in order to avoid data races and race conditions

```C
tay_add_see(tay, group, group, agent_see, "agent_see", see_radii, 0, 0);
tay_add_act(tay, group, agent_act, "agent_act", 0, 0);
```

### Simulation setup and running

Then we allocate agents, initialize their data and add them to the space (storage is in the TayGroup object, adding to a space just makes the agent "live"):

```C
Agent *agent = tay_get_available_agent(tay, group); /* gets first available "dead" agent from storage */

/* initialize agent data here... */

tay_commit_available_agent(tay, group); /* makes the agent "live" */
```

Then we can run the simulation:

```C
double average_milliseconds_per_step = tay_run(tay, 100, TAY_SPACE_CPU_GRID, 1);
```

and get new agent data:

```C
Agent *agent = tay_get_agent(tay, group, agent_index);
```

## Space partitioning structures

(list with short descriptions)

## What's next?

Currently I use Tay as a vehicle to better understand what's necessary for efficient agent-based simulation, in future it should become

(In future, through its application in more complex simulations, it should become a library for use in any agent-based simulation and, more importantly, easily configured for optimal simulation running.)

These are some of the more significant things to do in that direction:

* Support for non-point agents in the `CpuTree` structure
* Combining multiple structures in a single simulation
* More control over when structures get updated
* Efficient communication through references or connections
* Efficient communication through particle grids
* Implement and benchmark the new `CpuAABBTree` structure
* Implement thread balancing for the `CpuTree` structure
* Add a simple sampling profiler to measure time spent updating and using structures
* Automatic adjustment of `depth_correction`
