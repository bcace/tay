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

# Space partitioning

(why not grid? why not quad/octree?)
(minimizing false positives)
(tree implementation description, the way space is partitioned and how the execution is multithreaded)
(optimum between minimizing false positives by increasing tree depth and minimizing tree traversal overhead by decreasing tree depth, related to interaction radii and can be adjusted at runtime automatically by sampling run-times of both)

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
