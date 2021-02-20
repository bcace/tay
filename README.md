# Tay

Tay is an agent-based simulation library written in C. It includes various space partitioning structures both for multithreaded execution on CPU or GPU.

Initially, Tay can be used to simply provide a quick and easy way to get efficient agent-based simulation running in C. But since it separates simulation infrastructure from model code and allows combining different space partitioning structures (even switching them while the simulation is running) it allows optimal simulation running throughout its development. For example if some parts of the model are stationary and can be kept in a structure that only has to be built once at the beginning while other agents are moving and require that their structure gets rebuilt every simulation step. Or there could be a difference in size of agents; some are points so they work best in a grid structure, others vary in size - they should probably be in a tree structure. The goal of Tay is to allow all those structures to work together seamlessly.

Currently the library is in experimental state, I'm still building a set of test cases, benchmarking simulation runs and comparing results, all of which can be seen [here](https://bcace.github.io/ochre.html). This repo contains three projects:

* tay - the library itself,
* benchmark - executable that runs simulations for different test cases, checks that all run correctly and logs run-times,
* flocking - a showcase program that uses the library to its fullest potential, see [one of the videos](https://www.youtube.com/watch?v=DD93xIQqz5s).

## How does it work?

First, a Tay state has to be created:

```C
float4 partition_radii = { 10.0f, 10.0f, 10.0f };
TayState *tay = tay_create_state(3, partition_radii);

/* use Tay */

tay_destroy_state(tay);
```

After creating the state we can add some agent types to it:

```C
/* our agent type, defined in host code */
struct Agent {
    TayAgentTag tag;    /* space reserved for Tay, must be first member */
    float4 p;           /* agent position, must be float4 and immediately follow tag */
    int a;              /* some agent variable */
};

TayGroup *agent_type = tay_add_group(tay, sizeof(Agent), 100000);
```

Then we can add some behaviors between agents of those types:

```C
tay_add_see(tay, agent_type, agent_type, agent_see, "agent_see", see_radii, 0, 0);
tay_add_act(tay, agent_type, agent_act, "agent_act", 0, 0);
```

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
