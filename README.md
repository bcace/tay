# Tay

Efficient simulation runtime for spatial ABMs.

To quote [Methodological Issues of Spatial Agent-Based Models](http://jasss.soc.surrey.ac.uk/23/1/3.html), "Spatial ABMs are those where space is an explicit component in the model, and geographical proximity effects form part of model behavior", but in general space doesn't have to consist of just physical space dimensions, it can also contain any other dimension which can be used to constrain by proximity which agents interact.

When running simulations with large numbers of agents their interactions can dramatically slow down the simulation, and if simulated model is such that it requires that all agents interact there's not much that can be done. Often the case is that agents only have to interact with a limited number of other agents, and this limitation is broadly done in two different ways: proximity or explicit connections. Limiting interaction by proximity can be done in two main ways: finding neighboring agents and executing their interactions, or agents interacting through a shared environment ([a very nice example](https://observablehq.com/@rreusser/gpgpu-boids)). For this project I (for now) concentrated on finding neighboring agents using [space parititioning](https://en.wikipedia.org/wiki/Space_partitioning), and executing as much of that work as possible in parallel, on multiple threads.

To ensure this simulation runtime is flexible enough for most ABMs the following requirements must be met:

* Number of space dimensions should be variable
* Space should not have "walls", agents should not be forced to "bounce" off the edges or jump to the opposite side just because of the defficiencies of the implementation
* Space should be able to handle agents that are not points (lines, polygons or bodies) equally efficiently (e.g. stationary terrain and building polygons as agents)
* Any other type of interaction (e.g. connections) should work alongside the proximity system without any problems
* (arbitrary number of action and interaction passes)
* (arbitrary number of agent types (groups of agents with different attributes and behaviors) to which those actions and interactions apply)
* Skipping random agents/interactions for a number of simulation steps should be supported (e.g. agent types that should be simulated at the same time but at different speeds)

# Space partitioning

If we don't use space partitioning in models where agents only interact with other agents if they're within some range we have to test proximity of each pair of agents, and in the case where 4000 agents move in a 400 x 400 x 400 box and interact at radius of 40, in 100 simulation steps we have to perform 1599600000 tests of which only 28991382 are positive (pair of agents that are close enough to interact). With the tree structure implemented in this project number of tests can be reduced to 71612980 (for the same number of actual interactions). Of course, this reduction isn't free, we just replace the large number of proximity tests with building the tree and its traversal to find neighboring nodes whose agents should be tested for interaction. Reducing this overhead is not simple because there are two conflicting influences: tree node traversal on one hand (reduced by decreasing tree depth) and proximity testing between agents of nodes that have been found to be close enough (reduced by increasing tree depth), which means an optimum tree depth can be found. Since both influences depend on space size and agent density, both of which can change over time, this optimal tree depth should be adjusted dynamically.

## Tree depths

Currently an initial tree depth is calculated for each dimension individually and an additional parameter that increases or decreases these initial depths is varied to show the position of this optimum for different models and cases. For example, for the case described above ...

depth_correction: 0
interactions/tests: 28991382/737226964
run time: 1.87762 sec, 53.2589 fps

depth_correction: 1
interactions/tests: 28991382/154761512
run time: 0.717073 sec, 139.456 fps

depth_correction: 2
interactions/tests: 28991382/95648922
run time: 0.752878 sec, 132.824 fps

depth_correction: 3
interactions/tests: 28991382/71612980
run time: 1.27378 sec, 78.5067 fps

reference:
depth_correction: 0
interactions/tests: 28991382/1599600000
run time: 4.1433 sec, 24.1353 fps

All the experiments above were executed on 8 threads, and the last "reference" experiment is the case with no space partitioning but still multithreaded.

(sampling run-times for adjusting tree depth automatically)

## Tree structure

For space partitioning an unbalanced k-d tree is used where space is always split in half, both leaf and non-leaf nodes can contain agents and tree depth is limited in each dimension individually according to interaction radius in that dimension and length of the smallest bounding box containing all agents in the same dimension.

(why not grid? why not quad/octree?)
(tree implementation description, the way space is partitioned and how execution is multithreaded)

# Multithreading

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
