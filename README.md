# Tay

Flexible simulation runtime for spatial ABMs. The goal of this project is to implement various space partitioning and multi-threading setups so that an optimal setup can be found and tweaked for a specific model to run efficiently (either on CPU or GPU). These setups should be interchangeable so that during model development, as the model changes, developers don't have to commit to a specific way of storing agents and executing their interactions - model-specific code and simulation runtime should be completely independent.

## Where does the performance (usually) go?

The biggest contribution to simulation slowdown comes from agent interactions. For example, if we have 1000 agents, and they all have to interact at each step, we will have to execute 999000 interactions at each step. In most models, however, interaction between agents is limited in some way - not all agents have to interact. The way interactions are limited can be either through using connections (only connected agents interact), or through proximity: an interaction distance is defined, and only agents that are close enough actually interact. Interactions with neighbors can happen in two ways: directly or through some kind of environment ([particle mesh method](https://en.wikipedia.org/wiki/Particle_Mesh)). Of course, all these methods can be combined; for example, agents can interact with other agents through their connections and at each 10-th step update those connections by searching for best candidates to connect with among their neighbors.

## Space partitioning

If interactions are spatially limited then the usual procedure is to split the space into partitions, either a grid or a tree structure. When space is partitioned only neighboring partitions' agents have to be considered for interactions. So if finding neighboring partitions represents the *broad phase* of neighbor search, checking if agents are actually within interaction distance from each other before actually interacting is the *narrow phase*. Of course, *broad phase* should never leave out agent pairs that should interact (false negatives), but it will always include some pairs that should not interact (false positives), hence the need for the *narrow phase*. After that the job of the space partitioning algorithm is to balance between the overhead of managing the structure and having to handle too many false positives.

(Grids - not so good with sparse distributions of agents, and not practical for agents that are not points)
(Trees -)

## Multithreading

(To combine multithreading with that we have to know which data is written to and which is read from. Asymmetric interaction code, one agent can write to itself, other one's data is read-only. Once we know that we can organize multi-threaded execution such that an agent can appear as a *seer* only in a single thread, but can appear as a *seen* agent in multiple threads.)

## Tree

### Tree depths

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

### Tree structure

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

## Experiments

(describe models, what is (will be) varied, what is measured, reference implementation)

Model variables:
* number of agents/agent groups
* spatial distribution
* direction and speed distribution
* agent sizes
* number of spatial dimensions
* how expensive see and act codes are

## Results
