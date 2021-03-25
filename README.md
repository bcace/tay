# Tay

Tay is an embeddable framework for fast spatial agent-based simulations.

Agent-based simulations, just like all simulations in my experience, tend to be slow and scale badly. Generally, there are three main areas for optimization: model code, neighbor-finding and parallelization. Model code is up to the model developer, therefore outside of Tay's scope. Neighbor-finding can be optimized using different space partitioning structures, and the resulting workload can be distributed evenly among different processors (CPU or GPU).

Which space partitioning structure is best suited to a given model depends heavily on the nature of agents in the model: are they point or non-point, do they move or are they static, do they tend to gather into groups or are they uniformly distributed in space, etc. The goal of Tay is to provide a comprehensive set of parallelized space partitioning structures that you can easily swap and test on your models, and even keep different parts of the model in different structures for optimal performance.

This repo contains Tay source (`tay/tay` directory), tests to compare structures' performance in different test cases and verify their correctness (`tay/benchmark` directory), and a showcase application (`tay/flocking` directory). Development progess is documented in a series of posts [here](https://bcace.github.io), and you can also watch [videos](https://www.youtube.com/watch?v=DD93xIQqz5s) where I try to showcase some of Tay's features.

## Overview

Tay is a C library that enables embedding agent-based simulations in host programs. Each simulation is run in its separate Tay state, and host can create as many Tay states as it needs.

```C
TayState *tay = tay_create_state();
/* define model and run simulations */
tay_destroy_state(tay);
```

#### Agents and agent groups

Host defines agent types as regular C structs, and creates an agent group for each of those types. Agent group reserves storage for its agents (agent pool) and maintains the selected space partitioning structure during simulation. When host wants to create an agent it requests an agent from storage, initializes its data, and adds it to the structure.

```C
typedef struct {
    /* agent data */
} MyAgent;

TayState *tay = tay_create_state();
TayGroup *group = tay_add_group(tay, sizeof(MyAgent), 100000, TAY_POINT);

MyAgent *agent = tay_get_available_agent(tay, group);
/* initialize agent's data */
tay_commit_available_agent(tay, group);
```

#### Agent behavior

Agent behavior is defined in host as a set of functions that describe how agents act and interact. These functions have predefined signatures and are passed to Tay as function pointers to be executed at the appropriate time.

As described in [this](https://bcace.github.io/ochre.html) post, to achieve lock-free parallel execution of agent actions and interactions when agents essentially exist in and communicate trough shared memory, actions and interactions are separated into multiple passes.

Currently there are two types of passes: *see* and *act*. *See* passes are used so agents can perceive their surroundings and neighbors (accumulation phase), and *act* passes are used for agents to act on the information they each gathered from their environment (swap phase).

Host function that describes how agents gather information during a *see* pass receives three arguments from Tay: `seer_agent` is the one gathering or accumulating information from the environment, `seen_agent` is the "perceived" agent and should not change its state within this function, and `see_context` is a custom struct defined in host that provides global, read-only data relevant for the pass.

Host function that describes how agents act individually during a *act* pass receives two arguments: `agent` is obviously the acting agent, and `act_context` is global read-only data relevant for that *act* pass.

```C
void agent_see(MyAgent *seer_agent, MyAgent *seen_agent, void *see_context) {
    /* ...  */
}

void agent_act(MyAgent *agent, void *act_context) {
    /* ... */
}
```

Now passes can be defined. *See* passes define for which groups of agents it defines interaction, and with ...

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
