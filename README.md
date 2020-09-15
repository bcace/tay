# Tay

Efficient simulation runtime for spatial ABMs.

To quote [Methodological Issues of Spatial Agent-Based Models](http://jasss.soc.surrey.ac.uk/23/1/3.html), "Spatial ABMs are those where space is an explicit component in the model, and geographical proximity effects form part of model behavior", but in general space doesn't have to consist of just physical space dimensions, it can also contain any other dimension which can be used to constrain by proximity which agents interact.

When running simulations with large numbers of agents their interactions can dramatically slow down the simulation, and if simulated model is such that it requires that all agents interact there's not much that can be done. Often the case is that agents only have to interact with a limited number of other agents, and this limitation is broadly done in two different ways: proximity or explicit connections. Limiting interaction by proximity can be done in two main ways: finding neighboring agents and executing their interactions, or agents interacting through a shared environment ([a very nice example](https://observablehq.com/@rreusser/gpgpu-boids)). For this project I (for now) concentrated on finding neighboring agents using [space parititioning](https://en.wikipedia.org/wiki/Space_partitioning), and executing as much of that work as possible in parallel, on multiple threads.

To ensure this simulation runtime is flexible enough for most ABMs the following requirements must be met:

* Number of space dimensions should be variable
* Space should not have "walls", agents should not be forced to "bounce" off the edges or jump to the opposite side just because of the defficiencies of the implementation
* Space should be able to handle agents that are not points (lines, polygons or bodies) equally efficiently (e.g. stationary terrain and building polygons as agents)
* Any other type of interaction (e.g. connections) should work alongside the proximity system without any problems
* Skipping random agents/interactions for a number of simulation steps should be supported (e.g. agent types that should be simulated at the same time but at different speeds)
* (arbitrary number of action and interaction passes)
* (arbitrary number of agent types (groups of agents with different attributes and behaviors) to which those actions and interactions apply)
* (adding and removing agents)

# Space partitioning

If we don't use space partitioning in models where agents only interact with other agents if they're within some range we have to test proximity of each pair of agents, and in the case where 4000 agents move in a 400 x 400 x 400 box and interact at radius of 40, in 100 simulation steps we have to perform 1599600000 tests of which only 28991382 are positive (pair of agents that are close enough to interact). With the tree structure implemented in this project number of tests can be reduced to 71612980 (for the same number of actual interactions). Of course, this reduction isn't free, we just replace the large number of proximity tests with building the tree and its traversal to find neighboring nodes whose agents should be tested for interaction. Reducing this overhead is not simple because there are two conflicting influences: tree node traversal on one hand (reduced by decreasing tree depth) and proximity testing between agents of nodes that have been found to be close enough (reduced by increasing tree depth), which means an optimum tree depth can be found. Since both influences depend on space size and agent density, both of which can change over time, this optimal tree depth should be adjusted dynamically.

## Tree depths

Currently an initial tree depth is calculated for each dimension individually and an additional parameter that increases or decreases these initial depths is varied to show the position of this optimum for different models and cases. For example, for the case described above ...

(what is depth correction)

| depth correction | interactions | tests | run-time (100 steps)
| --- | --- | --- | ---
| (no partitioning) | 28991382 | 1599600000 | 4.1433
| 0 | 28991382 | 737226964 | 1.87762
| 1 | 28991382 | 154761512 | 0.717073
| 2 | 28991382 | 95648922 | 0.752878
| 3 | 28991382 | 71612980 | 1.27378

All the experiments above were executed on 8 threads, including the first run where space is not partitioned, but the workload is still optimally divided between threads.

(sampling run-times for adjusting tree depth automatically)

## Tree structure

Currently the space is partitioned with an unbalanced k-d tree where both leaf and non-leaf nodes can contain (multiple) agents. Leaf nodes can contain multiple agents because, as described above, tree depth is limited. Non-leaf nodes must be able to contain agents because that's the most practical way of handling agents that are not points (which is exactly why a grid instead of a tree would not be appropriate).

The entire tree is updated at each step because agents are expected to move quickly, and profiling shows that updating the tree takes very little time (143 samples in tree update which includes teardown and buildup, compared to 12156 samples from worker threads). This is expected because tree update process consists of a single pass through the tree where agents are removed from each node, then a single pass where agents are sorted into a new tree. Creating new nodes along the way is a cheap operation which consists of incrementing a counter on the node pool and some very quick node initialization.

(tree implementation description, the way space is partitioned and how execution is multithreaded)

Once the tree is updated it can be used to efficiently execute proximity tests and interactions between agents of neighboring tree nodes. Each tree node searches the tree for neighboring nodes such that their bounding boxes plus the interaction radius overlap.

(actions can be multi-threaded even more easily)

Before describing how this phase is multi-threaded I should explain what kind of interaction between agents is performed. Most relevant to performance of multithreading is that the two agents in this interaction have two distinct roles: "seer" and "seen". Seer is the agent that reads values from the seen agent. This means that in this relationship seer agent is the one whose values get changes and seen agent is the one whose values only get read (don't change).

If we can rely on interaction code being written so that one agent is written to and other is read from we can see how to easily multi-thread execution from the perspective of a tree structure. Each thread does the same traversal through the tree where each encountered node is a "seer" node, and each "seer" node from within its thread searches the tree for neighboring "seen" nodes (meaning nodes which contain agents that assume "seer" or "seen" role). Of course, each thread doesn't find neighbors for each tree node,

 but only for neighbor nodes for certain nodes: if there are 4 threads, then first thread searches neighbors of 0-th, 4-th, 8-th ... nodes, and the second thread searches neighbors of 1-st, 5-th, 9-th ... nodes, and so on. Even if the tree is unbalanced, because of this node skipping workload ends up being well balanced between threads:

| thread | interactions | tests
| --- | --- | ---
| 0 | 3455539 | 18478171
| 1 | 3444997 | 18472097
| 2 | 3612807 | 19238994
| 3 | 3685869 | 19655973
| 4 | 3622106 | 19365190
| 5 | 3764484 | 20042284
| 6 | 3814683 | 20309528
| 7 | 3590897 | 19199275
| all | 28991382 | 154761512

(skipping tree nodes from threads, balanced workloads, determinism, parallelizable interaction and action code)

# Experiments

(describe models, what is (will be) varied, what is measured, reference implementation)

Model variables:
* number of agents/agent groups
* spatial distribution
* direction and speed distribution
* agent sizes
* number of spatial dimensions
* how expensive see and act codes are

# Results
