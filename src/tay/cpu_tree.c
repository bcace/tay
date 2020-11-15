// #include "tree.h"
// #include "state.h"
// #include "thread.h"
// #include "space_impl.h"
// #include <stdlib.h>
// #include <assert.h>


// typedef struct {
//     TreeBase base;
// } Tree;

// static Tree *_init(int dims, float4 radii, int max_depth_correction) {
//     Tree *tree = malloc(sizeof(Tree));
//     tree_init(&tree->base, dims, radii, max_depth_correction);
//     return tree;
// }

// static void _destroy(TaySpaceContainer *container) {
//     Tree *tree = container->storage;
//     tree_destroy(&tree->base);
// }

// static void _add(TaySpaceContainer *container, TayAgentTag *agent, int group, int index) {
//     Tree *tree = container->storage;
//     tree_add(&tree->base, agent, group);
// }

// static void _on_step_start(TayState *state) {
//     Tree *tree = state->space.storage;
//     tree_update(&tree->base);
// }

// typedef struct {
//     Tree *tree;
//     TayPass *pass;
//     int counter; /* tree cell skipping counter so each thread processes different seer nodes */
// } SeeTask;

// static void _init_tree_see_task(SeeTask *task, Tree *tree, TayPass *pass, int thread_index) {
//     task->tree = tree;
//     task->pass = pass;
//     task->counter = thread_index;
// }

// static void _thread_traverse_seen(Tree *tree, Cell *seer_cell, Cell *seen_cell, TayPass *pass, Box seer_box, TayThreadContext *thread_context) {
//     for (int i = 0; i < tree->base.dims; ++i)
//         if (seer_box.min.arr[i] > seen_cell->box.max.arr[i] || seer_box.max.arr[i] < seen_cell->box.min.arr[i])
//             return;
//     if (seen_cell->first[pass->seen_group]) /* if there are any "seen" agents */
//         tay_see(seer_cell->first[pass->seer_group], seen_cell->first[pass->seen_group], pass->see, pass->radii, tree->base.dims, thread_context);
//     if (seen_cell->lo)
//         _thread_traverse_seen(tree, seer_cell, seen_cell->lo, pass, seer_box, thread_context);
//     if (seen_cell->hi)
//         _thread_traverse_seen(tree, seer_cell, seen_cell->hi, pass, seer_box, thread_context);
// }

// static void _thread_traverse_seen_non_recursive(Tree *tree, Cell *seer_cell, Cell *seen_cell, TayPass *pass, Box seer_box, TayThreadContext *thread_context) {
//     TreeBase *base = &tree->base;

//     Cell *stack[16];
//     int stack_size = 0;
//     stack[stack_size++] = base->cells;

//     float4 radii = pass->radii;
//     int dims = base->dims;

//     while (stack_size) {
//         Cell *seen_cell = stack[--stack_size];

//         while (seen_cell) {

//             for (int i = 0; i < base->dims; ++i) {
//                 if (seer_box.min.arr[i] > seen_cell->box.max.arr[i] || seer_box.max.arr[i] < seen_cell->box.min.arr[i]) {
//                     seen_cell = 0;
//                     goto SKIP_CELL;
//                 }
//             }

//             if (seen_cell->hi) {
//                 stack[stack_size++] = seen_cell->hi;
//                 assert(stack_size < 16);
//             }

//             TayAgentTag *seer_agent = seer_cell->first[pass->seer_group];
//             while (seer_agent) {
//                 float4 seer_p = TAY_AGENT_POSITION(seer_agent);

//                 TayAgentTag *seen_agent = seen_cell->first[pass->seen_group];
//                 while (seen_agent) {
//                     float4 seen_p = TAY_AGENT_POSITION(seen_agent);

//                     if (seer_agent == seen_agent)
//                         goto SKIP_SEE;

//                     for (int k = 0; k < dims; ++k) {
//                         float d = seer_p.arr[k] - seen_p.arr[k];
//                         if (d < -radii.arr[k] || d > radii.arr[k])
//                             goto SKIP_SEE;
//                     }

//                     pass->see((void *)seer_agent, (void *)seen_agent, thread_context->context);

//                     SKIP_SEE:;
//                     seen_agent = seen_agent->next;
//                 }

//                 seer_agent = seer_agent->next;
//             }

//             seen_cell = seen_cell->lo;

//             SKIP_CELL:;
//         }
//     }
// }

// static void _thread_traverse_seers(SeeTask *task, Cell *cell, TayThreadContext *thread_context) {
//     if (task->counter % runner.count == 0) {
//         if (cell->first[task->pass->seer_group]) { /* if there are any "seeing" agents */
//             Box seer_box = cell->box;
//             for (int i = 0; i < task->tree->base.dims; ++i) {
//                 seer_box.min.arr[i] -= task->pass->radii.arr[i];
//                 seer_box.max.arr[i] += task->pass->radii.arr[i];
//             }
//             _thread_traverse_seen(task->tree, cell, task->tree->base.cells, task->pass, seer_box, thread_context);
//             // _thread_traverse_seen_non_recursive(task->tree, cell, task->tree->base.cells, task->pass, seer_box, thread_context);
//         }
//     }
//     ++task->counter;
//     if (cell->lo)
//         _thread_traverse_seers(task, cell->lo, thread_context);
//     if (cell->hi)
//         _thread_traverse_seers(task, cell->hi, thread_context);
// }

// static void _thread_see_traverse(SeeTask *task, TayThreadContext *thread_context) {
//     _thread_traverse_seers(task, task->tree->base.cells, thread_context);
// }

// static void _see(TayState *state, int pass_index) {
//     Tree *tree = state->space.storage;
//     TayPass *pass = state->passes + pass_index;

//     static SeeTask tasks[TAY_MAX_THREADS];

//     for (int i = 0; i < runner.count; ++i) {
//         SeeTask *task = tasks + i;
//         _init_tree_see_task(task, tree, pass, i);
//         tay_thread_set_task(i, _thread_see_traverse, task, pass->context);
//     }

//     tay_runner_run();
// }

// typedef struct {
//     Tree *tree;
//     TayPass *pass;
//     int counter; /* tree cell skipping counter so each thread processes different seer nodes */
// } ActTask;

// static void _init_tree_act_task(ActTask *task, Tree *tree, TayPass *pass, int thread_index) {
//     task->tree = tree;
//     task->pass = pass;
//     task->counter = thread_index;
// }

// static void _thread_traverse_actors(ActTask *task, Cell *cell, TayThreadContext *thread_context) {
//     if (task->counter % runner.count == 0) {
//         if (cell->first[task->pass->seer_group]) { /* if there are any "seeing" agents */
//             for (TayAgentTag *tag = cell->first[task->pass->seer_group]; tag; tag = tag->next)
//                 task->pass->act(tag, thread_context->context);
//         }
//     }
//     ++task->counter;
//     if (cell->lo)
//         _thread_traverse_actors(task, cell->lo, thread_context);
//     if (cell->hi)
//         _thread_traverse_actors(task, cell->hi, thread_context);
// }

// static void _thread_act_traverse(ActTask *task, TayThreadContext *thread_context) {
//     _thread_traverse_actors(task, task->tree->base.cells, thread_context);
// }

// static void _act(TayState *state, int pass_index) {
//     Tree *tree = state->space.storage;
//     TayPass *pass = state->passes + pass_index;

//     static ActTask tasks[TAY_MAX_THREADS];

//     for (int i = 0; i < runner.count; ++i) {
//         ActTask *task = tasks + i;
//         _init_tree_act_task(task, tree, pass, i);
//         tay_thread_set_task(i, _thread_act_traverse, task, pass->context);
//     }

//     tay_runner_run();
// }

// void space_cpu_tree_init(TaySpaceContainer *container, int dims, float4 radii, int max_depth_correction) {
//     space_container_init(container, _init(dims, radii, max_depth_correction), dims, _destroy);
//     container->add = _add;
//     container->see = _see,
//     container->act = _act;
//     container->on_step_start = _on_step_start;
// }

// void cpu_tree_step(TayState *state) {
//     _on_step_start(state);
//     for (int i = 0; i < state->passes_count; ++i) {
//         TayPass *pass = state->passes + i;
//         if (pass->type == TAY_PASS_SEE)
//             _see(state, i);
//         else if (pass->type == TAY_PASS_ACT)
//             _act(state, i);
//         else
//             assert(0); /* unhandled pass type */
//     }
// }
