#include "cgraph/graph.h"
#include "cgraph/iter.h"
#include "graph.h"

#include <stdlib.h>
#include <string.h>

typedef enum { NODE_VALUE = 1, NODE_COMPUTE = 2, NODE_BOTH = 3 } NodeType;

typedef struct {
  NodeType type;
  float value;
  int soln_count; // solution count
  GeomInt ref_count;

  GeomInt indegree; // sub-graph
  GeomId dep_head; // eid
  ValueEval eval;
} GraphNode;

typedef struct {
  GeomSize front, rear;
  GeomId *elems;
} Queue;

static struct {
  CGraph graph;
  GraphNode *nodes;
  GeomId *dep_next;
  Queue queue;
} internal;

static void graph_vert_resize_callback(CGraphSize, CGraphSize);
static void graph_edge_resize_callback(CGraphSize, CGraphSize);
static void graph_get_inputs(const GraphNode *node, float *inputs);
static void graph_link_inputs(GeomId id, GeomSize input_size,
                              const GeomId *inputs, float *input_values);
static void graph_init_indegree(Queue *queue, CGraphIter *iter);
static void graph_spread_invalid(Queue *queue, CGraphIter *iter);

static void enqueue(Queue *q, const GeomId id) { q->elems[q->rear++] = id; }
static GeomId dequeue(Queue *q) { return q->elems[q->front++]; }
static void queue_clear(Queue *q) { q->front = q->rear = 0; }
static bool queue_empty(const Queue *q) { return q->front == q->rear; }

void computation_graph_init(const GeomSize init_size) {
  internal.nodes = malloc(init_size * sizeof(GraphNode));
  internal.dep_next = malloc(init_size * sizeof(GeomId));
  internal.queue.elems = malloc(init_size * sizeof(GeomId));
  memset(internal.dep_next, -1, init_size * sizeof(GeomId));

  cgraphInit(&internal.graph, true, init_size, init_size);
  cgraphSetVertResizeCallback(&internal.graph, graph_vert_resize_callback);
  cgraphSetEdgeResizeCallback(&internal.graph, graph_edge_resize_callback);
}

void computation_graph_cleanup() {
  free(internal.nodes);
  free(internal.dep_next);
  free(internal.queue.elems);
  cgraphRelease(&internal.graph);
}

void computation_graph_clear() {
  memset(internal.dep_next, -1, internal.graph.edgeCap * sizeof(GeomId));
  cgraphClear(&internal.graph);
}

GeomId graph_add_value(const float value) {
  const GeomId id = (GeomId)cgraphAddVert(&internal.graph);
  GraphNode *node = internal.nodes + id;
  node->type = NODE_VALUE;
  node->soln_count = 1;
  node->value = value;
  node->ref_count = 0;
  node->indegree = 0;
  node->dep_head = -1;
  node->eval = NULL;
  return id;
}

static void graph_combine(const GeomId id, const GeomSize input_size,
                          const GeomId *inputs, const ValueEval eval) {
  GraphNode *node = internal.nodes + id;
  node->type = NODE_BOTH;
  node->eval = eval;

  float input_values[6], *output_value = &node->value;
  graph_link_inputs(id, input_size, inputs, input_values);

  node->soln_count = eval(input_values, &output_value);
}

GeomId graph_add_constraint(const GeomSize input_size, const GeomId *inputs,
                            const GeomSize output_size, const GeomId *outputs,
                            const ValueEval eval) {
  if (output_size == 1) {
    graph_combine(outputs[0], input_size, inputs, eval);
    return outputs[0];
  }

  const GeomId node_id = (GeomId)cgraphAddVert(&internal.graph);
  GraphNode *node = internal.nodes + node_id;
  node->type = NODE_COMPUTE;
  node->ref_count = (GeomInt)output_size;
  node->indegree = 0;
  node->eval = eval;

  float input_values[6], *output_values[6];
  graph_link_inputs(node_id, input_size, inputs, input_values);

  for (GeomSize i = 0; i < output_size; i++) {
    const GeomId output_id = outputs[i];
    const GeomId eid =
        (GeomId)cgraphPushEdgeBack(&internal.graph, node_id, output_id);
    internal.nodes[output_id].dep_head = eid;
    output_values[i] = &internal.nodes[output_id].value;
  }

  node->soln_count = eval(input_values, output_values);
  if (node->soln_count == 0) {
    for (GeomSize i = 0; i < output_size; i++) {
      internal.nodes[outputs[i]].soln_count = 0;
    }
  }
  return node_id;
}

float graph_get_value(const GeomId id) { return internal.nodes[id].value; }

bool graph_is_valid(const GeomSize count, const GeomId *ids) {
  for (GeomSize i = 0; i < count; i++) {
    if (internal.nodes[ids[i]].soln_count == 0) return false;
  }
  return true;
}

bool graph_is_degenerate(const GeomId id) {
  return internal.nodes[id].soln_count == 1;
}

void graph_ref_value(const GeomId id) { internal.nodes[id].ref_count++; }

// TODO FIX: 当删除部分OUTPUTS时，COMPUTE计算bug
void graph_unref_value(const GeomSize count, const GeomId *ids) {
  queue_clear(&internal.queue);
  for (GeomSize i = 0; i < count; i++) {
    const GeomId id = ids[i];
    if (id < 0) continue;
    if (--internal.nodes[id].ref_count <= 0) enqueue(&internal.queue, id);
  }

  while (!queue_empty(&internal.queue)) {
    const GeomId node_id = dequeue(&internal.queue);
    for (GeomId dep = internal.nodes[node_id].dep_head; dep != -1;
         dep = internal.dep_next[dep]) {
      CGraphId from, to;
      cgraphParseEdgeId(&internal.graph, dep, &from, &to);
      cgraphDeleteEdge(&internal.graph, dep);
      if (--internal.nodes[from].ref_count == 0) {
        enqueue(&internal.queue, (GeomId)from);
      }
    }
    cgraphDeleteVert(&internal.graph, node_id);
  }
}

void graph_change_value(const GeomSize count, const GeomId *ids,
                        const float *values) {
  if (count == 0) return;

  GeomSize invalid_size = 0;
  Queue *queue = &internal.queue;
  CGraphIter *iter = cgraphGetIter(&internal.graph);

  queue_clear(queue);
  for (GeomSize i = 0; i < count; i++) {
    enqueue(queue, ids[i]);
    internal.nodes[ids[i]].value = values[i];
  }
  graph_init_indegree(queue, iter);

  queue->front = 0;
  queue->rear = count;
  while (!queue_empty(queue)) {
    const GeomId id = dequeue(queue);
    GraphNode *node = internal.nodes + id;

    CGraphId eid, to;
    cgraphIterResetEdge(iter, id);
    switch (node->type) {
    case NODE_COMPUTE: {
      GeomId output_ids[6];
      float inputs[6], *outputs[6];
      graph_get_inputs(node, inputs);

      int out_size = 0;
      while (cgraphIterNextEdge(iter, id, &eid, &to)) {
        output_ids[out_size] = (GeomId)to;
        outputs[out_size] = &internal.nodes[to].value;
        out_size++;
      }

      node->soln_count = node->eval(inputs, outputs);
      if (node->soln_count == 0) {
        queue->elems[invalid_size++] = id;
        break;
      }

      for (int i = 0; i < out_size; i++) {
        if (--internal.nodes[output_ids[i]].indegree == 0) {
          enqueue(&internal.queue, output_ids[i]);
        }
      }
      break;
    }
    case NODE_BOTH: {
      float inputs[6], *output = &node->value;
      graph_get_inputs(node, inputs);
      if (!node->eval(inputs, &output)) {
        node->soln_count = 0;
        queue->elems[invalid_size++] = id;
        break;
      }
    }
    // no break
    case NODE_VALUE:
      node->soln_count = 1;
      while (cgraphIterNextEdge(iter, id, &eid, &to)) {
        if (--internal.nodes[to].indegree == 0) {
          enqueue(&internal.queue, (GeomId)to);
        }
      }
    }
  }

  queue->front = 0;
  queue->rear = invalid_size;
  graph_spread_invalid(queue, iter);
  cgraphIterRelease(iter);
}

static void graph_vert_resize_callback(const CGraphSize old_cap,
                                       const CGraphSize new_cap) {
  void *mem = realloc(internal.nodes, new_cap * sizeof(GraphNode));
  if (!mem) abort();
  internal.nodes = mem;
  mem = realloc(internal.queue.elems, new_cap * sizeof(GeomId));
  if (!mem) abort();
  internal.queue.elems = mem;
}

static void graph_edge_resize_callback(const CGraphSize old_cap,
                                       const CGraphSize new_cap) {
  void *mem = realloc(internal.dep_next, new_cap * sizeof(GeomId));
  if (!mem) abort();
  internal.dep_next = mem;
  memset(internal.dep_next + old_cap, -1, (new_cap - old_cap) * sizeof(GeomId));
}

static void graph_link_inputs(const GeomId id, const GeomSize input_size,
                              const GeomId *inputs, float *input_values) {
  GeomId *pred_ptr = &internal.nodes[id].dep_head;
  for (GeomSize i = 0; i < input_size; i++) {
    const GeomId input_id = inputs[i];
    const GeomId eid = (GeomId)cgraphAddEdge(&internal.graph, input_id, id);
    *pred_ptr = eid;
    pred_ptr = internal.dep_next + eid;
    internal.nodes[input_id].ref_count++;
    input_values[i] = internal.nodes[input_id].value;
  }
}

static void graph_get_inputs(const GraphNode *node, float *inputs) {
  int i = 0;
  for (CGraphId dep = node->dep_head; dep != -1; dep = internal.dep_next[dep]) {
    CGraphId from, to;
    cgraphParseEdgeId(&internal.graph, dep, &from, &to);
    inputs[i++] = internal.nodes[from].value;
  }
}

static void graph_init_indegree(Queue *queue, CGraphIter *iter) {
  bool *visited = calloc(internal.graph.vertCap, sizeof(bool));
  while (!queue_empty(queue)) {
    const GeomId id = dequeue(queue);
    visited[id] = true;

    CGraphId eid, to;
    cgraphIterResetEdge(iter, id);
    while (cgraphIterNextEdge(iter, id, &eid, &to)) {
      internal.nodes[to].indegree++;
      if (!visited[to]) {
        visited[to] = true;
        enqueue(queue, (GeomId)to);
      }
    }
  }
  free(visited);
}

static void graph_spread_invalid(Queue *queue, CGraphIter *iter) {
  while (!queue_empty(queue)) {
    const GeomId id = dequeue(queue);
    GraphNode *node = internal.nodes + id;
    node->soln_count = 0;

    CGraphId eid, to;
    cgraphIterResetEdge(iter, id);
    while (cgraphIterNextEdge(iter, id, &eid, &to)) {
      if (--internal.nodes[to].indegree == 0) {
        enqueue(&internal.queue, (GeomId)to);
      }
    }
  }
}