# Tay

Tay is a simulation runtime for spatial ABMs containing a collection of space partitioning and multithreading setups with the goal of testing their suitability for running efficient simulations of different types of models.

## Where does the performance (usually) go?

Agent interactions take up most of the simulation run time. However, in most models interactions between agents are limited in some way (not all agents have to interact) and that provides the opportunity for optimizations. Limiting interactions is usually done by having agents interact through connections ([interaction topology](https://en.wikipedia.org/wiki/Network_topology)), or agents interacting only with other agents that are within some given range (proximity). Interactions through proximity can be direct or through a shared environment ([particle mesh method](https://en.wikipedia.org/wiki/Particle_Mesh)). Of course, any of these methods can be combined; for example, agents can interact with other agents through their connections and every few steps update those connections by searching for best candidates to connect with among agents that are close enough.

## Space partitioning

If interactions are spatially limited then the usual procedure is to split the space into partitions, either a grid or a tree structure. When space is partitioned only neighboring partitions' agents have to be considered for interactions. So if finding neighboring partitions represents the *broad phase* of neighbor search, checking if agents are actually within the required interaction distance from each other before actually interacting is the *narrow phase*. Of course, *broad phase* should never leave out agent pairs that should interact (false negatives), but it will always include some pairs that should not interact (false positives), hence the need for the *narrow phase*. After that the job of the space partitioning algorithm is to balance between the overhead of managing the structure (updating, neighbor partitions search) if partitions are too small, and having to handle too many false positives if partitions are too large.

## Multithreading

[Readers-writers](https://en.wikipedia.org/wiki/Readers%E2%80%93writers_problem) problem is a common concurrency problem, and, as elaborated in [ochre](https://github.com/bcace/ochre) in ABM simulations we can solve the problem of concurrent read and write by double-buffering, and ensure correct *results* of multiple concurrent writes by using only commutative operations, but we still have to make sure that those commutative writes don't actually happen at the same time.

For this we can introduce a notion of a *see* agent interaction, a piece of code that describes interaction between two agents where one of those agents "perceives" the other. So both agents have their distinct roles in the context of that interaction: *seer* agent whose state changes as a result of that interaction (by a commutative write), and a *seen* agent whose state is read-only (in that thread, in another thread that same agent can have the role of the *seer* agent and because of double-buffering whatever writes happen in the second thread won't clash with any reads from the first thread).

Once we have the interaction code where we know which of the two agents is read-only and which changes state, we can schedule that code to be executed in multiple threads, we only have to make sure that an agent is a *seer* agent only in one thread, but can be a *seen* agent in multiple threads.

Between interaction sections of a simulation step agents sometimes need action (*act*) passes, pieces of code that describe what each agent does by itself, usually to perform the *swap* phase of a double-buffered perception and then act on the perceived information. Since there is no interaction between agents at this time, these passes can be trivially multithreaded.

> Since *act* pass is O(n), where worst case for a *see* phase is O(n^2) it's not usually that important to multithread the *act* phase, but there could be cases where the *act* code is so complicated and slow that the *act* pass starts competing with the *see* passes for time, so we do it anyway.

## Implemented setups

### Tree

#### Tree depths

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

#### Tree structure

Currently the space is partitioned with an unbalanced k-d tree where both leaf and non-leaf nodes can contain (multiple) agents. Leaf nodes can contain multiple agents because, as described above, tree depth is limited. Non-leaf nodes must be able to contain agents because that's the most practical way of handling agents that are not points (which is exactly why a grid instead of a tree would not be appropriate).

The entire tree is updated at each step because agents are expected to move quickly, and profiling shows that updating the tree takes very little time (143 samples in tree update which includes teardown and buildup, compared to 12156 samples from worker threads). This is expected because tree update process consists of a single pass through the tree where agents are removed from each node, then a single pass where agents are sorted into a new tree. Creating new nodes along the way is a cheap operation which consists of incrementing a counter on the node pool and some very quick node initialization.

(tree implementation description, the way space is partitioned and how execution is multithreaded)

Once the tree is updated it can be used to efficiently execute proximity tests and interactions between agents of neighboring tree nodes. Each tree node searches the tree for neighboring nodes such that their bounding boxes plus the interaction radius overlap.

(actions can be multithreaded even more easily)

Before describing how this phase is multithreaded I should explain what kind of interaction between agents is performed. Most relevant to performance of multithreading is that the two agents in this interaction have two distinct roles: "seer" and "seen". Seer is the agent that reads values from the seen agent. This means that in this relationship seer agent is the one whose values get changes and seen agent is the one whose values only get read (don't change).

If we can rely on interaction code being written so that one agent is written to and other is read from we can see how to easily multithread execution from the perspective of a tree structure. Each thread does the same traversal through the tree where each encountered node is a "seer" node, and each "seer" node from within its thread searches the tree for neighboring "seen" nodes (meaning nodes which contain agents that assume "seer" or "seen" role). Of course, each thread doesn't find neighbors for each tree node,

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
