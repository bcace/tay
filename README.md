# Tay

Tay is a spatial agent-based simulation library that focuses on simulation speed.

Agent-based simulation speed quickly decreases with the increase of interactions between agents, and the best approach to optimizing the simulation depends to a high degree on the model itself: are agents points or do they vary in size? Do they interact at certain range, or through fixed references, or through a grid? Do all agents move or are some of them stationary? Are agents evenly distributed in space or are they tightly grouped in certain locations? Sometimes it even depends on hardware used: is GPU available for simulation, or is it already busy with some demanding rendering?

Tay tries to cover all these cases with a set of *interchangeable* and *composable* space partitioning structures. *Interchangeable* means that it's easy to experiment which structure best suits the model, even during later stages of model development. *Composable* means that agents can be divided into several different structures, each chosen to be optimal for its agents while never interfering with their ability to interact.

> Note that Tay is still in its experimental stage, there's still a lot of features to add, but progress can be seen [here](https://bcace.github.io/tay.html), where I use a series of tests to compare how different structures work in different scenarios (*benchmark* directory), and [here](https://www.youtube.com/watch?v=DD93xIQqz5s), where I try to showcase some of Tay's features (*flocking* directory).

(Tay decouples the infrastucture needed to run efficient simulations (managing structures, storage, worker threads, synchronization with GPU) from the model itself (agent definitions and behavior).)

## Tay basics

A a library written in C Tay is meant to used from other C code with which it can share data and code (simple C structs and C function pointers). `TayState` object contains the environment that contains the model definition and runs simulations mased on that model.

```C
float4 partition_radii = { 10.0f, 10.0f, 10.0f };
TayState *tay = tay_create_state(3, partition_radii);

/* define the model, run simulations */

tay_destroy_state(tay);
```

#### Agent types

Agent types are simply C structs that as first two members must contain a `TayAgentTag` structure for Tay's intenal use and a `float4` structure representing the agent's position in space.

```C
/* our agent type, defined in host code */
struct Agent {
    TayAgentTag tag;    /* space reserved for Tay, must be first member */
    float4 p;           /* agent position, must be float4 and immediately follow tag */
    int a;              /* some agent variable */
};

/* we let Tay know about the new type, and reserve storage for 100000 agents of that type */
TayGroup *agent_type = tay_add_group(tay, sizeof(Agent), 100000);
```

#### Agent behavior

Agent behavior is just a set of C functions with predefined arguments that describe either what a single agent does, or how two agents interact. These functions are simply passed to Tay as function pointers. In case of GPU simulations, agent behavior code is passed as a C string containing OpenCL C code similar to that of normal C functions.

As described in [this](https://bcace.github.io/ochre.html) post, if agents use shared memory to communicate directly, in order to avoid data races and race conditions

```C
tay_add_see(tay, agent_type, agent_type, agent_see, "agent_see", see_radii, 0, 0);
tay_add_act(tay, agent_type, agent_act, "agent_act", 0, 0);
```

### Simulation setup and running

Then we allocate agents, initialize their data and add them to the space (storage is in the TayGroup object, adding to a space just makes the agent "live"):

```C
Agent *agent = tay_get_available_agent(tay, agent_type); /* gets first available "dead" agent from storage */

/* initialize agent data here... */

tay_commit_available_agent(tay, agent_type); /* makes the agent "live" */
```

Then we can run the simulation:

```C
double average_milliseconds_per_step = tay_run(tay, 100, TAY_SPACE_CPU_GRID, 1);
```

and get new agent data:

```C
Agent *agent = tay_get_agent(tay, agent_type, agent_index);
```

## Space partitioning structures

(list with short descriptions)

## What's next?

Currently I use Tay as a vehicle to better understand what's necessary for efficient agent-based simulation, in future it should become

(In future, through its application in more complex simulations, it should become a library for use in any agent-based simulation and, more importantly, easily configured for optimal simulation running.)

These are some of the more significant things to do in that direction:

* Automatic adjustment of `depth_correction`.
* Support for non-point agents in the `CpuTree` structure.
* Combining multiple structures in a single simulation.
* More control over when structures get updated.
* Efficient communication through references or connections.
* Efficient communication through particle grids.
* Implement and benchmark the new `CpuAABBTree` structure.
* Implement thread balancing for the `CpuTree` structure.
* Add a simple sampling profiler to measure time spent updating and using structures.
