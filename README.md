# Tay

Tay is an embeddable framework for fast spatial agent-based simulations.

Just like most simulations in my experience agent-based simulations tend to be slow and scale badly, and generally there are three main areas for optimization: model code, neighbor-finding and parallelization. Model code is up to the model developer, therefore outside of Tay's influence. Neighbor-finding can be optimized using different space partitioning structures, and the resulting workload can be distributed evenly among different processors (CPU or GPU).

Which space partitioning structure is best suited to a given model depends mostly on the nature of agents in the model: if they're point or non-point agents, are they static or do they move, do they tend to gather into groups or are they uniformly distributed in space, etc. The goal of Tay is to provide a comprehensive set of parallelized space partitioning structures that can be easily tested and combined in a model.

This repo contains the Tay library source (`tay/tay` directory), tests to compare structures' performance in different test cases and verify their correctness (`tay/benchmark` directory), and a showcase application (`tay/examples` directory). Development progess is documented in a series of posts [here](https://bcace.github.io), and you can also watch [videos](https://www.youtube.com/watch?v=DD93xIQqz5s) where I try to showcase some of Tay's features.

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

TayGroup *my_group = tay_add_group(tay, sizeof(MyAgent), 100000, TAY_POINT);
MyAgent *agent = tay_get_available_agent(tay, my_group);

/* initialize agent's data */

tay_commit_available_agent(tay, my_group);
```

#### Agent behavior

As described in the [Exploring parallelism in agent-based simulations](https://bcace.github.io/ochre.html) post, to achieve lock-free parallel execution of agent actions and their shared memory communication, Tay requires that agent behavior is separated into multiple passes.

A pass can either be a *see* pass or an *act* pass. During *see* passes agents gather information from the environment (accumulation phase), and during *act* passes agents act individually on the information gathered during previous *see* passes (swap phase).

Host defines pass functions that describe how individual agents act during *act* passes and how pairs of agents interact during *see* passes:

```C
void agent_see(MyAgent *seer_agent, MyAgent *seen_agent, void *see_context) {
    /* change "seer_agent" state based on the "seen_agent" state */
}

void agent_act(MyAgent *agent, void *act_context) {
    /* change "agent" state */
}

/* see pass between agents of the same group */
tay_add_see(tay, my_group, my_group, agent_see, see_radii, 0);

/* act pass for agents of my_group */
tay_add_act(tay, my_group, agent_act, 0);
```

And the host code has to make sure that when in the above example in the `agent_see` function `seer_agent` changes its state (gathers information about its environment - in this case the `seen_agent`) the member variables it writes to are not read from in that same function. Also, `seen_agent` state must be read-only in that function.

During simulation Tay state executes passes, using space partitioning structures to find neighbors (`see_radii` argument), and calls given pass functions on agents making sure there are no data races or race conditions, that threads are never blocking each other, and workload is balanced evenly.

## Available space partitioning structures

When specifying the agent group's structure (`CpuSimple` is the default) host suggests approximate minimum partition sizes and defines the amount of memory the structure will have available for its internal use. If a structure runs out of memory during a simulation it will continue working correctly, but its performance will decrease.

```C
tay_configure_space(tay, my_group,
                    TAY_CPU_AABB_TREE,              /* structure type enum */
                    3,                              /* 3D space, can be 1, 2, 3 or 4 dimensions */
                    min_part_sizes,                 /* minimum partition sizes for each dimension (float4) */
                    internal_memory_in_megabytes    /* memory used by the structure, more memory generally means faster structure */
);
```

`CpuSimple (TAY_CPU_SIMPLE)` Simple non-structure used when all agents have to interact (there's no limit to the distance at which agents interact). This space can contain both point and non-point agents.

`CpuKdTree (TAY_CPU_KD_TREE)` [K-d tree](https://en.wikipedia.org/wiki/K-d_tree) structure. If it runs out of memory it sorts agents into most appropriate available nodes it can find. Can contain both point and non-point agents.

`CpuAabbTree (TAY_CPU_AABB_TREE)` AABB tree structure. If it runs out of memory it stores multiple agents into same nodes. Can only contain non-point agents.

`CpuGrid (TAY_CPU_GRID)` Simple grid structure. If for a given space size there's not enough memory for cell storage, cell size will simply increase, making the structure less efficient.

`CpuHashGrid (TAY_CPU_HASH_GRID)` Hash grid structure. Less available memory will result in more hash collisions, which leads to decreased performance. Can only contain point agents.
